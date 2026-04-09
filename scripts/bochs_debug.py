#!/usr/bin/env python3
from __future__ import annotations

import argparse
import os
import re
import select
import shutil
import signal
import subprocess
import time
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parent.parent
DEFAULT_ARTIFACTS_DIR = REPO_ROOT / ".cache" / "bochs-smoke" / "latest"
VGA_TEXT_BASE = 0xC00B8000
VGA_VISIBLE_ROWS = 24
VGA_MARKER_ROWS = 1
VGA_COLS = 80
VGA_ROW_BYTES = VGA_COLS * 2
PROMPT_TEXT = "k@kkkzbh /$ >"


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


def dump_screen(gdb: GdbMiSession, artifacts_dir: Path, stem: str, window_id: str) -> tuple[list[str], str]:
    visible = gdb.read_bytes(VGA_TEXT_BASE, VGA_VISIBLE_ROWS * VGA_ROW_BYTES)
    marker = gdb.read_bytes(
        VGA_TEXT_BASE + VGA_VISIBLE_ROWS * VGA_ROW_BYTES,
        VGA_MARKER_ROWS * VGA_ROW_BYTES,
    )
    visible_rows = decode_vga_rows(visible, VGA_VISIBLE_ROWS)
    marker_row = decode_vga_rows(marker, VGA_MARKER_ROWS)[0]

    capture_window(window_id, artifacts_dir, stem)
    dump_path = artifacts_dir / f"{stem}.txt"
    dump_path.write_text(
        "\n".join(
            [
                "== Visible ==",
                *visible_rows,
                "",
                "== Marker ==",
                marker_row,
                "",
            ]
        )
    )
    return visible_rows, marker_row


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


def wait_for_guest_state(
    *,
    gdb: GdbMiSession,
    window_id: str,
    artifacts_dir: Path,
    label: str,
    timeout: float,
    matcher,
    interval: float = 0.5,
) -> tuple[list[str], str]:
    deadline = time.time() + timeout
    attempt = 0
    latest_rows: list[str] = []
    latest_marker = ""

    gdb.continue_run()
    while time.time() < deadline:
        time.sleep(interval)
        gdb.interrupt()
        stem = f"{label}-{attempt:02d}"
        latest_rows, latest_marker = dump_screen(gdb, artifacts_dir, stem, window_id)
        if matcher(latest_rows, latest_marker):
            print(f"[ok] {label}: {artifacts_dir / (stem + '.txt')}")
            return latest_rows, latest_marker
        attempt += 1
        gdb.continue_run()

    raise TimeoutError(
        f"timed out waiting for {label}; latest marker={latest_marker!r}; "
        f"latest visible tail={latest_rows[-5:]!r}"
    )


def has_prompt(rows: list[str], marker: str) -> bool:
    return "BOOT:SH" in marker and any(PROMPT_TEXT in row for row in rows)


def has_ps_output(rows: list[str], marker: str) -> bool:
    joined = "\n".join(rows)
    return "BOOT:SH" in marker and "PID" in joined and "COMMAND" in joined and PROMPT_TEXT in joined


def has_pwd_output(rows: list[str], marker: str) -> bool:
    joined = "\n".join(rows)
    return "BOOT:SH" in marker and PROMPT_TEXT in joined and "\n/" in ("\n" + joined)


def smoke(artifacts_dir: Path, bochs_binary: str, bochs_config: str, kernel_path: Path) -> int:
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
    gdb: GdbMiSession | None = None

    try:
        window_id = find_bochs_window()
        print(f"[info] bochs window: {window_id}")

        gdb = connect_gdb(kernel_path, 1234)

        wait_for_guest_state(
            gdb=gdb,
            window_id=window_id,
            artifacts_dir=artifacts_dir,
            label="prompt",
            timeout=20.0,
            matcher=has_prompt,
        )

        print("[info] sending: ps")
        type_command(window_id, "ps")
        wait_for_guest_state(
            gdb=gdb,
            window_id=window_id,
            artifacts_dir=artifacts_dir,
            label="ps",
            timeout=10.0,
            matcher=has_ps_output,
        )

        print("[info] sending: pwd")
        type_command(window_id, "pwd")
        wait_for_guest_state(
            gdb=gdb,
            window_id=window_id,
            artifacts_dir=artifacts_dir,
            label="pwd",
            timeout=10.0,
            matcher=has_pwd_output,
        )

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


def parser() -> argparse.ArgumentParser:
    p = argparse.ArgumentParser(description="Drive and smoke-test knix under Bochs via gdbstub.")
    p.add_argument("--artifacts-dir", default=str(DEFAULT_ARTIFACTS_DIR))
    p.add_argument("--bochs-binary", default="/usr/local/bochs-gdb/bin/bochs")
    p.add_argument("--bochs-config", default="bochsrc-gdb.disk")
    p.add_argument("--kernel", default=str(REPO_ROOT / "build-fs" / "bin" / "kernel"))
    return p


def main() -> int:
    args = parser().parse_args()
    return smoke(
        Path(args.artifacts_dir),
        args.bochs_binary,
        args.bochs_config,
        Path(args.kernel),
    )


if __name__ == "__main__":
    raise SystemExit(main())
