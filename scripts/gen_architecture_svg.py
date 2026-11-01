"""Generate knix architecture SVGs (en/zh x light/dark)."""
import sys
from pathlib import Path
from html import escape

OUT = Path(sys.argv[1]) if len(sys.argv) > 1 else Path(__file__).resolve().parent.parent / "assets" / "readme"

THEMES = {
    "light": dict(bg="#ffffff", panel="#f6f8fa", panel_stroke="#d8dee4", card="#ffffff",
                  card_stroke="#d0d7de", text="#1f2328", sub="#59636e", faint="#818b98",
                  chip="#eef1f4", chip_text="#3b434b", shadow="#1f2328", shadow_op="0.06"),
    "dark": dict(bg="#0d1117", panel="#151b23", panel_stroke="#2a313c", card="#1c232d",
                 card_stroke="#333c47", text="#e6edf3", sub="#9ea7b3", faint="#768390",
                 chip="#262e38", chip_text="#c9d1d9", shadow="#000000", shadow_op="0.35"),
}

ACCENT = {
    "user": ("#6366f1", "#818cf8"),
    "proc": ("#8b5cf6", "#a78bfa"),
    "mem": ("#0ea5e9", "#38bdf8"),
    "fs": ("#10b981", "#34d399"),
    "intr": ("#f59e0b", "#fbbf24"),
    "blk": ("#14b8a6", "#2dd4bf"),
    "drv": ("#ef4444", "#f87171"),
    "std": ("#ec4899", "#f472b6"),
    "boot": ("#64748b", "#94a3b8"),
}

TEXT = {
    "en": dict(
        ring3="USER SPACE · RING 3", ring0="KERNEL · RING 0", hw="HARDWARE", boot="BOOT",
        syscall="int 0x80  ·  26 system calls",
        user=[("Shell", "builtins · fork / exec / wait"),
              ("User programs", "cat · test runners · ELF"),
              ("CRT + user std", "argc / argv · print · syscall")],
        proc=("Processes & Scheduling", ["PCB + kernel stack in one page", "Round-robin · 100Hz PIT",
                                          "Semaphore · reentrant mutex", "fork · exec · wait · exit"]),
        mem=("Memory", ["2-level paging · 3G / 1G split", "Recursive page directory",
                        "Bitmap pools: kernel / user", "Arena malloc · 16B – 1KB"]),
        fs=("File System", ["inode · 12 direct + 1 indirect", "Directories · path resolution",
                            "Global file table + fd table", "open · read · write · stat"]),
        intr=("Interrupts", "IDT × 129  ·  8259A PIC × 2  ·  single TSS  ·  exceptions"),
        blk=("Block Device Layer", "read / write_blocks · partitions"),
        drv="Drivers", drv_chips=["PIT", "Keyboard", "Console", "PCI", "AHCI", "ATA PIO"],
        std=("Freestanding std · C++26 Modules", "string · vector · array · format · print · ranges"),
        hw_desc="i386  ·  single core  ·  QEMU / Bochs",
        boot_steps=["BIOS", "MBR", "Loader", "Paging", "Kernel", "init", "Shell"],
    ),
    "zh": dict(
        ring3="用户态 · RING 3", ring0="内核 · RING 0", hw="硬件", boot="启动",
        syscall="int 0x80  ·  26 个系统调用",
        user=[("Shell", "内建命令 · fork / exec / wait"),
              ("用户程序", "cat · 测试程序 · ELF"),
              ("CRT + 用户态 std", "argc / argv · print · syscall")],
        proc=("进程与调度", ["PCB 与内核栈同页", "时间片轮转 · 100Hz PIT",
                            "信号量 · 可重入互斥锁", "fork · exec · wait · exit"]),
        mem=("内存管理", ["二级分页 · 3G / 1G 划分", "页目录自映射",
                         "位图内存池：内核 / 用户", "Arena 分配器 · 16B – 1KB"]),
        fs=("文件系统", ["inode · 12 直接块 + 1 间接块", "目录 · 路径解析",
                        "全局文件表 + 进程 fd 表", "open · read · write · stat"]),
        intr=("中断", "IDT × 129  ·  8259A PIC × 2  ·  单 TSS  ·  异常处理"),
        blk=("块设备层", "read / write_blocks · 分区扫描"),
        drv="驱动", drv_chips=["PIT", "键盘", "控制台", "PCI", "AHCI", "ATA PIO"],
        std=("Freestanding std · C++26 Modules", "string · vector · array · format · print · ranges"),
        hw_desc="i386  ·  单核  ·  QEMU / Bochs",
        boot_steps=["BIOS", "MBR", "Loader", "分页", "Kernel", "init", "Shell"],
    ),
}

