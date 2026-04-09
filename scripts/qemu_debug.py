#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import os
import re
import shutil
import signal
import socket
import subprocess
import time
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parent.parent
DEFAULT_BUILD_DIR = REPO_ROOT / "cmake-build-debug"
DEFAULT_ARTIFACTS_DIR = REPO_ROOT / ".cache" / "qemu-smoke" / "latest"
DEFAULT_GDB_PORT = 1234
ROOT_PROMPT_PATTERN = re.compile(r"K@KKKZBH /\$ >")
PROMPT_PATTERN = re.compile(r"K@KKKZBH [^\n]+\$ >")
BOOT_MARKERS = ["BOOT:M1", "BOOT:L1", "BOOT:L2", "BOOT:K1", "BOOT:SH"]
SECTOR_SIZE = 512
DISK_MODE_MARKER = b"DISKTEST"
DISK_MODE_MARKER_LBA = 1799
DISK_CASE_OFFSET = 16
FS_RUNNER_TIMEOUT = 60.0
FATAL_PATTERNS = [
    re.compile(pattern, re.IGNORECASE)
    for pattern in (
        r"#PF",
        r"PAGE-FAULT",
        r"PANIC",
        r"ASSERT",
        r"GENERAL PROTECTION",
        r"DOUBLE FAULT",
        r"TRIPLE FAULT",
        r"EXCEPTION MESSAGE BEGIN",
    )
]


class QmpClient:
    def __init__(self, socket_path: Path) -> None:
        self.socket_path = socket_path
        self.sock: socket.socket | None = None
        self.reader = None
        self.writer = None

    def connect(self, timeout: float = 15.0) -> None:
        deadline = time.time() + timeout
        while time.time() < deadline:
            if self.socket_path.exists():
                try:
                    self.sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
                    self.sock.connect(str(self.socket_path))
                    self.reader = self.sock.makefile("r", encoding="utf-8")
                    self.writer = self.sock.makefile("w", encoding="utf-8")
                    greeting = self._read_message()
                    if "QMP" not in greeting:
                        raise RuntimeError(f"unexpected QMP greeting: {greeting}")
                    self.execute("qmp_capabilities")
                    return
                except OSError:
                    self.close()
            time.sleep(0.1)
        raise TimeoutError(f"timed out waiting for QMP socket: {self.socket_path}")

    def close(self) -> None:
        if self.reader is not None:
            self.reader.close()
        if self.writer is not None:
            self.writer.close()
        if self.sock is not None:
            self.sock.close()
        self.sock = None
        self.reader = None
        self.writer = None

    def _read_message(self) -> dict:
        assert self.reader is not None
        while True:
            line = self.reader.readline()
            if line == "":
                raise RuntimeError("QMP connection closed")
            msg = json.loads(line)
            if "event" in msg:
                continue
            return msg

    def execute(self, command: str, arguments: dict | None = None) -> dict:
        assert self.writer is not None
        payload: dict[str, object] = {"execute": command}
        if arguments:
            payload["arguments"] = arguments
        self.writer.write(json.dumps(payload))
        self.writer.write("\n")
        self.writer.flush()
        msg = self._read_message()
        if "error" in msg:
            raise RuntimeError(f"QMP {command} failed: {msg['error']}")
        return msg

    def hmp(self, command: str) -> str:
        msg = self.execute("human-monitor-command", {"command-line": command})
        return str(msg.get("return", ""))

    def screendump(self, filename: Path) -> None:
        self.execute("screendump", {"filename": str(filename)})

    def quit(self) -> None:
        try:
            self.execute("quit")
        except Exception:
            pass


def qemu_command(
    *,
    qemu_binary: str,
    display: str,
    qmp_socket: Path | None,
    monitor: str,
    gdb_port: int | None,
    paused: bool,
    snapshot: bool,
    os_image: Path,
    data_image: Path,
) -> list[str]:
    cmd = [
        qemu_binary,
        "-m",
        "32",
        "-boot",
        "c",
        "-name",
        "knix-debug",
        "-rtc",
        "base=localtime",
        "-no-reboot",
        "-no-shutdown",
        "-drive",
        f"file={os_image},if=ide,index=0,media=disk,format=raw",
        "-drive",
        f"file={data_image},if=ide,index=1,media=disk,format=raw",
    ]

    if snapshot:
        cmd.append("-snapshot")

    cmd.extend(["-display", "none" if display == "none" else display])

    if qmp_socket is not None:
        qmp_socket.parent.mkdir(parents=True, exist_ok=True)
        if qmp_socket.exists():
            qmp_socket.unlink()
        cmd.extend(["-qmp", f"unix:{qmp_socket},server=on,wait=off"])

    if monitor == "stdio":
        cmd.extend(["-monitor", "stdio"])
    elif monitor == "none":
        cmd.extend(["-monitor", "none"])

    if gdb_port is not None:
        cmd.extend(["-gdb", f"tcp::{gdb_port}"])

    if paused:
        cmd.append("-S")

    return cmd


