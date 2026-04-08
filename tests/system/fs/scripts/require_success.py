#!/usr/bin/env python3
from __future__ import annotations

import argparse
import subprocess
import sys
from pathlib import Path


def parser() -> argparse.ArgumentParser:
    p = argparse.ArgumentParser(description="Run a command only if a prerequisite success marker exists.")
    p.add_argument("--required-marker", required=True)
    p.add_argument("--label", required=True)
    p.add_argument("command", nargs=argparse.REMAINDER)
    return p


def main() -> int:
    args = parser().parse_args()
    marker = Path(args.required_marker).resolve()
    command = list(args.command)
    if command and command[0] == "--":
        command = command[1:]
    if not command:
        raise SystemExit("missing command")

    if not marker.exists():
        print(f"[skip] {args.label}: prerequisite success marker missing: {marker}")
        return 125

    proc = subprocess.run(command, check=False)
    return int(proc.returncode)


if __name__ == "__main__":
    raise SystemExit(main())
