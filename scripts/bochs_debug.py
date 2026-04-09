#!/usr/bin/env python3
from __future__ import annotations

import argparse
import os
import re
import select
import shutil
import signal
import subprocess
import sys
import time
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parent.parent
DEFAULT_ARTIFACTS_DIR = REPO_ROOT / ".cache" / "bochs-smoke" / "latest"
VGA_TEXT_BASE = 0xC00B8000
VGA_VISIBLE_ROWS = 24
VGA_MARKER_ROWS = 1
VGA_COLS = 80
VGA_ROW_BYTES = VGA_COLS * 2
SECTOR_SIZE = 512
DISK_MODE_MARKER = b"DISKTEST"
DISK_MODE_MARKER_LBA = 1799
DISK_CASE_OFFSET = 16
ROOT_PROMPT_PATTERN = re.compile(r"K@KKKZBH /\$ >")
PROMPT_PATTERN = re.compile(r"K@KKKZBH [^\n]+\$ >")
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


def run(cmd: list[str], *, check: bool = True) -> subprocess.CompletedProcess[str]:
    return subprocess.run(cmd, check=check, text=True, capture_output=True)


def normalize_text(text: str) -> str:
    return " ".join(text.upper().split())


def literal_pattern(text: str) -> re.Pattern[str]:
    return re.compile(re.escape(text.upper()))


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


def render_bochs_config(base_config: Path, output_config: Path, os_image: Path, data_image: Path, artifacts_dir: Path) -> None:
    lines: list[str] = []
    for line in base_config.read_text(encoding="utf-8").splitlines():
        if line.startswith("log:"):
            lines.append(f"log: {artifacts_dir / 'bochs.log'}")
        elif line.startswith("debugger_log:"):
            lines.append(f"debugger_log: {artifacts_dir / 'bochs_debugger.log'}")
        elif line.startswith("com1:"):
            lines.append(f'com1: enabled=1, mode=file, dev="{artifacts_dir / "serial.txt"}"')
        elif line.startswith("ata0-master:"):
            lines.append(
                f'ata0-master: type=disk, path="{os_image}", mode=flat, cylinders=130, heads=16, spt=63'
            )
        elif line.startswith("ata0-slave:"):
            lines.append(
                f'ata0-slave: type=disk, path="{data_image}", mode=flat, cylinders=162, heads=16, spt=63'
            )
        else:
            lines.append(line)
    output_config.write_text("\n".join(lines) + "\n", encoding="utf-8")


def find_bochs_window(timeout: float = 15.0) -> str:
    deadline = time.time() + timeout
    while time.time() < deadline:
        proc = run(["xdotool", "search", "--name", "Bochs"], check=False)
        ids = [line.strip() for line in proc.stdout.splitlines() if line.strip()]
        if ids:
            return ids[0]
        time.sleep(0.2)
    raise TimeoutError("timed out waiting for Bochs window")


def capture_window(window_id: str, artifacts_dir: Path, stem: str) -> Path:
    raw_path = artifacts_dir / f"{stem}.png"
    run(["import", "-window", window_id, str(raw_path)])
    return raw_path


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


def decode_vga_rows(data: bytes, rows: int) -> list[str]:
    expected = rows * VGA_ROW_BYTES
    if len(data) != expected:
        raise ValueError(f"unexpected vga dump length: got={len(data)} expected={expected}")
    lines: list[str] = []
    for row in range(rows):
        line = []
        base = row * VGA_ROW_BYTES
        for col in range(VGA_COLS):
            ch = data[base + col * 2]
            if 32 <= ch <= 126:
                line.append(chr(ch))
            else:
                line.append(" ")
        lines.append("".join(line).rstrip())
    return lines


