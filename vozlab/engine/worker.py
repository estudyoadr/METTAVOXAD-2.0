"""Persistent local VOZLAB worker for Brazilian Portuguese voice cloning.

The worker speaks JSON Lines over stdin/stdout so Electron can keep the model
loaded between generations. Diagnostics go to stderr; stdout is protocol only.
"""
from __future__ import annotations

import json
import hashlib
import os
import re
import shutil
import sys
import traceback
import unicodedata
import uuid
from pathlib import Path


BASE_REPO = "ResembleAI/chatterbox"
PTBR_REPO = "ResembleAI/Chatterbox-Multilingual-pt-br"
PTBR_CHECKPOINT = "t3_pt_br.safetensors"
LOCAL_T3_FILENAME = "t3_mtl23ls_v2.safetensors"
PTBR_DECODER = "s3gen_v3.pt"
MODEL_FILES = ("ve.pt", "grapheme_mtl_merged_expanded_v1.json")
model = None
model_device = "cpu"
S3GEN_IGNORABLE_MISSING = frozenset({"tokenizer._mel_filters", "tokenizer.window"})


def configure_stdio() -> None:
    """Keep Electron's UTF-8 JSON IPC intact on Windows' legacy console encodings."""
    for stream in (sys.stdin, sys.stdout, sys.stderr):
        if stream is not None and hasattr(stream, "reconfigure"):
            errors = "replace" if stream is sys.stdin else "backslashreplace"
            stream.reconfigure(encoding="utf-8", errors=errors)


configure_stdio()


def emit(payload: dict) -> None:
    sys.stdout.write(json.dumps(payload, ensure_ascii=False) + "\n")
    sys.stdout.flush()


def data_dir() -> Path:
    root = os.environ.get("VOZLAB_DATA_DIR")
    if root:
        return Path(root)
    return Path.home() / "AppData" / "Roaming" / "VOZLAB"


def model_dir() -> Path:
    return data_dir() / "models" / "chatterbox-ptbr"


def installed_status() -> dict:
    try:
        import torch
        from chatterbox.mtl_tts import ChatterboxMultilingualTTS  # noqa: F401

        folder = model_dir()
        required = (*MODEL_FILES, LOCAL_T3_FILENAME, "s3gen.pt")
        ready = all((folder / file).is_file() for file in required)
        device = "CUDA" if torch.cuda.is_available() else "CPU"
        return {"engineInstalled": True, "modelReady": ready, "device": device}
    except Exception as exc:
        return {"engineInstalled": False, "modelReady": False, "device": None, "error": str(exc)}


def install_s3gen_checkpoint_compat() -> None:
    """Allow only two deterministic tokenizer buffers omitted by s3gen_v3.pt."""
    from chatterbox.models.s3gen import S3Gen

    if getattr(S3Gen, "_vozlab_checkpoint_compat", False):
        return

    original_load_state_dict = S3Gen.load_state_dict

    def load_state_dict_compatible(self, state_dict, strict=True, assign=False):
        if not strict:
            return original_load_state_dict(self, state_dict, strict=False, assign=assign)

        result = original_load_state_dict(self, state_dict, strict=False, assign=assign)
        missing = set(result.missing_keys)
        unexpected = set(result.unexpected_keys)
        invalid_missing = missing - S3GEN_IGNORABLE_MISSING
        if invalid_missing or unexpected:
            details = []
            if invalid_missing:
                details.append(f"chaves ausentes: {sorted(invalid_missing)}")
            if unexpected:
                details.append(f"chaves inesperadas: {sorted(unexpected)}")
            raise RuntimeError("Checkpoint S3 incompatível; " + "; ".join(details))
        return result

    S3Gen.load_state_dict = load_state_dict_compatible
    S3Gen._vozlab_checkpoint_compat = True


def verify_s3gen_checkpoint_compat() -> bool:
    """Exercise the allowlist and ensure other missing weights still fail."""
    import torch
    from torch import nn
    from chatterbox.models.s3gen import S3Gen

    install_s3gen_checkpoint_compat()

    class ProbeTokenizer(nn.Module):
        def __init__(self):
            super().__init__()
            self.register_buffer("_mel_filters", torch.ones(2))
            self.register_buffer("window", torch.ones(2))

    class ProbeModule(nn.Module):
        def __init__(self):
            super().__init__()
            self.tokenizer = ProbeTokenizer()
            self.weight = nn.Parameter(torch.ones(1))

    probe = ProbeModule()
    state = probe.state_dict()
    state.pop("tokenizer._mel_filters")
    state.pop("tokenizer.window")
    compatible = S3Gen.load_state_dict(probe, state)
    if set(compatible.missing_keys) != S3GEN_IGNORABLE_MISSING or compatible.unexpected_keys:
        raise RuntimeError(f"Compatibilidade S3 não validou os buffers esperados: {compatible}")

    invalid_state = probe.state_dict()
    invalid_state.pop("weight")
    try:
        S3Gen.load_state_dict(probe, invalid_state)
    except RuntimeError as exc:
        if "chaves ausentes" not in str(exc):
            raise
    else:
        raise RuntimeError("O teste permitiu uma chave de peso obrigatória ausente.")
    return True