def ensure_images_exist(os_image: Path, data_image: Path) -> None:
    for path in (os_image, data_image):
        if not path.exists():
            raise FileNotFoundError(f"missing disk image: {path}")


def clone_images_for_test(os_image: Path, data_image: Path, artifacts_dir: Path, clone_images: bool) -> tuple[Path, Path]:
    ensure_images_exist(os_image, data_image)
    if not clone_images:
        return os_image, data_image

    cloned_os = artifacts_dir / os_image.name
    cloned_data = artifacts_dir / data_image.name
    shutil.copy2(os_image, cloned_os)
    shutil.copy2(data_image, cloned_data)
    return cloned_os, cloned_data


def configure_disk_autorun(os_image: Path, case_name: str) -> None:
    payload = bytearray(SECTOR_SIZE)
    payload[: len(DISK_MODE_MARKER)] = DISK_MODE_MARKER
    case_bytes = case_name.encode("ascii")
    payload[DISK_CASE_OFFSET : DISK_CASE_OFFSET + len(case_bytes)] = case_bytes
    with os_image.open("r+b") as fp:
        fp.seek(DISK_MODE_MARKER_LBA * SECTOR_SIZE)
        fp.write(payload)


def send_text(qmp: QmpClient, text: str, delay: float = 0.08) -> None:
    keymap = {
        " ": "spc",
        "/": "slash",
        ".": "dot",
        "-": "minus",
        "_": "shift-minus",
        "\n": "ret",
    }
    for ch in text:
        if "a" <= ch <= "z" or "0" <= ch <= "9":
            key = ch
        else:
            key = keymap.get(ch)
        if key is None:
            raise ValueError(f"unsupported key for automation: {ch!r}")
        qmp.hmp(f"sendkey {key}")
        time.sleep(delay)


def preprocess_image(ppm_path: Path) -> Path:
    png_path = ppm_path.with_suffix(".png")
    cmd = [
        "magick",
        str(ppm_path),
        "-colorspace",
        "Gray",
        "-negate",
        "-scale",
        "400%",
        "-threshold",
        "55%",
        str(png_path),
    ]
    subprocess.run(cmd, check=True, text=True, capture_output=True)
    return png_path


def ocr_image(image_path: Path) -> str:
    proc = subprocess.run(
        ["tesseract", str(image_path), "stdout", "--psm", "6"],
        check=True,
        text=True,
        capture_output=True,
    )
    return proc.stdout


def decode_vga_text(vram: bytes) -> str:
    lines: list[str] = []
    for row in range(25):
        chars: list[str] = []
        for col in range(80):
            idx = (row * 80 + col) * 2
            ch = vram[idx]
            chars.append(" " if ch == 0 else chr(ch))
        lines.append("".join(chars).rstrip())
    return "\n".join(lines)


def capture_ocr(qmp: QmpClient, artifacts_dir: Path, stem: str) -> tuple[Path, str]:
    ppm_path = artifacts_dir / f"{stem}.ppm"
    qmp.screendump(ppm_path)
    png_path = preprocess_image(ppm_path)
    text = ocr_image(png_path)
    (artifacts_dir / f"{stem}.txt").write_text(text)
    return ppm_path, text


def capture_vga_text(qmp: QmpClient, artifacts_dir: Path, stem: str) -> tuple[Path, str]:
    bin_path = artifacts_dir / f"{stem}.vram.bin"
    try:
        qmp_path = bin_path.relative_to(REPO_ROOT)
    except ValueError:
        qmp_path = bin_path
    qmp.hmp(f"pmemsave 0xb8000 4000 {qmp_path}")
    text = decode_vga_text(bin_path.read_bytes())
    (artifacts_dir / f"{stem}.txt").write_text(text)
    return bin_path, text


def normalize_text(text: str) -> str:
    return " ".join(text.upper().split())


def check_fatal(text: str, artifact: Path) -> None:
    for pattern in FATAL_PATTERNS:
        if pattern.search(text):
            raise RuntimeError(f"fatal screen pattern {pattern.pattern!r} seen in {artifact}")


def capture_screen(
    *,
    qmp: QmpClient,
    artifacts_dir: Path,
    stem: str,
    capture: str,
) -> tuple[Path, str]:
    if capture == "vga":
        artifact_path, text = capture_vga_text(qmp, artifacts_dir, stem)
    else:
        artifact_path, text = capture_ocr(qmp, artifacts_dir, stem)
    check_fatal(text, artifact_path)
    return artifact_path, text


