#!/usr/bin/env python3
from __future__ import annotations

import argparse
import shutil
import subprocess
from pathlib import Path


BASE_DIR_NAME = "base"
SUITE_DIR_NAME = "suite"
SECTOR_SIZE = 512
MODE_MARKER_LBA = 1799
READ_SECTOR_LBA = 1800
CROSS_SECTOR_LBA = 1801
SCRATCH_SECTOR_LBA = 1803
MULTI_SECTOR_READ_LBA = 1804
MULTI_SECTOR_READ_COUNT = 16
MAX_TRANSFER_BOUNDARY_LBA = MULTI_SECTOR_READ_LBA + MULTI_SECTOR_READ_COUNT
MAX_TRANSFER_BOUNDARY_COUNT = 257
MULTI_SECTOR_WRITE_LBA = MAX_TRANSFER_BOUNDARY_LBA + MAX_TRANSFER_BOUNDARY_COUNT
MULTI_SECTOR_WRITE_COUNT = 8
SEED_SALT = 0x11
DISK_MODE_MARKER = b"DISKTEST"


def run(cmd: list[str], cwd: Path) -> None:
    subprocess.run(cmd, cwd=cwd, check=True, text=True)


def copy_pair(src_dir: Path, dst_dir: Path) -> None:
    dst_dir.mkdir(parents=True, exist_ok=True)
    shutil.copy2(src_dir / "hd64M.img", dst_dir / "hd64M.img")
    shutil.copy2(src_dir / "hd80M.img", dst_dir / "hd80M.img")


def write_region(image: Path, binary: Path, sector_start: int, sector_count: int) -> None:
    payload = binary.read_bytes()
    region_size = sector_count * SECTOR_SIZE
    if len(payload) > region_size:
        raise AssertionError(f"{binary} exceeds reserved region of {region_size} bytes")

    with image.open("r+b") as fp:
        fp.seek(sector_start * SECTOR_SIZE)
        fp.write(payload)


def pattern_byte(lba: int, byte_off: int, salt: int) -> int:
    return (salt + ((lba * 17) & 0xFF) + byte_off) & 0xFF


def fill_pattern(image: Path, start_lba: int, sector_count: int, salt: int) -> None:
    payload = bytearray(sector_count * SECTOR_SIZE)
    for sec in range(sector_count):
        lba = start_lba + sec
        base = sec * SECTOR_SIZE
        for byte_off in range(SECTOR_SIZE):
            payload[base + byte_off] = pattern_byte(lba, byte_off, salt)

    with image.open("r+b") as fp:
        fp.seek(start_lba * SECTOR_SIZE)
        fp.write(payload)


def write_marker(image: Path, lba: int, marker: bytes) -> None:
    payload = bytearray(SECTOR_SIZE)
    payload[: len(marker)] = marker
    with image.open("r+b") as fp:
        fp.seek(lba * SECTOR_SIZE)
        fp.write(payload)


def command_copy(args: argparse.Namespace) -> int:
    source_dir = Path(args.source_dir).resolve()
    image_root = Path(args.image_root).resolve()
    base_dir = image_root / BASE_DIR_NAME

    if image_root.exists():
        shutil.rmtree(image_root)
    base_dir.mkdir(parents=True, exist_ok=True)
    copy_pair(source_dir, base_dir)
    print(f"[ok] copied base images to {base_dir}")
    return 0


def command_seed(args: argparse.Namespace) -> int:
    build_dir = Path(args.build_dir).resolve()
    source_dir = Path(args.source_dir).resolve()
    image_root = Path(args.image_root).resolve()
    base_dir = image_root / BASE_DIR_NAME
    suite_dir = image_root / SUITE_DIR_NAME

    if not base_dir.exists():
        raise AssertionError(f"missing base image directory: {base_dir}")

    run(
        [
            args.cmake_command,
            "--build",
            str(build_dir),
            "--parallel",
            "1",
            "--target",
            "mbr_bin",
            "loader_bin",
            "kernel_stripped_bin",
            "fs_test_runner",
        ],
        cwd=source_dir,
    )

    if suite_dir.exists():
        shutil.rmtree(suite_dir)
    copy_pair(base_dir, suite_dir)

    os_image = suite_dir / "hd64M.img"
    write_region(os_image, build_dir / "boot" / "mbr.bin", sector_start=0, sector_count=1)
    write_region(os_image, build_dir / "boot" / "loader.bin", sector_start=2, sector_count=4)
    write_region(os_image, build_dir / "bin" / "kernel_stripped", sector_start=9, sector_count=455)
    write_region(os_image, build_dir / "bin" / "fs_test_runner", sector_start=1500, sector_count=200)

    write_marker(os_image, MODE_MARKER_LBA, DISK_MODE_MARKER)
    fill_pattern(os_image, READ_SECTOR_LBA, 1, SEED_SALT)
    fill_pattern(os_image, CROSS_SECTOR_LBA, 2, SEED_SALT)
    fill_pattern(os_image, SCRATCH_SECTOR_LBA, 1, SEED_SALT)
    fill_pattern(os_image, MULTI_SECTOR_READ_LBA, MULTI_SECTOR_READ_COUNT, SEED_SALT)
    fill_pattern(os_image, MAX_TRANSFER_BOUNDARY_LBA, MAX_TRANSFER_BOUNDARY_COUNT, SEED_SALT)
    fill_pattern(os_image, MULTI_SECTOR_WRITE_LBA, MULTI_SECTOR_WRITE_COUNT, SEED_SALT)

    print(f"[ok] seeded disk suite images under {image_root}")
    return 0


def parser() -> argparse.ArgumentParser:
    p = argparse.ArgumentParser(description="Prepare build-local IDE disk regression images.")
    sub = p.add_subparsers(dest="cmd", required=True)

    copy_p = sub.add_parser("copy")
    copy_p.add_argument("--source-dir", required=True)
    copy_p.add_argument("--image-root", required=True)
    copy_p.set_defaults(func=command_copy)

    seed_p = sub.add_parser("seed")
    seed_p.add_argument("--cmake-command", required=True)
    seed_p.add_argument("--build-dir", required=True)
    seed_p.add_argument("--source-dir", required=True)
    seed_p.add_argument("--image-root", required=True)
    seed_p.set_defaults(func=command_seed)
    return p


def main() -> int:
    args = parser().parse_args()
    return int(args.func(args))


if __name__ == "__main__":
    raise SystemExit(main())
