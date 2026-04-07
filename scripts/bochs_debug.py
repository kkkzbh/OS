#!/usr/bin/env python3
from __future__ import annotations

import argparse
import os
import re
import shutil
import signal
import subprocess
import time
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parent.parent
DEFAULT_ARTIFACTS_DIR = REPO_ROOT / ".cache" / "bochs-smoke" / "latest"


def run(cmd: list[str], *, check: bool = True) -> subprocess.CompletedProcess[str]:
    return subprocess.run(cmd, check=check, text=True, capture_output=True)


def find_bochs_window(timeout: float = 15.0) -> str:
    deadline = time.time() + timeout
    while time.time() < deadline:
        proc = run(["xdotool", "search", "--name", "Bochs"], check=False)
        ids = [line.strip() for line in proc.stdout.splitlines() if line.strip()]
        if ids:
            return ids[0]
        time.sleep(0.2)
    raise TimeoutError("timed out waiting for Bochs window")


def capture_window(window_id: str, artifacts_dir: Path, stem: str) -> tuple[Path, Path, str]:
    raw_path = artifacts_dir / f"{stem}.png"
    ocr_path = artifacts_dir / f"{stem}.ocr.png"
    txt_path = artifacts_dir / f"{stem}.txt"

    run(["import", "-window", window_id, str(raw_path)])
    run(
        [
            "magick",
            str(raw_path),
            "-crop",
            "720x420+0+28",
            "-colorspace",
            "Gray",
            "-negate",
            "-scale",
            "300%",
            "-threshold",
            "55%",
            str(ocr_path),
        ]
    )
    text = run(["tesseract", str(ocr_path), "stdout", "--psm", "6"]).stdout
    txt_path.write_text(text)
    return raw_path, ocr_path, text


def normalize_text(text: str) -> str:
    return " ".join(text.upper().split())


def wait_for_patterns(
    *,
    window_id: str,
    artifacts_dir: Path,
    label: str,
    patterns: list[re.Pattern[str]],
    timeout: float,
    interval: float = 1.0,
) -> tuple[Path, str]:
    deadline = time.time() + timeout
    attempt = 0
    latest_path: Path | None = None
    latest_text = ""
    while time.time() < deadline:
        raw_path, _, text = capture_window(window_id, artifacts_dir, f"{label}-{attempt:02d}")
        latest_path = raw_path
        latest_text = text
        normalized = normalize_text(text)
        if all(pattern.search(normalized) for pattern in patterns):
            print(f"[ok] {label}: {raw_path}")
            return raw_path, text
        time.sleep(interval)
        attempt += 1
    raise TimeoutError(
        f"timed out waiting for {label}; latest screenshot={latest_path}, latest OCR={latest_text!r}"
    )


def focus_guest(window_id: str) -> None:
    run(["xdotool", "windowactivate", window_id])
    time.sleep(0.3)
    run(["xdotool", "mousemove", "--window", window_id, "120", "120"])
    run(["xdotool", "click", "--window", window_id, "1"])
    time.sleep(0.3)


def type_command(window_id: str, command: str) -> None:
    focus_guest(window_id)
    run(["xdotool", "type", "--window", window_id, command])
    run(["xdotool", "key", "--window", window_id, "Return"])


def smoke(artifacts_dir: Path, bochs_binary: str, bochs_config: str) -> int:
    artifacts_dir.parent.mkdir(parents=True, exist_ok=True)
    if artifacts_dir.exists():
        shutil.rmtree(artifacts_dir)
    artifacts_dir.mkdir(parents=True, exist_ok=True)

    cmd = [bochs_binary, "-qf", bochs_config]
    print(f"[info] artifacts: {artifacts_dir}")
    print(f"[info] bochs: {' '.join(cmd)}")
    proc = subprocess.Popen(
        cmd,
        cwd=REPO_ROOT,
        stdout=subprocess.DEVNULL,
        stderr=subprocess.STDOUT,
        start_new_session=True,
    )

    try:
        window_id = find_bochs_window()
        print(f"[info] bochs window: {window_id}")

        wait_for_patterns(
            window_id=window_id,
            artifacts_dir=artifacts_dir,
            label="prompt",
            patterns=[re.compile(r"KKKZBH"), re.compile(r"/")],
            timeout=20,
        )

        print("[info] sending: ps")
        type_command(window_id, "ps")
        wait_for_patterns(
            window_id=window_id,
            artifacts_dir=artifacts_dir,
            label="ps",
            patterns=[re.compile(r"PID"), re.compile(r"COMMAND")],
            timeout=10,
        )

        print("[info] sending: pwd")
        type_command(window_id, "pwd")
        wait_for_patterns(
            window_id=window_id,
            artifacts_dir=artifacts_dir,
            label="pwd",
            patterns=[re.compile(r"PWD"), re.compile(r"KKKZBH")],
            timeout=10,
        )

        print("[ok] bochs shell smoke passed")
        return 0
    finally:
        os.killpg(proc.pid, signal.SIGTERM)
        try:
            proc.wait(timeout=5)
        except subprocess.TimeoutExpired:
            os.killpg(proc.pid, signal.SIGKILL)
            proc.wait(timeout=5)


def parser() -> argparse.ArgumentParser:
    p = argparse.ArgumentParser(description="Drive and smoke-test knix under Bochs.")
    p.add_argument("--artifacts-dir", default=str(DEFAULT_ARTIFACTS_DIR))
    p.add_argument("--bochs-binary", default=shutil.which("bochs") or "/usr/local/bin/bochs")
    p.add_argument("--bochs-config", default="bochsrc.disk")
    return p


def main() -> int:
    args = parser().parse_args()
    return smoke(Path(args.artifacts_dir), args.bochs_binary, args.bochs_config)


if __name__ == "__main__":
    raise SystemExit(main())