def wait_for_patterns(
    *,
    qmp: QmpClient,
    artifacts_dir: Path,
    label: str,
    patterns: list[re.Pattern[str]],
    timeout: float,
    interval: float = 1.0,
    capture: str = "vga",
) -> tuple[Path, str]:
    deadline = time.time() + timeout
    attempt = 0
    latest_artifact: Path | None = None
    latest_text = ""

    while time.time() < deadline:
        artifact_path, text = capture_screen(
            qmp=qmp,
            artifacts_dir=artifacts_dir,
            stem=f"{label}-{attempt:02d}",
            capture=capture,
        )
        latest_artifact = artifact_path
        latest_text = text
        normalized = normalize_text(text)
        if all(pattern.search(normalized) for pattern in patterns):
            print(f"[ok] {label}: {artifact_path}")
            return artifact_path, text
        time.sleep(interval)
        attempt += 1

    raise TimeoutError(
        f"timed out waiting for {label}; latest artifact={latest_artifact}, latest text={latest_text!r}"
    )


def capture_once(
    *,
    qmp: QmpClient,
    artifacts_dir: Path,
    label: str,
    capture: str = "vga",
) -> tuple[Path, str]:
    artifact_path, text = capture_screen(
        qmp=qmp,
        artifacts_dir=artifacts_dir,
        stem=label,
        capture=capture,
    )
    print(f"[ok] {label}: {artifact_path}")
    return artifact_path, text


def assert_marker_order(text: str, markers: list[str]) -> None:
    normalized = normalize_text(text)
    offset = -1
    for marker in markers:
        pos = normalized.find(marker)
        if pos == -1:
            raise RuntimeError(f"missing marker {marker!r} in screen: {normalized!r}")
        if pos < offset:
            raise RuntimeError(f"marker order mismatch for {markers!r} in screen: {normalized!r}")
        offset = pos


def build_smoke_patterns(path_fragment: str) -> list[re.Pattern[str]]:
    return [
        re.compile(r"KKKZBH"),
        re.compile(re.escape(path_fragment.upper())),
    ]


def literal_pattern(text: str) -> re.Pattern[str]:
    return re.compile(re.escape(text.upper()))


def prompt_for(path_fragment: str) -> re.Pattern[str]:
    return re.compile(re.escape(f"K@KKKZBH {path_fragment.upper()}$ >"))


def run_shell_command(
    qmp: QmpClient,
    artifacts_dir: Path,
    *,
    label: str,
    command: str,
    patterns: list[re.Pattern[str]] | None = None,
    timeout: float = 10.0,
) -> tuple[Path, str]:
    print(f"[info] sending: {command}")
    send_text(qmp, f"{command}\n")
    time.sleep(0.3)
    expected = list(patterns or [])
    expected.append(PROMPT_PATTERN)
    return wait_for_patterns(
        qmp=qmp,
        artifacts_dir=artifacts_dir,
        label=label,
        patterns=expected,
        timeout=timeout,
        interval=0.5,
    )


def run_runner_case(qmp: QmpClient, artifacts_dir: Path, case_name: str, timeout: float = FS_RUNNER_TIMEOUT) -> tuple[Path, str]:
    token = literal_pattern(f"FSCASE:{case_name}:PASS")
    return run_shell_command(
        qmp,
        artifacts_dir,
        label=f"runner-{case_name}",
        command=f"/bin/fs_test_runner {case_name}",
        patterns=[token],
        timeout=timeout,
    )


def scenario_boot_banner(qmp: QmpClient, artifacts_dir: Path, boot_timeout: float) -> None:
    print("[info] waiting for MBR banner")
    wait_for_patterns(
        qmp=qmp,
        artifacts_dir=artifacts_dir,
        label="boot-banner",
        patterns=[re.compile(r"KKK[Z2]BH"), re.compile(r"BOOT:M1")],
        timeout=boot_timeout,
    )


def scenario_boot_milestones(qmp: QmpClient, artifacts_dir: Path, boot_timeout: float) -> None:
    print("[info] waiting for boot milestones")
    _, text = wait_for_patterns(
        qmp=qmp,
        artifacts_dir=artifacts_dir,
        label="boot-milestones",
        patterns=[*(re.compile(re.escape(marker)) for marker in BOOT_MARKERS), ROOT_PROMPT_PATTERN],
        timeout=boot_timeout,
        interval=0.5,
    )
    assert_marker_order(text, BOOT_MARKERS)


def scenario_boot_prompt_clean(qmp: QmpClient, artifacts_dir: Path, boot_timeout: float) -> None:
    print("[info] waiting for shell prompt")
    _, text = wait_for_patterns(
        qmp=qmp,
        artifacts_dir=artifacts_dir,
        label="boot-prompt",
        patterns=[ROOT_PROMPT_PATTERN, re.compile(r"BOOT:SH")],
        timeout=boot_timeout,
        interval=0.5,
    )
    assert_marker_order(text, BOOT_MARKERS)