class GdbMiSession:
    def __init__(self, kernel_path: Path) -> None:
        self.proc = subprocess.Popen(
            ["gdb", "-q", "--interpreter=mi2", str(kernel_path)],
            cwd=REPO_ROOT,
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            bufsize=1,
            start_new_session=True,
        )
        self._drain_to_prompt(timeout=10.0)

    def close(self) -> None:
        if self.proc.poll() is not None:
            return
        try:
            self._command("-gdb-exit", timeout=3.0)
        except Exception:
            pass
        try:
            self.proc.terminate()
            self.proc.wait(timeout=3)
        except Exception:
            self.proc.kill()
            self.proc.wait(timeout=3)

    def connect(self, port: int) -> None:
        self._command("-gdb-set pagination off")
        result = self._command(f"-target-select remote :{port}", timeout=10.0)
        if not any(line.startswith("^connected") or line.startswith("^done") for line in result):
            raise RuntimeError(f"failed to connect gdb to tcp::{port}: {result}")

    def continue_run(self) -> None:
        result = self._command("-exec-continue")
        if not any(line.startswith("^running") for line in result):
            raise RuntimeError(f"continue did not start target: {result}")

    def interrupt(self) -> list[str]:
        os.kill(self.proc.pid, signal.SIGINT)
        result = self._drain_to_prompt(timeout=10.0)
        if not any("*stopped" in line for line in result):
            raise RuntimeError(f"interrupt did not stop target: {result}")
        return result

    def read_bytes(self, address: int, count: int) -> bytes:
        result = self._command(f"-data-read-memory-bytes 0x{address:x} {count}")
        contents = None
        for line in result:
            match = re.search(r'contents="([0-9a-fA-F]+)"', line)
            if match:
                contents = match.group(1)
                break
        if contents is None:
            raise RuntimeError(f"failed to read memory at 0x{address:x}: {result}")
        return bytes.fromhex(contents)

    def _command(self, command: str, timeout: float = 5.0) -> list[str]:
        assert self.proc.stdin is not None
        self.proc.stdin.write(command + "\n")
        self.proc.stdin.flush()
        return self._drain_to_prompt(timeout=timeout)

    def _drain_to_prompt(self, timeout: float) -> list[str]:
        assert self.proc.stdout is not None
        fd = self.proc.stdout.fileno()
        deadline = time.time() + timeout
        chunks: list[str] = []
        while time.time() < deadline:
            remaining = max(0.1, deadline - time.time())
            ready, _, _ = select.select([fd], [], [], remaining)
            if not ready:
                continue
            chunk = os.read(fd, 4096).decode(errors="replace")
            if not chunk:
                raise RuntimeError("gdb terminated unexpectedly")
            chunks.append(chunk)
            if "(gdb)" in "".join(chunks):
                break
        else:
            raise TimeoutError("timed out waiting for gdb prompt")
        text = "".join(chunks)
        return [line for line in text.splitlines() if line and line != "(gdb) " and line != "(gdb)"]


def connect_gdb(kernel_path: Path, port: int, timeout: float = 15.0) -> GdbMiSession:
    deadline = time.time() + timeout
    last_error: Exception | None = None
    while time.time() < deadline:
        session = GdbMiSession(kernel_path)
        try:
            session.connect(port)
            return session
        except Exception as exc:
            last_error = exc
            session.close()
            time.sleep(0.5)
    raise TimeoutError(f"timed out connecting gdb to tcp::{port}: {last_error}")


def capture_screen(gdb: GdbMiSession, artifacts_dir: Path, stem: str, window_id: str) -> tuple[Path, str]:
    visible = gdb.read_bytes(VGA_TEXT_BASE, VGA_VISIBLE_ROWS * VGA_ROW_BYTES)
    marker = gdb.read_bytes(
        VGA_TEXT_BASE + VGA_VISIBLE_ROWS * VGA_ROW_BYTES,
        VGA_MARKER_ROWS * VGA_ROW_BYTES,
    )
    visible_rows = decode_vga_rows(visible, VGA_VISIBLE_ROWS)
    marker_row = decode_vga_rows(marker, VGA_MARKER_ROWS)[0]

    capture_window(window_id, artifacts_dir, stem)
    dump_path = artifacts_dir / f"{stem}.txt"
    text = "\n".join(
        [
            "== Visible ==",
            *visible_rows,
            "",
            "== Marker ==",
            marker_row,
            "",
        ]
    )
    dump_path.write_text(text, encoding="utf-8")

    normalized = normalize_text(text)
    for pattern in FATAL_PATTERNS:
        if pattern.search(normalized):
            raise RuntimeError(f"fatal screen pattern {pattern.pattern!r} seen in {dump_path}")

    return dump_path, text