FONT = "-apple-system, BlinkMacSystemFont, 'Segoe UI', 'Noto Sans', 'PingFang SC', 'Microsoft YaHei', 'Noto Sans CJK SC', Helvetica, Arial, sans-serif"
MONO = "ui-monospace, SFMono-Regular, 'SF Mono', Menlo, Consolas, 'Liberation Mono', monospace"

W, H = 960, 720
X0, X1 = 24, 936           # panel bounds
CX0, CX1 = 44, 916         # card bounds inside panels
GAP = 16
COL = (CX1 - CX0 - 2 * GAP) / 3


def est(s, size=12):
    return sum(size if ord(ch) > 0x2E80 else size * 0.58 for ch in s)


def generate(lang, theme):
    t, c = TEXT[lang], THEMES[theme]
    dark = theme == "dark"
    acc = {k: v[1] if dark else v[0] for k, v in ACCENT.items()}
    out = []
    a = out.append

    def text(x, y, s, size=13, fill=None, weight=400, anchor="start", mono=False, spacing=0):
        fam = MONO if mono else FONT
        ls = f' letter-spacing="{spacing}"' if spacing else ""
        a(f'<text x="{x:.1f}" y="{y:.1f}" font-family="{fam}" font-size="{size}" font-weight="{weight}" '
          f'fill="{fill or c["text"]}" text-anchor="{anchor}"{ls}>{escape(s)}</text>')

    def panel(y, h, label, color, mid=None):
        a(f'<rect x="{X0}" y="{y}" width="{X1 - X0}" height="{h}" rx="16" fill="{c["panel"]}" stroke="{c["panel_stroke"]}"/>')
        m = mid if mid is not None else 22
        a(f'<circle cx="{X0 + 22}" cy="{y + m}" r="4" fill="{color}"/>')
        text(X0 + 34, y + m + 4.5, label, size=11.5, fill=c["sub"], weight=600, spacing=1.2)

    def card(x, y, w, h, color):
        a(f'<rect x="{x:.1f}" y="{y}" width="{w:.1f}" height="{h}" rx="12" fill="{c["card"]}" '
          f'stroke="{c["card_stroke"]}" filter="url(#sh)"/>')
        a(f'<rect x="{x + 14:.1f}" y="{y}" width="{min(48, w - 28):.1f}" height="3" rx="1.5" fill="{color}"/>')

    def chip(x, y, s, color=None, w=None):
        w = w or (len(s) * 7.6 + 22)
        a(f'<rect x="{x:.1f}" y="{y}" width="{w:.1f}" height="26" rx="13" fill="{c["chip"]}"/>')
        if color:
            a(f'<circle cx="{x + 14:.1f}" cy="{y + 13}" r="3" fill="{color}"/>')
            text(x + 24, y + 17.5, s, size=12, fill=c["chip_text"], weight=500)
        else:
            text(x + w / 2, y + 17.5, s, size=12, fill=c["chip_text"], weight=500, anchor="middle")
        return w

    a(f'<svg xmlns="http://www.w3.org/2000/svg" width="{W}" height="{H}" viewBox="0 0 {W} {H}">')
    a(f'<defs><filter id="sh" x="-10%" y="-10%" width="120%" height="140%">'
      f'<feDropShadow dx="0" dy="1.5" stdDeviation="2" flood-color="{c["shadow"]}" flood-opacity="{c["shadow_op"]}"/></filter>'
      f'<marker id="ar" viewBox="0 0 10 10" refX="5" refY="5" markerWidth="7" markerHeight="7" orient="auto-start-reverse">'
      f'<path d="M1 1 L9 5 L1 9 z" fill="{c["faint"]}"/></marker></defs>')
    a(f'<rect width="{W}" height="{H}" rx="20" fill="{c["bg"]}"/>')

    # ---- user space
    y = 20
    panel(y, 112, t["ring3"], acc["user"])
    for i, (title, sub) in enumerate(t["user"]):
        x = CX0 + i * (COL + GAP)
        card(x, y + 42, COL, 56, acc["user"])
        text(x + 16, y + 66, title, size=14.5, weight=600)
        text(x + 16, y + 86, sub, size=12, fill=c["sub"])

    # ---- syscall boundary
    by = 152
    a(f'<line x1="{X0 + 8}" y1="{by}" x2="{X1 - 8}" y2="{by}" stroke="{c["faint"]}" stroke-dasharray="4 5" opacity="0.7"/>')
    pw = 300
    a(f'<rect x="{W / 2 - pw / 2}" y="{by - 15}" width="{pw}" height="30" rx="15" fill="{c["bg"]}" stroke="{acc["user"]}" stroke-width="1.5"/>')
    text(W / 2, by + 4.5, t["syscall"], size=12.5, weight=600, fill=acc["user"], anchor="middle", mono=lang == "en")
    for dx in (-190, 190):
        a(f'<line x1="{W / 2 + dx}" y1="{by - 14}" x2="{W / 2 + dx}" y2="{by + 14}" stroke="{c["faint"]}" '
          f'stroke-width="1.5" marker-end="url(#ar)" marker-start="url(#ar)"/>')

    # ---- kernel
    ky = 172
    panel(ky, 406, t["ring0"], acc["proc"])
    ry = ky + 42
    for i, key in enumerate(("proc", "mem", "fs")):
        title, items = t[key]
        x = CX0 + i * (COL + GAP)
        card(x, ry, COL, 140, acc[key])
        text(x + 16, ry + 30, title, size=15, weight=650)
        for j, it in enumerate(items):
            yy = ry + 56 + j * 21
            a(f'<circle cx="{x + 19:.1f}" cy="{yy - 4}" r="2.2" fill="{acc[key]}"/>')
            text(x + 29, yy, it, size=12.5, fill=c["sub"])

    # row B: interrupts (2 cols) + block layer (col 3)
    rb = ry + 156
    wi = 2 * COL + GAP
    card(CX0, rb, wi, 60, acc["intr"])
    text(CX0 + 16, rb + 26, t["intr"][0], size=14.5, weight=600)
    text(CX0 + 16, rb + 46, t["intr"][1], size=12.5, fill=c["sub"])
    bx = CX0 + 2 * (COL + GAP)
    card(bx, rb, COL, 60, acc["blk"])
    text(bx + 16, rb + 26, t["blk"][0], size=14.5, weight=600)
    text(bx + 16, rb + 46, t["blk"][1], size=12.5, fill=c["sub"])
    # FS -> block connector
    a(f'<line x1="{bx + COL / 2:.1f}" y1="{ry + 141}" x2="{bx + COL / 2:.1f}" y2="{rb - 3}" stroke="{c["faint"]}" stroke-width="1.5" marker-end="url(#ar)"/>')

    # row C: drivers
    rc = rb + 76
    card(CX0, rc, CX1 - CX0, 56, acc["drv"])
    text(CX0 + 16, rc + 34, t["drv"], size=14.5, weight=600)
    chips = t["drv_chips"]
    widths = [est(s) + 38 for s in chips]
    cx = CX1 - 16 - sum(widths) - (len(chips) - 1) * 10
    for s, cw in zip(chips, widths):
        chip(cx, rc + 15, s, acc["drv"], w=cw)
        cx += cw + 10
    a(f'<line x1="{bx + COL / 2:.1f}" y1="{rb + 61}" x2="{bx + COL / 2:.1f}" y2="{rc - 3}" stroke="{c["faint"]}" stroke-width="1.5" marker-end="url(#ar)"/>')

    # row D: std foundation
    rd = rc + 72
    card(CX0, rd, CX1 - CX0, 46, acc["std"])
    text(CX0 + 16, rd + 29, t["std"][0], size=14, weight=600)
    text(CX1 - 16, rd + 29, t["std"][1], size=12.5, fill=c["sub"], anchor="end", mono=True)

    # ---- hardware
    hy = ky + 406 + 16
    panel(hy, 44, t["hw"], acc["boot"], mid=22)
    text(X1 - 20, hy + 27, t["hw_desc"], size=12.5, fill=c["sub"], anchor="end", mono=True)

    # ---- boot chain
    sy = hy + 60
    panel(sy, 56, t["boot"], acc["boot"], mid=28)
    steps = t["boot_steps"]
    sw, sg = 86, 30
    total = len(steps) * sw + (len(steps) - 1) * sg
    sx = X1 - 20 - total
    for i, s in enumerate(steps):
        chip(sx, sy + 15, s, w=sw)
        if i < len(steps) - 1:
            a(f'<line x1="{sx + sw + 6}" y1="{sy + 28}" x2="{sx + sw + sg - 6}" y2="{sy + 28}" stroke="{c["faint"]}" stroke-width="1.5" marker-end="url(#ar)"/>')
        sx += sw + sg

    a('</svg>')
    return "\n".join(out)


OUT.mkdir(parents=True, exist_ok=True)
for lang in TEXT:
    for theme in THEMES:
        (OUT / f"architecture-{lang}-{theme}.svg").write_text(generate(lang, theme), encoding="utf-8")
print("ok")