def scenario_legacy_shell_smoke(qmp: QmpClient, artifacts_dir: Path, boot_timeout: float) -> None:
    scenario_boot_prompt_clean(qmp, artifacts_dir, boot_timeout)

    print("[info] sending: ps")
    send_text(qmp, "ps\n")
    wait_for_patterns(
        qmp=qmp,
        artifacts_dir=artifacts_dir,
        label="ps",
        patterns=[re.compile(r"PID"), re.compile(r"COMMAND")],
        timeout=10,
    )

    print("[info] sending: mkdir /qq")
    send_text(qmp, "mkdir /qq\n")
    time.sleep(1.5)
    capture_once(qmp=qmp, artifacts_dir=artifacts_dir, label="post-mkdir")

    print("[info] sending: cd /qq")
    send_text(qmp, "cd /qq\n")
    wait_for_patterns(
        qmp=qmp,
        artifacts_dir=artifacts_dir,
        label="cd-qq",
        patterns=build_smoke_patterns("/QQ"),
        timeout=10,
    )

    print("[info] sending: pwd")
    send_text(qmp, "pwd\n")
    wait_for_patterns(
        qmp=qmp,
        artifacts_dir=artifacts_dir,
        label="pwd-qq",
        patterns=[re.compile(r"/QQ")],
        timeout=10,
    )


def scenario_shell_fs_cwd(qmp: QmpClient, artifacts_dir: Path, boot_timeout: float) -> None:
    scenario_boot_prompt_clean(qmp, artifacts_dir, boot_timeout)
    run_shell_command(qmp, artifacts_dir, label="fs-cwd-mkdir", command="mkdir /fscwd")
    run_shell_command(qmp, artifacts_dir, label="fs-cwd-cd-root", command="cd /fscwd", patterns=[prompt_for("/FSCWD")])
    run_shell_command(qmp, artifacts_dir, label="fs-cwd-mkdir-sub", command="mkdir .//sub", patterns=[prompt_for("/FSCWD")])
    run_shell_command(qmp, artifacts_dir, label="fs-cwd-cd-sub", command="cd .//sub", patterns=[prompt_for("/FSCWD/SUB")])
    run_shell_command(qmp, artifacts_dir, label="fs-cwd-pwd-sub", command="pwd", patterns=[literal_pattern("/fscwd/sub")])
    run_shell_command(qmp, artifacts_dir, label="fs-cwd-cd-parent", command="cd ..", patterns=[prompt_for("/FSCWD")])
    run_shell_command(qmp, artifacts_dir, label="fs-cwd-pwd-parent", command="pwd", patterns=[literal_pattern("/fscwd")])


def scenario_shell_fs_mkdir_rmdir(qmp: QmpClient, artifacts_dir: Path, boot_timeout: float) -> None:
    scenario_boot_prompt_clean(qmp, artifacts_dir, boot_timeout)
    run_shell_command(qmp, artifacts_dir, label="fs-mkdir-create", command="mkdir /fsmr")
    run_shell_command(qmp, artifacts_dir, label="fs-mkdir-list", command="ls /", patterns=[literal_pattern("fsmr")])
    run_shell_command(qmp, artifacts_dir, label="fs-mkdir-remove", command="rmdir /fsmr")
    run_shell_command(
        qmp,
        artifacts_dir,
        label="fs-mkdir-cd-removed",
        command="cd /fsmr",
        patterns=[literal_pattern("cd: no such directory /fsmr")],
    )


def scenario_shell_fs_ls(qmp: QmpClient, artifacts_dir: Path, boot_timeout: float) -> None:
    scenario_boot_prompt_clean(qmp, artifacts_dir, boot_timeout)
    run_shell_command(qmp, artifacts_dir, label="fs-ls-mkdir-root", command="mkdir /fsls")
    run_shell_command(qmp, artifacts_dir, label="fs-ls-mkdir-a", command="mkdir /fsls/alphaone")
    run_shell_command(qmp, artifacts_dir, label="fs-ls-mkdir-b", command="mkdir /fsls/betatwo")
    run_shell_command(
        qmp,
        artifacts_dir,
        label="fs-ls-list",
        command="ls /fsls",
        patterns=[literal_pattern("alphaone"), literal_pattern("betatwo")],
    )


def scenario_shell_fs_negative(qmp: QmpClient, artifacts_dir: Path, boot_timeout: float) -> None:
    scenario_boot_prompt_clean(qmp, artifacts_dir, boot_timeout)
    run_shell_command(qmp, artifacts_dir, label="fs-neg-mkdir-root", command="mkdir /fsneg")
    run_shell_command(
        qmp,
        artifacts_dir,
        label="fs-neg-mkdir-dup",
        command="mkdir /fsneg",
        patterns=[literal_pattern("mkdir: create directory /fsneg failed.")],
    )
    run_shell_command(qmp, artifacts_dir, label="fs-neg-mkdir-child", command="mkdir /fsneg/child")
    run_shell_command(
        qmp,
        artifacts_dir,
        label="fs-neg-rmdir-nonempty",
        command="rmdir /fsneg",
        patterns=[literal_pattern("rmdir: remove /fsneg failed.")],
    )
    run_shell_command(
        qmp,
        artifacts_dir,
        label="fs-neg-cd-missing",
        command="cd /fsneg/missing",
        patterns=[literal_pattern("cd: no such directory /fsneg/missing")],
    )