def wait_for_patterns(
    *,
    gdb: GdbMiSession,
    window_id: str,
    artifacts_dir: Path,
    label: str,
    patterns: list[re.Pattern[str]],
    timeout: float,
    interval: float = 0.5,
) -> tuple[Path, str]:
    deadline = time.time() + timeout
    attempt = 0
    latest_artifact: Path | None = None
    latest_text = ""

    gdb.continue_run()
    while time.time() < deadline:
        time.sleep(interval)
        gdb.interrupt()
        artifact_path, text = capture_screen(gdb, artifacts_dir, f"{label}-{attempt:02d}", window_id)
        latest_artifact = artifact_path
        latest_text = text
        normalized = normalize_text(text)
        if all(pattern.search(normalized) for pattern in patterns):
            print(f"[ok] {label}: {artifact_path}")
            return artifact_path, text
        attempt += 1
        gdb.continue_run()

    raise TimeoutError(
        f"timed out waiting for {label}; latest artifact={latest_artifact}, latest text={latest_text!r}"
    )


def run_shell_command(
    gdb: GdbMiSession,
    window_id: str,
    artifacts_dir: Path,
    *,
    label: str,
    command: str,
    patterns: list[re.Pattern[str]] | None = None,
    timeout: float = 10.0,
) -> tuple[Path, str]:
    print(f"[info] sending: {command}")
    type_command(window_id, command)
    expected = list(patterns or [])
    expected.append(PROMPT_PATTERN)
    return wait_for_patterns(
        gdb=gdb,
        window_id=window_id,
        artifacts_dir=artifacts_dir,
        label=label,
        patterns=expected,
        timeout=timeout,
    )


def scenario_boot_prompt_clean(gdb: GdbMiSession, window_id: str, artifacts_dir: Path, boot_timeout: float) -> None:
    print("[info] waiting for shell prompt")
    wait_for_patterns(
        gdb=gdb,
        window_id=window_id,
        artifacts_dir=artifacts_dir,
        label="boot-prompt",
        patterns=[ROOT_PROMPT_PATTERN, literal_pattern("BOOT:SH")],
        timeout=boot_timeout,
    )


def scenario_disk_read_sector(gdb: GdbMiSession, window_id: str, artifacts_dir: Path, boot_timeout: float) -> None:
    wait_for_patterns(
        gdb=gdb,
        window_id=window_id,
        artifacts_dir=artifacts_dir,
        label="disk-read-sector",
        patterns=[literal_pattern("DISKCASE:read_sector:PASS")],
        timeout=boot_timeout,
    )


def scenario_disk_cross_sector_read(gdb: GdbMiSession, window_id: str, artifacts_dir: Path, boot_timeout: float) -> None:
    wait_for_patterns(
        gdb=gdb,
        window_id=window_id,
        artifacts_dir=artifacts_dir,
        label="disk-cross-sector-read",
        patterns=[literal_pattern("DISKCASE:cross_sector_read:PASS")],
        timeout=boot_timeout,
    )


def scenario_disk_read_after_write(gdb: GdbMiSession, window_id: str, artifacts_dir: Path, boot_timeout: float) -> None:
    wait_for_patterns(
        gdb=gdb,
        window_id=window_id,
        artifacts_dir=artifacts_dir,
        label="disk-read-after-write",
        patterns=[literal_pattern("DISKCASE:read_after_write:PASS")],
        timeout=boot_timeout,
    )


def scenario_disk_partition_table_scan(gdb: GdbMiSession, window_id: str, artifacts_dir: Path, boot_timeout: float) -> None:
    wait_for_patterns(
        gdb=gdb,
        window_id=window_id,
        artifacts_dir=artifacts_dir,
        label="disk-partition-table-scan",
        patterns=[literal_pattern("DISKCASE:partition_table_scan:PASS")],
        timeout=boot_timeout,
    )


def scenario_disk_multi_sector_read(gdb: GdbMiSession, window_id: str, artifacts_dir: Path, boot_timeout: float) -> None:
    wait_for_patterns(
        gdb=gdb,
        window_id=window_id,
        artifacts_dir=artifacts_dir,
        label="disk-multi-sector-read",
        patterns=[
            literal_pattern("DISKCASE:multi_sector_read:PASS"),
            literal_pattern("DISKSTAT:multi_sector_read:cmds=1"),
            literal_pattern("timeouts=0"),
            literal_pattern("reads=16"),
        ],
        timeout=boot_timeout,
    )


