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
import sys
import time
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parent.parent
DEFAULT_BUILD_DIR = REPO_ROOT / "cmake-build-debug"
DEFAULT_ARTIFACTS_DIR = REPO_ROOT / ".cache" / "qemu-smoke" / "latest"
DEFAULT_GDB_PORT = 1234
DEFAULT_QMP_SOCKET = REPO_ROOT / ".cache" / "qemu-smoke" / "qmp.sock"


def run(cmd: list[str], *, cwd: Path | None = None, check: bool = True) -> subprocess.CompletedProcess[str]:
    return subprocess.run(cmd, cwd=cwd, check=check, text=True, capture_output=True)


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
        f"file={REPO_ROOT / 'hd64M.img'},if=ide,index=0,media=disk,format=raw",
        "-drive",
        f"file={REPO_ROOT / 'hd80M.img'},if=ide,index=1,media=disk,format=raw",
    ]

    if display == "none":
        cmd.extend(["-display", "none"])
    else:
        cmd.extend(["-display", display])

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


def ensure_images_exist() -> None:
    for name in ("hd64M.img", "hd80M.img"):
        path = REPO_ROOT / name
        if not path.exists():
            raise FileNotFoundError(f"missing disk image: {path}")


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


def capture_ocr(qmp: QmpClient, artifacts_dir: Path, stem: str) -> tuple[Path, Path, str]:
    ppm_path = artifacts_dir / f"{stem}.ppm"
    qmp.screendump(ppm_path)
    png_path = preprocess_image(ppm_path)
    text = ocr_image(png_path)
    (artifacts_dir / f"{stem}.txt").write_text(text)
    return ppm_path, png_path, text


def capture_vga_text(qmp: QmpClient, artifacts_dir: Path, stem: str) -> tuple[Path, str]:
    bin_path = artifacts_dir / f"{stem}.vram.bin"
    rel_bin_path = bin_path.relative_to(REPO_ROOT)
    qmp.hmp(f"pmemsave 0xb8000 4000 {rel_bin_path}")
    text = decode_vga_text(bin_path.read_bytes())
    (artifacts_dir / f"{stem}.txt").write_text(text)
    return bin_path, text


def normalize_text(text: str) -> str:
    return " ".join(text.upper().split())


def wait_for_patterns(
    *,
    qmp: QmpClient,
    artifacts_dir: Path,
    label: str,
    patterns: list[re.Pattern[str]],
    timeout: float,
    interval: float = 1.0,
    capture: str = "ocr",
) -> tuple[Path, str]:
    deadline = time.time() + timeout
    attempt = 0
    latest_artifact: Path | None = None
    latest_text = ""

    while time.time() < deadline:
        stem = f"{label}-{attempt:02d}"
        if capture == "vga":
            artifact_path, text = capture_vga_text(qmp, artifacts_dir, stem)
        else:
            artifact_path, _, text = capture_ocr(qmp, artifacts_dir, stem)
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


def build_smoke_patterns(path_fragment: str) -> list[re.Pattern[str]]:
    return [
        re.compile(r"KKKZBH"),
        re.compile(re.escape(path_fragment.upper())),
    ]


def smoke(build_dir: Path, qemu_binary: str, artifacts_dir: Path, boot_timeout: float) -> int:
    ensure_images_exist()
    artifacts_dir.parent.mkdir(parents=True, exist_ok=True)
    if artifacts_dir.exists():
        shutil.rmtree(artifacts_dir)
    artifacts_dir.mkdir(parents=True, exist_ok=True)

    qmp_socket = DEFAULT_QMP_SOCKET
    if qmp_socket.exists():
        qmp_socket.unlink()

    qemu_log = artifacts_dir / "qemu.log"
    cmd = qemu_command(
        qemu_binary=qemu_binary,
        display="none",
        qmp_socket=qmp_socket,
        monitor="none",
        gdb_port=None,
        paused=False,
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
        print("[info] waiting for boot banner")
        wait_for_patterns(
            qmp=qmp,
            artifacts_dir=artifacts_dir,
            label="boot",
            patterns=[re.compile(r"KKK[Z2]BH")],
            timeout=boot_timeout,
            capture="vga",
        )

        print("[info] waiting for shell prompt")
        wait_for_patterns(
            qmp=qmp,
            artifacts_dir=artifacts_dir,
            label="prompt",
            patterns=[re.compile(r"K@KKKZBH /\$ >")],
            timeout=boot_timeout,
            capture="vga",
        )

        print("[info] sending: ps")
        send_text(qmp, "ps\n")
        wait_for_patterns(
            qmp=qmp,
            artifacts_dir=artifacts_dir,
            label="ps",
            patterns=[re.compile(r"PID"), re.compile(r"COMMAND")],
            timeout=10,
            capture="vga",
        )

        print("[info] sending: mkdir /qq")
        send_text(qmp, "mkdir /qq\n")
        time.sleep(1.5)

        print("[info] sending: cd /qq")
        send_text(qmp, "cd /qq\n")
        wait_for_patterns(
            qmp=qmp,
            artifacts_dir=artifacts_dir,
            label="cd-qq",
            patterns=build_smoke_patterns("/QQ"),
            timeout=10,
            capture="vga",
        )

        print("[info] sending: pwd")
        send_text(qmp, "pwd\n")
        wait_for_patterns(
            qmp=qmp,
            artifacts_dir=artifacts_dir,
            label="pwd-qq",
            patterns=[re.compile(r"/QQ")],
            timeout=10,
            capture="vga",
        )
        print("[ok] qemu shell smoke passed")
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
    ensure_images_exist()
    cmd = qemu_command(
        qemu_binary=args.qemu_binary,
        display=args.display,
        qmp_socket=Path(args.qmp_socket) if args.qmp_socket else None,
        monitor=args.monitor,
        gdb_port=args.gdb,
        paused=args.paused,
    )
    os.execvp(cmd[0], cmd)
    return 0


def command_smoke(args: argparse.Namespace) -> int:
    return smoke(
        build_dir=Path(args.build_dir),
        qemu_binary=args.qemu_binary,
        artifacts_dir=Path(args.artifacts_dir),
        boot_timeout=args.boot_timeout,
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
    run_p.set_defaults(func=command_run)

    smoke_p = sub.add_parser("smoke", help="Run a headless shell smoke test.")
    smoke_p.add_argument("--build-dir", default=str(DEFAULT_BUILD_DIR))
    smoke_p.add_argument("--qemu-binary", default=shutil.which("qemu-system-i386") or "qemu-system-i386")
    smoke_p.add_argument("--artifacts-dir", default=str(DEFAULT_ARTIFACTS_DIR))
    smoke_p.add_argument("--boot-timeout", type=float, default=40.0)
    smoke_p.set_defaults(func=command_smoke)
    return p


def main() -> int:
    args = parser().parse_args()
    return int(args.func(args))


if __name__ == "__main__":
    raise SystemExit(main())