def scenario_shell_fs_regression_mkdir_rootqq_pf(qmp: QmpClient, artifacts_dir: Path, boot_timeout: float) -> None:
    scenario_boot_prompt_clean(qmp, artifacts_dir, boot_timeout)
    run_shell_command(qmp, artifacts_dir, label="fs-regression-mkdir-qq", command="mkdir /qq", timeout=10.0)


def scenario_runner_fs_basic(qmp: QmpClient, artifacts_dir: Path, boot_timeout: float) -> None:
    scenario_boot_prompt_clean(qmp, artifacts_dir, boot_timeout)
    run_runner_case(qmp, artifacts_dir, "file_basic")


def scenario_runner_fs_file_offset(qmp: QmpClient, artifacts_dir: Path, boot_timeout: float) -> None:
    scenario_boot_prompt_clean(qmp, artifacts_dir, boot_timeout)
    run_runner_case(qmp, artifacts_dir, "file_offset")


def scenario_runner_fs_directory_basic(qmp: QmpClient, artifacts_dir: Path, boot_timeout: float) -> None:
    scenario_boot_prompt_clean(qmp, artifacts_dir, boot_timeout)
    run_runner_case(qmp, artifacts_dir, "directory_basic")


def scenario_runner_fs_directory_iteration(qmp: QmpClient, artifacts_dir: Path, boot_timeout: float) -> None:
    scenario_boot_prompt_clean(qmp, artifacts_dir, boot_timeout)
    run_runner_case(qmp, artifacts_dir, "directory_iteration")


def scenario_runner_fs_metadata(qmp: QmpClient, artifacts_dir: Path, boot_timeout: float) -> None:
    scenario_boot_prompt_clean(qmp, artifacts_dir, boot_timeout)
    run_runner_case(qmp, artifacts_dir, "metadata")


def scenario_runner_fs_error_paths(qmp: QmpClient, artifacts_dir: Path, boot_timeout: float) -> None:
    scenario_boot_prompt_clean(qmp, artifacts_dir, boot_timeout)
    run_runner_case(qmp, artifacts_dir, "error_paths")


def scenario_runner_fs_persistence_write(qmp: QmpClient, artifacts_dir: Path, boot_timeout: float) -> None:
    scenario_boot_prompt_clean(qmp, artifacts_dir, boot_timeout)
    run_runner_case(qmp, artifacts_dir, "persistence_file_write")


def scenario_runner_fs_persistence_verify(qmp: QmpClient, artifacts_dir: Path, boot_timeout: float) -> None:
    scenario_boot_prompt_clean(qmp, artifacts_dir, boot_timeout)
    run_runner_case(qmp, artifacts_dir, "persistence_file_verify")


def scenario_runner_fs_persistence_dir_write(qmp: QmpClient, artifacts_dir: Path, boot_timeout: float) -> None:
    scenario_boot_prompt_clean(qmp, artifacts_dir, boot_timeout)
    run_runner_case(qmp, artifacts_dir, "persistence_directory_write")


def scenario_runner_fs_persistence_dir_verify(qmp: QmpClient, artifacts_dir: Path, boot_timeout: float) -> None:
    scenario_boot_prompt_clean(qmp, artifacts_dir, boot_timeout)
    run_runner_case(qmp, artifacts_dir, "persistence_directory_verify")


def scenario_disk_read_sector(qmp: QmpClient, artifacts_dir: Path, boot_timeout: float) -> None:
    wait_for_patterns(
        qmp=qmp,
        artifacts_dir=artifacts_dir,
        label="disk-read-sector",
        patterns=[literal_pattern("DISKCASE:read_sector:PASS")],
        timeout=boot_timeout,
        interval=0.5,
    )


def scenario_disk_cross_sector_read(qmp: QmpClient, artifacts_dir: Path, boot_timeout: float) -> None:
    wait_for_patterns(
        qmp=qmp,
        artifacts_dir=artifacts_dir,
        label="disk-cross-sector-read",
        patterns=[literal_pattern("DISKCASE:cross_sector_read:PASS")],
        timeout=boot_timeout,
        interval=0.5,
    )


def scenario_disk_read_after_write(qmp: QmpClient, artifacts_dir: Path, boot_timeout: float) -> None:
    wait_for_patterns(
        qmp=qmp,
        artifacts_dir=artifacts_dir,
        label="disk-read-after-write",
        patterns=[literal_pattern("DISKCASE:read_after_write:PASS")],
        timeout=boot_timeout,
        interval=0.5,
    )


