#!/usr/bin/env python3
from __future__ import annotations

import argparse
import subprocess
from pathlib import Path


def run(cmd: list[str], cwd: Path) -> None:
    subprocess.run(cmd, cwd=cwd, check=True, text=True)


def verify_magic(mbr: Path) -> None:
    data = mbr.read_bytes()
    if len(data) != 512:
        raise AssertionError(f"{mbr} must be exactly 512 bytes, got {len(data)}")
    if data[510:512] != b"\x55\xaa":
        raise AssertionError(f"{mbr} missing boot signature 0x55AA")


def verify_region(image: Path, binary: Path, sector_start: int, sector_count: int) -> None:
    expected = binary.read_bytes()
    region_offset = sector_start * 512
    region_size = sector_count * 512

    if len(expected) > region_size:
        raise AssertionError(
            f"{binary} is {len(expected)} bytes, exceeds reserved region {region_size} bytes"
        )

    with image.open("rb") as fp:
        fp.seek(region_offset)
        actual = fp.read(len(expected))

    if actual != expected:
        raise AssertionError(f"{binary} does not match {image} at sector {sector_start}")


def main() -> int:
    parser = argparse.ArgumentParser(description="Validate boot image layout for knix.")
    parser.add_argument("--cmake-command", required=True)
    parser.add_argument("--build-dir", required=True)
    parser.add_argument("--source-dir", required=True)
    args = parser.parse_args()

    build_dir = Path(args.build_dir).resolve()
    source_dir = Path(args.source_dir).resolve()
    image = source_dir / "hd64M.img"
    mbr = build_dir / "boot" / "mbr.bin"
    loader = build_dir / "boot" / "loader.bin"
    kernel = build_dir / "bin" / "kernel_stripped"

    run([args.cmake_command, "--build", str(build_dir), "--target", "osbuild"], cwd=source_dir)

    if not image.exists():
        raise AssertionError(f"missing disk image: {image}")
    for path in (mbr, loader, kernel):
        if not path.exists():
            raise AssertionError(f"missing build artifact: {path}")

    verify_magic(mbr)

    if loader.stat().st_size > 4 * 512:
        raise AssertionError(f"{loader} exceeds 4 sectors")
    if kernel.stat().st_size > 380 * 512:
        raise AssertionError(f"{kernel} exceeds 380 sectors")

    verify_region(image, mbr, sector_start=0, sector_count=1)
    verify_region(image, loader, sector_start=2, sector_count=4)
    verify_region(image, kernel, sector_start=9, sector_count=380)

    print(f"[ok] verified boot image layout in {image}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