def load_model() -> None:
    global model, model_device
    if model is not None:
        return

    import torch
    from huggingface_hub import hf_hub_download, snapshot_download
    from chatterbox.mtl_tts import ChatterboxMultilingualTTS

    folder = model_dir()
    folder.mkdir(parents=True, exist_ok=True)
    emit({"event": "progress", "phase": "assets", "message": "Baixando os componentes do motor PT-BR…"})

    # Keep the model in the user's private app-data folder, outside the install.
    snapshot_download(
        repo_id=BASE_REPO,
        allow_patterns=list(MODEL_FILES),
        local_dir=str(folder),
    )
    # chatterbox-tts 0.1.7 reads this fixed checkpoint filename from_local().
    # Migrate files downloaded by the previous app build without downloading them again.
    checkpoint_target = folder / LOCAL_T3_FILENAME
    legacy_checkpoint = folder / PTBR_CHECKPOINT
    if not checkpoint_target.is_file():
        if legacy_checkpoint.is_file():
            shutil.copyfile(legacy_checkpoint, checkpoint_target)
        else:
            emit({"event": "progress", "phase": "assets", "message": "Baixando o checkpoint PT-BR…"})
            asset = hf_hub_download(
                repo_id=PTBR_REPO,
                filename=PTBR_CHECKPOINT,
                local_dir=str(folder),
            )
            shutil.copyfile(asset, checkpoint_target)

    decoder_target = folder / "s3gen.pt"
    if not decoder_target.is_file():
        emit({"event": "progress", "phase": "assets", "message": "Baixando o decodificador de áudio…"})
        asset = hf_hub_download(
            repo_id=PTBR_REPO,
            filename=PTBR_DECODER,
            local_dir=str(folder),
        )
        if Path(asset).resolve() != decoder_target.resolve():
            shutil.copyfile(asset, decoder_target)

    install_s3gen_checkpoint_compat()
    model_device = "cuda" if torch.cuda.is_available() else "cpu"
    emit({"event": "progress", "phase": "loading", "message": f"Carregando modelo na memória ({model_device.upper()})…"})
    model = ChatterboxMultilingualTTS.from_local(str(folder), device=model_device)
    emit({"event": "progress", "phase": "ready", "message": "Motor de voz pronto."})


def engine_api_selftest() -> dict:
    import inspect
    from chatterbox.mtl_tts import ChatterboxMultilingualTTS

    signature = inspect.signature(ChatterboxMultilingualTTS.from_local)
    parameters = signature.parameters
    if not parameters:
        raise RuntimeError("API incompatível: from_local não recebe a pasta do modelo.")
    first = next(iter(parameters.values()))
    if first.kind not in (inspect.Parameter.POSITIONAL_ONLY, inspect.Parameter.POSITIONAL_OR_KEYWORD):
        raise RuntimeError("API incompatível: from_local não aceita a pasta do modelo como argumento posicional.")
    if "device" not in parameters and not any(p.kind == inspect.Parameter.VAR_KEYWORD for p in parameters.values()):
        raise RuntimeError("API incompatível: from_local não aceita o parâmetro device.")
    compat_ok = verify_s3gen_checkpoint_compat()
    return {
        "engineInstalled": True,
        "fromLocalParameters": list(parameters),
        "checkpointFilename": LOCAL_T3_FILENAME,
        "s3genCheckpointCompat": compat_ok,
    }


def split_sentences(text: str) -> list[str]:
    # Keeping the punctuation at the end of each segment lets the model phrase
    # naturally while the pause control inserts silence between full sentences.
    pieces = re.split(r"(?<=[.!?])\s+", text.strip())
    return [piece.strip() for piece in pieces if piece.strip()]