def scenario_disk_partition_table_scan(qmp: QmpClient, artifacts_dir: Path, boot_timeout: float) -> None:
    wait_for_patterns(
        qmp=qmp,
        artifacts_dir=artifacts_dir,
        label="disk-partition-table-scan",
        patterns=[literal_pattern("DISKCASE:partition_table_scan:PASS")],
        timeout=boot_timeout,
        interval=0.5,
    )


def scenario_disk_multi_sector_read(qmp: QmpClient, artifacts_dir: Path, boot_timeout: float) -> None:
    wait_for_patterns(
        qmp=qmp,
        artifacts_dir=artifacts_dir,
        label="disk-multi-sector-read",
        patterns=[
            literal_pattern("DISKCASE:multi_sector_read:PASS"),
            literal_pattern("DISKSTAT:multi_sector_read:cmds=1"),
            literal_pattern("timeouts=0"),
            literal_pattern("reads=16"),
        ],
        timeout=boot_timeout,
        interval=0.5,
    )


def scenario_disk_max_transfer_boundary_read(qmp: QmpClient, artifacts_dir: Path, boot_timeout: float) -> None:
    wait_for_patterns(
        qmp=qmp,
        artifacts_dir=artifacts_dir,
        label="disk-max-transfer-boundary-read",
        patterns=[
            literal_pattern("DISKCASE:max_transfer_boundary_read:PASS"),
            literal_pattern("DISKSTAT:max_transfer_boundary_read:cmds=2"),
            literal_pattern("timeouts=0"),
            literal_pattern("reads=257"),
        ],
        timeout=boot_timeout,
        interval=0.5,
    )


def scenario_disk_multi_sector_write(qmp: QmpClient, artifacts_dir: Path, boot_timeout: float) -> None:
    wait_for_patterns(
        qmp=qmp,
        artifacts_dir=artifacts_dir,
        label="disk-multi-sector-write",
        patterns=[
            literal_pattern("DISKCASE:multi_sector_write:PASS"),
            literal_pattern("DISKSTAT:multi_sector_write:cmds=2"),
            literal_pattern("timeouts=0"),
            literal_pattern("reads=8"),
            literal_pattern("writes=8"),
        ],
        timeout=boot_timeout,
        interval=0.5,
    )


