#!/usr/bin/env python3
from __future__ import annotations

import argparse
import shutil
import subprocess
from pathlib import Path


BASE_DIR_NAME = "base"
SUITE_DIR_NAME = "suite"
PERSIST_FILE_DIR_NAME = "persistence-file"
PERSIST_DIR_NAME = "persistence-directory"


def run(cmd: list[str], cwd: Path) -> None:
    subprocess.run(cmd, cwd=cwd, check=True, text=True)


def copy_pair(src_dir: Path, dst_dir: Path) -> None:
    dst_dir.mkdir(parents=True, exist_ok=True)
    shutil.copy2(src_dir / "hd64M.img", dst_dir / "hd64M.img")
    shutil.copy2(src_dir / "hd80M.img", dst_dir / "hd80M.img")


def write_region(image: Path, binary: Path, sector_start: int, sector_count: int) -> None:
    payload = binary.read_bytes()
    region_size = sector_count * 512
    if len(payload) > region_size:
        raise AssertionError(f"{binary} exceeds reserved region of {region_size} bytes")

    with image.open("r+b") as fp:
        fp.seek(sector_start * 512)
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
    persist_file_dir = image_root / PERSIST_FILE_DIR_NAME
    persist_dir_dir = image_root / PERSIST_DIR_NAME

    if not base_dir.exists():
        raise AssertionError(f"missing base image directory: {base_dir}")

    run(
        [
            args.cmake_command,
            "--build",
            str(build_dir),
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
    write_region(os_image, build_dir / "bin" / "kernel_stripped", sector_start=9, sector_count=380)
    write_region(os_image, build_dir / "bin" / "fs_test_runner", sector_start=1500, sector_count=200)

    for cloned_dir in (persist_file_dir, persist_dir_dir):
        if cloned_dir.exists():
            shutil.rmtree(cloned_dir)
        copy_pair(suite_dir, cloned_dir)

    print(f"[ok] seeded fs suite images under {image_root}")
    return 0


def parser() -> argparse.ArgumentParser:
    p = argparse.ArgumentParser(description="Prepare build-local filesystem test images.")
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
