"""Run the frozen engine through a real checkpoint load and short WAV generation."""
from __future__ import annotations

import json
import math
import os
import struct
import subprocess
import sys
import tempfile
import wave
from pathlib import Path


def write_reference(path: Path) -> None:
    """Write a deterministic 4-second speech-like fixture without user audio."""
    sample_rate = 24000
    frames = []
    for index in range(sample_rate * 4):
        t = index / sample_rate
        envelope = min(1.0, t * 8.0, (4.0 - t) * 8.0)
        f0 = 145.0 + 18.0 * math.sin(2.0 * math.pi * 2.1 * t)
        voiced = (
            0.50 * math.sin(2.0 * math.pi * f0 * t)
            + 0.25 * math.sin(2.0 * math.pi * f0 * 2.0 * t)
            + 0.12 * math.sin(2.0 * math.pi * f0 * 3.0 * t)
        )
        sample = max(-1.0, min(1.0, voiced * envelope * 0.65))
        frames.append(struct.pack("<h", int(sample * 32767)))
    with wave.open(str(path), "wb") as audio:
        audio.setnchannels(1)
        audio.setsampwidth(2)
        audio.setframerate(sample_rate)
        audio.writeframes(b"".join(frames))


def main() -> int:
    executable = Path(sys.argv[1]).resolve()
    with tempfile.TemporaryDirectory(prefix="mettavox-integration-") as data_dir:
        reference = Path(data_dir) / "reference-fixture.wav"
        write_reference(reference)
        requests = (
            {"id": "smoke-selftest", "command": "selftest"},
            {"id": "smoke-status", "command": "status"},
            {
                "id": "smoke-generate",
                "command": "generate",
                "voicePath": str(reference),
                "text": "Olá, este é um teste curto de locução.",
                "pace": 50,
                "expression": 55,
                "pauses": 45,
            },
            {"id": "smoke-shutdown", "command": "shutdown"},
        )
        environment = os.environ.copy()
        environment["VOZLAB_DATA_DIR"] = data_dir
        process = subprocess.Popen(
            [str(executable)], stdin=subprocess.PIPE, stdout=subprocess.PIPE,
            stderr=subprocess.PIPE, text=True, encoding="utf-8", errors="replace", env=environment,
        )
        try:
            stdout, stderr = process.communicate(
                "".join(json.dumps(item, ensure_ascii=False) + "\n" for item in requests),
                timeout=2700,
            )
        except subprocess.TimeoutExpired:
            process.kill()
            stdout, stderr = process.communicate()
            raise RuntimeError(
                "Full model download/load/generation exceeded 45 minutes. "
                f"stdout={stdout!r}; stderr={stderr!r}"
            )

        messages = []
        for line in stdout.splitlines():
            try:
                messages.append(json.loads(line))
            except json.JSONDecodeError:
                continue
        by_id = {item.get("id"): item for item in messages if item.get("id")}
        api = by_id.get("smoke-selftest")
        status = by_id.get("smoke-status")
        generated = by_id.get("smoke-generate")
        if process.returncode != 0 or not api or not api.get("engineInstalled") or not status or not status.get("engineInstalled"):
            detail = (generated or api or status or {}).get("error") or "No valid response from bundled engine."
            raise RuntimeError(
                f"Frozen engine failed: {detail}; exit={process.returncode}; "
                f"stdout={stdout!r}; stderr={stderr!r}"
            )
        if api.get("checkpointFilename") != "t3_mtl23ls_v2.safetensors":
            raise RuntimeError(f"Unexpected local checkpoint mapping: {api!r}")
        if api.get("s3genCheckpointCompat") is not True:
            raise RuntimeError(f"S3 checkpoint compatibility test failed: {api!r}")
        if not generated or not generated.get("ok"):
            raise RuntimeError(f"Real checkpoint load or WAV generation failed: {generated!r}; stderr={stderr!r}")
        output = Path(generated["path"])
        if not output.is_file() or output.stat().st_size <= 44:
            raise RuntimeError(f"Engine reported success without a valid WAV file: {generated!r}")
        with wave.open(str(output), "rb") as audio:
            if audio.getnframes() <= 0:
                raise RuntimeError(f"Generated WAV is empty: {generated!r}")
            audio_info = {
                "sampleRate": audio.getframerate(),
                "frames": audio.getnframes(),
                "channels": audio.getnchannels(),
            }

        for message in messages:
            if message.get("event") == "progress":
                print(f"Model integration: {message.get('message', message.get('phase', 'working'))}")
        print(
            "Frozen engine loaded the real PT-BR checkpoint and generated a valid WAV: "
            + json.dumps({"api": api, "audio": audio_info}, ensure_ascii=False)
        )
        return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as error:
        print(str(error), file=sys.stderr)
        raise SystemExit(1)