def run_scenario(
    *,
    scenario: str,
    qemu_binary: str,
    artifacts_dir: Path,
    boot_timeout: float,
    snapshot: bool,
    os_image: Path,
    data_image: Path,
    clone_images: bool,
) -> int:
    artifacts_dir.parent.mkdir(parents=True, exist_ok=True)
    if artifacts_dir.exists():
        shutil.rmtree(artifacts_dir)
    artifacts_dir.mkdir(parents=True, exist_ok=True)
    os_image, data_image = clone_images_for_test(os_image, data_image, artifacts_dir, clone_images)
    disk_case_name = {
        "disk_read_sector": "read_sector",
        "disk_cross_sector_read": "cross_sector_read",
        "disk_read_after_write": "read_after_write",
        "disk_partition_table_scan": "partition_table_scan",
        "disk_multi_sector_read": "multi_sector_read",
        "disk_max_transfer_boundary_read": "max_transfer_boundary_read",
        "disk_multi_sector_write": "multi_sector_write",
    }.get(scenario)
    if disk_case_name is not None:
        configure_disk_autorun(os_image, disk_case_name)

    qmp_socket = artifacts_dir / "qmp.sock"
    qemu_log = artifacts_dir / "qemu.log"
    cmd = qemu_command(
        qemu_binary=qemu_binary,
        display="none",
        qmp_socket=qmp_socket,
        monitor="none",
        gdb_port=None,
        paused=False,
        snapshot=snapshot,
        os_image=os_image,
        data_image=data_image,
    )
    qemu_log.write_text(" ".join(cmd) + "\n")
    print(f"[info] artifacts: {artifacts_dir}")
    print(f"[info] qemu: {' '.join(cmd)}")

    proc = subprocess.Popen(
        cmd,
        cwd=REPO_ROOT,
        stdout=subprocess.DEVNULL,
        stderr=subprocess.STDOUT,
        start_new_session=True,
    )

    qmp = QmpClient(qmp_socket)
    try:
        qmp.connect()
        if scenario == "boot_banner":
            scenario_boot_banner(qmp, artifacts_dir, boot_timeout)
        elif scenario == "boot_milestones":
            scenario_boot_milestones(qmp, artifacts_dir, boot_timeout)
        elif scenario == "boot_prompt_clean":
            scenario_boot_prompt_clean(qmp, artifacts_dir, boot_timeout)
        elif scenario == "legacy_shell_smoke":
            scenario_legacy_shell_smoke(qmp, artifacts_dir, boot_timeout)
        elif scenario == "shell_fs_cwd":
            scenario_shell_fs_cwd(qmp, artifacts_dir, boot_timeout)
        elif scenario == "shell_fs_mkdir_rmdir":
            scenario_shell_fs_mkdir_rmdir(qmp, artifacts_dir, boot_timeout)
        elif scenario == "shell_fs_ls":
            scenario_shell_fs_ls(qmp, artifacts_dir, boot_timeout)
        elif scenario == "shell_fs_negative":
            scenario_shell_fs_negative(qmp, artifacts_dir, boot_timeout)
        elif scenario == "shell_fs_regression_mkdir_rootqq_pf":
            scenario_shell_fs_regression_mkdir_rootqq_pf(qmp, artifacts_dir, boot_timeout)
        elif scenario == "runner_fs_basic":
            scenario_runner_fs_basic(qmp, artifacts_dir, boot_timeout)
        elif scenario == "runner_fs_file_offset":
            scenario_runner_fs_file_offset(qmp, artifacts_dir, boot_timeout)
        elif scenario == "runner_fs_directory_basic":
            scenario_runner_fs_directory_basic(qmp, artifacts_dir, boot_timeout)
        elif scenario == "runner_fs_directory_iteration":
            scenario_runner_fs_directory_iteration(qmp, artifacts_dir, boot_timeout)
        elif scenario == "runner_fs_metadata":
            scenario_runner_fs_metadata(qmp, artifacts_dir, boot_timeout)
        elif scenario == "runner_fs_error_paths":
            scenario_runner_fs_error_paths(qmp, artifacts_dir, boot_timeout)
        elif scenario == "runner_fs_persistence_write":
            scenario_runner_fs_persistence_write(qmp, artifacts_dir, boot_timeout)
        elif scenario == "runner_fs_persistence_verify":
            scenario_runner_fs_persistence_verify(qmp, artifacts_dir, boot_timeout)
        elif scenario == "runner_fs_persistence_dir_write":
            scenario_runner_fs_persistence_dir_write(qmp, artifacts_dir, boot_timeout)
        elif scenario == "runner_fs_persistence_dir_verify":
            scenario_runner_fs_persistence_dir_verify(qmp, artifacts_dir, boot_timeout)
        elif scenario == "disk_read_sector":
            scenario_disk_read_sector(qmp, artifacts_dir, boot_timeout)
        elif scenario == "disk_cross_sector_read":
            scenario_disk_cross_sector_read(qmp, artifacts_dir, boot_timeout)
        elif scenario == "disk_read_after_write":
            scenario_disk_read_after_write(qmp, artifacts_dir, boot_timeout)
        elif scenario == "disk_partition_table_scan":
            scenario_disk_partition_table_scan(qmp, artifacts_dir, boot_timeout)
        elif scenario == "disk_multi_sector_read":
            scenario_disk_multi_sector_read(qmp, artifacts_dir, boot_timeout)
        elif scenario == "disk_max_transfer_boundary_read":
            scenario_disk_max_transfer_boundary_read(qmp, artifacts_dir, boot_timeout)
        elif scenario == "disk_multi_sector_write":
            scenario_disk_multi_sector_write(qmp, artifacts_dir, boot_timeout)
        else:
            raise ValueError(f"unknown scenario: {scenario}")
        (artifacts_dir / "scenario.pass").write_text(f"{scenario}\n", encoding="utf-8")
        print(f"[ok] scenario passed: {scenario}")
        return 0
    finally:
        qmp.quit()
        qmp.close()
        try:
            proc.wait(timeout=5)
        except subprocess.TimeoutExpired:
            os.killpg(proc.pid, signal.SIGTERM)
            try:
                proc.wait(timeout=5)
            except subprocess.TimeoutExpired:
                os.killpg(proc.pid, signal.SIGKILL)
                proc.wait(timeout=5)
        if qmp_socket.exists():
            qmp_socket.unlink()


def command_run(args: argparse.Namespace) -> int:
    os_image = Path(args.os_image).resolve() if args.os_image else REPO_ROOT / "hd64M.img"
    data_image = Path(args.data_image).resolve() if args.data_image else REPO_ROOT / "hd80M.img"
    ensure_images_exist(os_image, data_image)
    cmd = qemu_command(
        qemu_binary=args.qemu_binary,
        display=args.display,
        qmp_socket=Path(args.qmp_socket) if args.qmp_socket else None,
        monitor=args.monitor,
        gdb_port=args.gdb,
        paused=args.paused,
        snapshot=args.snapshot,
        os_image=os_image,
        data_image=data_image,
    )
    os.execvp(cmd[0], cmd)
    return 0


def command_smoke(args: argparse.Namespace) -> int:
    return run_scenario(
        scenario="legacy_shell_smoke",
        qemu_binary=args.qemu_binary,
        artifacts_dir=Path(args.artifacts_dir),
        boot_timeout=args.boot_timeout,
        snapshot=args.snapshot,
        os_image=Path(args.os_image).resolve() if args.os_image else REPO_ROOT / "hd64M.img",
        data_image=Path(args.data_image).resolve() if args.data_image else REPO_ROOT / "hd80M.img",
        clone_images=args.clone_images,
    )