def generate(request: dict) -> dict:
    import librosa
    import numpy as np
    import soundfile as sf
    import torch

    voice_path = Path(request.get("voicePath", ""))
    raw_text = request.get("text", "")
    if not isinstance(raw_text, str):
        raise ValueError(f"O texto recebido pelo motor não é texto: {type(raw_text).__name__}.")
    # IPC on Windows must preserve accented Portuguese as Unicode text for the tokenizer.
    text = unicodedata.normalize("NFC", raw_text).replace(chr(0), "").strip()
    if not voice_path.is_file():
        raise ValueError("A gravação de referência desta voz não foi encontrada.")
    if not text:
        raise ValueError("Digite o texto da locução.")
    if len(text) > 5000:
        raise ValueError("O texto ultrapassa o limite de 5.000 caracteres.")

    load_model()
    assert model is not None

    # Long demo tracks can contain intros, music, and silence. A local 20 s
    # reference is sufficient for the prompt path and avoids loading minutes
    # of audio into the speaker encoder on every generation.
    reference_cache = data_dir() / "prepared" / (hashlib.sha256(f"{voice_path.resolve()}:{voice_path.stat().st_size}:{voice_path.stat().st_mtime_ns}".encode()).hexdigest()[:20] + ".wav")
    if not reference_cache.is_file():
        reference_cache.parent.mkdir(parents=True, exist_ok=True)
        reference_audio, reference_sr = librosa.load(str(voice_path), sr=model.sr, mono=True, duration=20)
        if reference_audio.size < model.sr * 2:
            raise ValueError("A referência precisa ter pelo menos 2 segundos de fala audível.")
        sf.write(str(reference_cache), reference_audio, reference_sr, subtype="PCM_16")

    expression = min(1.0, max(0.0, float(request.get("expression", 55)) / 100.0))
    pace = min(1.0, max(0.0, float(request.get("pace", 50)) / 100.0))
    pauses = min(1.0, max(0.0, float(request.get("pauses", 45)) / 100.0))
    exaggeration = 0.25 + expression * 0.55
    # Lower CFG can help preserve natural pacing for fast references.
    cfg_weight = 0.25 + (1.0 - pace) * 0.45
    sentence_gap = 0.08 + pauses * 0.72
    sentences = split_sentences(text)

    emit({"event": "progress", "phase": "generating", "message": "Gerando a locução…", "total": len(sentences)})
    audio_parts = []
    for index, sentence in enumerate(sentences, start=1):
        # Pass a built-in Unicode str even if a future splitter returns a string subclass.
        sentence = unicodedata.normalize("NFC", str(sentence)).replace(chr(0), "")
        try:
            wav = model.generate(
                sentence,
                language_id="pt",
                audio_prompt_path=str(reference_cache) if index == 1 else None,
                exaggeration=exaggeration,
                cfg_weight=cfg_weight,
            )
        except TypeError as exc:
            if "TextInputSequence must be str" in str(exc):
                excerpt = sentence[:80].replace("\\n", " ")
                raise TypeError(
                    f"O tokenizador rejeitou a frase {index}/{len(sentences)} "
                    f"(tipo={type(sentence).__name__}, caracteres={len(sentence)}, início={excerpt!r})."
                ) from exc
            raise
        values = wav.squeeze().detach().cpu().numpy().astype(np.float32, copy=False)
        if values.size:
            audio_parts.append(values)
        if index < len(sentences):
            audio_parts.append(np.zeros(round(model.sr * sentence_gap), dtype=np.float32))
        emit({"event": "progress", "phase": "generating", "message": f"Frase {index} de {len(sentences)}…", "current": index, "total": len(sentences)})

    if not audio_parts:
        raise RuntimeError("O motor não gerou áudio para este texto.")
    audio = np.concatenate(audio_parts)

    # Small tempo adjustments use pitch-preserving time stretch. At neutral
    # settings the waveform is left untouched.
    stretch_rate = 1.0 + (pace - 0.5) * 0.28
    if abs(stretch_rate - 1.0) > 0.015:
        import librosa
        audio = librosa.effects.time_stretch(audio, rate=stretch_rate).astype(np.float32, copy=False)

    output_dir = data_dir() / "generated"
    output_dir.mkdir(parents=True, exist_ok=True)
    output_path = output_dir / f"VOZLAB-{uuid.uuid4().hex[:10]}.wav"
    sf.write(str(output_path), audio, model.sr, subtype="PCM_16")
    return {"ok": True, "path": str(output_path), "sampleRate": int(model.sr), "device": model_device}


def main() -> None:
    for line in sys.stdin:
        try:
            request = json.loads(line)
            request_id = request.get("id")
            command = request.get("command")
            if command == "status":
                result = installed_status()
            elif command == "selftest":
                result = engine_api_selftest()
            elif command == "generate":
                result = generate(request)
            elif command == "shutdown":
                emit({"id": request_id, "ok": True})
                break
            else:
                raise ValueError("Comando desconhecido para o motor de voz.")
            emit({"id": request_id, **result})
        except Exception as exc:
            emit({"id": request.get("id") if "request" in locals() else None, "ok": False, "error": str(exc)})
            traceback.print_exc(file=sys.stderr)


if __name__ == "__main__":
    main()