def scenario_disk_max_transfer_boundary_read(gdb: GdbMiSession, window_id: str, artifacts_dir: Path, boot_timeout: float) -> None:
    wait_for_patterns(
        gdb=gdb,
        window_id=window_id,
        artifacts_dir=artifacts_dir,
        label="disk-max-transfer-boundary-read",
        patterns=[
            literal_pattern("DISKCASE:max_transfer_boundary_read:PASS"),
            literal_pattern("DISKSTAT:max_transfer_boundary_read:cmds=2"),
            literal_pattern("timeouts=0"),
            literal_pattern("reads=257"),
        ],
        timeout=boot_timeout,
    )


def scenario_disk_multi_sector_write(gdb: GdbMiSession, window_id: str, artifacts_dir: Path, boot_timeout: float) -> None:
    wait_for_patterns(
        gdb=gdb,
        window_id=window_id,
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
    )


def run_disk_scenario(
    *,
    scenario: str,
    artifacts_dir: Path,
    bochs_binary: str,
    bochs_config: Path,
    kernel_path: Path,
    os_image: Path,
    data_image: Path,
    clone_images: bool,
    boot_timeout: float,
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
    rendered_config = artifacts_dir / "bochsrc.test.disk"
    render_bochs_config(bochs_config, rendered_config, os_image, data_image, artifacts_dir)

    cmd = [bochs_binary, "-qf", str(rendered_config)]
    print(f"[info] artifacts: {artifacts_dir}")
    print(f"[info] bochs: {' '.join(cmd)}")
    proc = subprocess.Popen(
        cmd,
        cwd=REPO_ROOT,
        stdout=subprocess.DEVNULL,
        stderr=subprocess.STDOUT,
        start_new_session=True,
    )
    gdb: GdbMiSession | None = None

    try:
        window_id = find_bochs_window()
        print(f"[info] bochs window: {window_id}")
        gdb = connect_gdb(kernel_path, 1234)

        if scenario == "disk_read_sector":
            scenario_disk_read_sector(gdb, window_id, artifacts_dir, boot_timeout)
        elif scenario == "disk_cross_sector_read":
            scenario_disk_cross_sector_read(gdb, window_id, artifacts_dir, boot_timeout)
        elif scenario == "disk_read_after_write":
            scenario_disk_read_after_write(gdb, window_id, artifacts_dir, boot_timeout)
        elif scenario == "disk_partition_table_scan":
            scenario_disk_partition_table_scan(gdb, window_id, artifacts_dir, boot_timeout)
        elif scenario == "disk_multi_sector_read":
            scenario_disk_multi_sector_read(gdb, window_id, artifacts_dir, boot_timeout)
        elif scenario == "disk_max_transfer_boundary_read":
            scenario_disk_max_transfer_boundary_read(gdb, window_id, artifacts_dir, boot_timeout)
        elif scenario == "disk_multi_sector_write":
            scenario_disk_multi_sector_write(gdb, window_id, artifacts_dir, boot_timeout)
        else:
            raise ValueError(f"unknown scenario: {scenario}")

        (artifacts_dir / "scenario.pass").write_text(f"{scenario}\n", encoding="utf-8")
        print(f"[ok] scenario passed: {scenario}")
        return 0
    finally:
        if gdb is not None:
            gdb.close()
        os.killpg(proc.pid, signal.SIGTERM)
        try:
            proc.wait(timeout=5)
        except subprocess.TimeoutExpired:
            os.killpg(proc.pid, signal.SIGKILL)
            proc.wait(timeout=5)


def smoke(artifacts_dir: Path, bochs_binary: str, bochs_config: Path, kernel_path: Path, boot_timeout: float) -> int:
    rendered_config = bochs_config.resolve()
    cmd = [bochs_binary, "-qf", str(rendered_config)]
    artifacts_dir.parent.mkdir(parents=True, exist_ok=True)
    if artifacts_dir.exists():
        shutil.rmtree(artifacts_dir)
    artifacts_dir.mkdir(parents=True, exist_ok=True)

    print(f"[info] artifacts: {artifacts_dir}")
    print(f"[info] bochs: {' '.join(cmd)}")
    proc = subprocess.Popen(
        cmd,
        cwd=REPO_ROOT,
        stdout=subprocess.DEVNULL,
        stderr=subprocess.STDOUT,
        start_new_session=True,
    )
    gdb: GdbMiSession | None = None

    try:
        window_id = find_bochs_window()
        print(f"[info] bochs window: {window_id}")
        gdb = connect_gdb(kernel_path, 1234)

        scenario_boot_prompt_clean(gdb, window_id, artifacts_dir, boot_timeout)
        run_shell_command(gdb, window_id, artifacts_dir, label="ps", command="ps", patterns=[literal_pattern("PID"), literal_pattern("COMMAND")])
        run_shell_command(gdb, window_id, artifacts_dir, label="pwd", command="pwd", patterns=[literal_pattern("/")])
        print("[ok] bochs shell smoke passed")
        return 0
    finally:
        if gdb is not None:
            gdb.close()
        os.killpg(proc.pid, signal.SIGTERM)
        try:
            proc.wait(timeout=5)
        except subprocess.TimeoutExpired:
            os.killpg(proc.pid, signal.SIGKILL)
            proc.wait(timeout=5)


def command_smoke(args: argparse.Namespace) -> int:
    return smoke(
        Path(args.artifacts_dir),
        args.bochs_binary,
        Path(args.bochs_config).resolve(),
        Path(args.kernel).resolve(),
        args.boot_timeout,
    )


def command_test(args: argparse.Namespace) -> int:
    return run_disk_scenario(
        scenario=args.scenario,
        artifacts_dir=Path(args.artifacts_dir),
        bochs_binary=args.bochs_binary,
        bochs_config=Path(args.bochs_config).resolve(),
        kernel_path=Path(args.kernel).resolve(),
        os_image=Path(args.os_image).resolve(),
        data_image=Path(args.data_image).resolve(),
        clone_images=args.clone_images,
        boot_timeout=args.boot_timeout,
    )


def parser() -> argparse.ArgumentParser:
    p = argparse.ArgumentParser(description="Drive and test knix under Bochs via gdbstub.")
    sub = p.add_subparsers(dest="cmd")

    smoke_p = sub.add_parser("smoke", help="Run the legacy Bochs shell smoke test.")
    smoke_p.add_argument("--artifacts-dir", default=str(DEFAULT_ARTIFACTS_DIR))
    smoke_p.add_argument("--bochs-binary", default="/usr/local/bochs-gdb/bin/bochs")
    smoke_p.add_argument("--bochs-config", default="bochsrc-gdb.disk")
    smoke_p.add_argument("--kernel", default=str(REPO_ROOT / "build-fs" / "bin" / "kernel"))
    smoke_p.add_argument("--boot-timeout", type=float, default=20.0)
    smoke_p.set_defaults(func=command_smoke)

    test_p = sub.add_parser("test", help="Run a Bochs disk regression scenario.")
    test_p.add_argument(
        "--scenario",
        required=True,
        choices=[
            "disk_read_sector",
            "disk_cross_sector_read",
            "disk_read_after_write",
            "disk_partition_table_scan",
            "disk_multi_sector_read",
            "disk_max_transfer_boundary_read",
            "disk_multi_sector_write",
        ],
    )
    test_p.add_argument("--artifacts-dir", default=str(DEFAULT_ARTIFACTS_DIR))
    test_p.add_argument("--bochs-binary", default="/usr/local/bochs-gdb/bin/bochs")
    test_p.add_argument("--bochs-config", default="bochsrc-gdb.disk")
    test_p.add_argument("--kernel", default=str(REPO_ROOT / "build-fs" / "bin" / "kernel"))
    test_p.add_argument("--os-image", required=True)
    test_p.add_argument("--data-image", required=True)
    test_p.add_argument("--clone-images", action="store_true")
    test_p.add_argument("--boot-timeout", type=float, default=20.0)
    test_p.set_defaults(func=command_test)
    return p


def main() -> int:
    argv = sys.argv[1:]
    if not argv or argv[0].startswith("-"):
        argv = ["smoke", *argv]
    args = parser().parse_args(argv)
    return int(args.func(args))


if __name__ == "__main__":
    raise SystemExit(main())
