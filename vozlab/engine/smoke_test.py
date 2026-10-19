"""Verify that the frozen Windows engine imports its voice stack and answers IPC."""
from __future__ import annotations

import json
import subprocess
import sys


def main() -> int:
    executable = sys.argv[1]
    requests = (
        {"id": "smoke-status", "command": "status"},
        {"id": "smoke-shutdown", "command": "shutdown"},
    )
    process = subprocess.Popen(
        [executable], stdin=subprocess.PIPE, stdout=subprocess.PIPE,
        stderr=subprocess.PIPE, text=True, encoding="utf-8",
    )
    try:
        stdout, stderr = process.communicate(
            "".join(json.dumps(item) + "\n" for item in requests), timeout=180
        )
    except subprocess.TimeoutExpired:
        process.kill()
        stdout, stderr = process.communicate()
        raise RuntimeError(f"Engine startup timed out. stdout={stdout!r}; stderr={stderr!r}")

    messages = []
    for line in stdout.splitlines():
        try:
            messages.append(json.loads(line))
        except json.JSONDecodeError:
            continue
    status = next((item for item in messages if item.get("id") == "smoke-status"), None)
    if process.returncode != 0 or not status or not status.get("engineInstalled"):
        detail = status.get("error") if status else "No status response from bundled worker."
        raise RuntimeError(
            f"Frozen engine smoke test failed: {detail}; exit={process.returncode}; "
            f"stdout={stdout!r}; stderr={stderr!r}"
        )
    print(f"Frozen engine responded successfully: {json.dumps(status, ensure_ascii=False)}")
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as error:
        print(str(error), file=sys.stderr)
        raise SystemExit(1)