def command_test(args: argparse.Namespace) -> int:
    return run_scenario(
        scenario=args.scenario,
        qemu_binary=args.qemu_binary,
        artifacts_dir=Path(args.artifacts_dir),
        boot_timeout=args.boot_timeout,
        snapshot=args.snapshot,
        os_image=Path(args.os_image).resolve() if args.os_image else REPO_ROOT / "hd64M.img",
        data_image=Path(args.data_image).resolve() if args.data_image else REPO_ROOT / "hd80M.img",
        clone_images=args.clone_images,
    )


def parser() -> argparse.ArgumentParser:
    p = argparse.ArgumentParser(description="Run and debug knix under QEMU.")
    sub = p.add_subparsers(dest="cmd", required=True)

    run_p = sub.add_parser("run", help="Run the OS under QEMU.")
    run_p.add_argument("--build-dir", default=str(DEFAULT_BUILD_DIR))
    run_p.add_argument("--qemu-binary", default=shutil.which("qemu-system-i386") or "qemu-system-i386")
    run_p.add_argument("--display", default="gtk", choices=["gtk", "sdl", "curses", "none"])
    run_p.add_argument("--monitor", default="none", choices=["none", "stdio"])
    run_p.add_argument("--qmp-socket", default="")
    run_p.add_argument("--gdb", type=int, default=None, nargs="?")
    run_p.add_argument("--paused", action="store_true")
    run_p.add_argument("--snapshot", action="store_true")
    run_p.add_argument("--os-image", default="")
    run_p.add_argument("--data-image", default="")
    run_p.set_defaults(func=command_run)

    smoke_p = sub.add_parser("smoke", help="Run the legacy headless shell smoke test.")
    smoke_p.add_argument("--build-dir", default=str(DEFAULT_BUILD_DIR))
    smoke_p.add_argument("--qemu-binary", default=shutil.which("qemu-system-i386") or "qemu-system-i386")
    smoke_p.add_argument("--artifacts-dir", default=str(DEFAULT_ARTIFACTS_DIR))
    smoke_p.add_argument("--boot-timeout", type=float, default=40.0)
    smoke_p.add_argument("--snapshot", dest="snapshot", action="store_true")
    smoke_p.add_argument("--no-snapshot", dest="snapshot", action="store_false")
    smoke_p.add_argument("--os-image", default="")
    smoke_p.add_argument("--data-image", default="")
    smoke_p.add_argument("--clone-images", action="store_true")
    smoke_p.set_defaults(snapshot=True)
    smoke_p.set_defaults(func=command_smoke)

    test_p = sub.add_parser("test", help="Run a boot-oriented QEMU test scenario.")
    test_p.add_argument(
        "--scenario",
        required=True,
        choices=[
            "boot_banner",
            "boot_milestones",
            "boot_prompt_clean",
            "legacy_shell_smoke",
            "shell_fs_cwd",
            "shell_fs_mkdir_rmdir",
            "shell_fs_ls",
            "shell_fs_negative",
            "shell_fs_regression_mkdir_rootqq_pf",
            "runner_fs_basic",
            "runner_fs_file_offset",
            "runner_fs_directory_basic",
            "runner_fs_directory_iteration",
            "runner_fs_metadata",
            "runner_fs_error_paths",
            "runner_fs_persistence_write",
            "runner_fs_persistence_verify",
            "runner_fs_persistence_dir_write",
            "runner_fs_persistence_dir_verify",
            "disk_read_sector",
            "disk_cross_sector_read",
            "disk_read_after_write",
            "disk_partition_table_scan",
            "disk_multi_sector_read",
            "disk_max_transfer_boundary_read",
            "disk_multi_sector_write",
        ],
    )
    test_p.add_argument("--build-dir", default=str(DEFAULT_BUILD_DIR))
    test_p.add_argument("--qemu-binary", default=shutil.which("qemu-system-i386") or "qemu-system-i386")
    test_p.add_argument("--artifacts-dir", default=str(DEFAULT_ARTIFACTS_DIR))
    test_p.add_argument("--boot-timeout", type=float, default=40.0)
    test_p.add_argument("--os-image", default="")
    test_p.add_argument("--data-image", default="")
    test_p.add_argument("--snapshot", dest="snapshot", action="store_true")
    test_p.add_argument("--no-snapshot", dest="snapshot", action="store_false")
    test_p.add_argument("--clone-images", action="store_true")
    test_p.set_defaults(snapshot=True)
    test_p.set_defaults(func=command_test)
    return p


def main() -> int:
    args = parser().parse_args()
    return int(args.func(args))


if __name__ == "__main__":
    raise SystemExit(main())
