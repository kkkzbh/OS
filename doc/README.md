# OS Project LaTeX Docs

This directory contains a ctex-based LaTeX book skeleton with a custom style, designed for textbook-like readability and rich code highlighting.

## Build

- Recommended: TeX Live (or MiKTeX) with `ctex`, `fontspec`, `tcolorbox`, `hyperref`, `titlesec`, `fancyhdr`.
- Code highlighting: uses `minted` (REQUIRED) with `-shell-escape` and Python Pygments.

Quick build (from this `doc/` folder):

```bash
latexmk
# Output: _build/main.pdf
```

Or explicitly:

```bash
latexmk -xelatex -shell-escape -interaction=nonstopmode -halt-on-error -outdir=_build main.tex
```

## Required Fonts (no fallback)

The style now requires these fonts to be installed, otherwise compilation will fail:

- Latin serif: Libertinus Serif
- Latin sans: Inter
- Monospace: JetBrains Mono
- CJK serif: Noto Serif CJK SC（或 Source Han Serif SC 同源替代）
- CJK sans: Noto Sans CJK SC（或 Source Han Sans SC 同源替代）

Install hints:
- Windows: install the OTF/TTF files (JetBrains Mono, Inter, Source Han Serif/Sans SC, Libertinus). Then reopen your shell/editor.
- macOS: install via Font Book; or `brew install --cask font-jetbrains-mono` and download Source Han fonts from Adobe.
- Linux: place fonts in `~/.local/share/fonts` and run `fc-cache -f`, or install distro packages if available.

Ubuntu/WSL quick install for CJK:

```bash
sudo apt update
sudo apt install -y fonts-noto-cjk
# Verify
fc-list | rg 'Noto (Sans|Serif) CJK SC'
```

## Structure

- `main.tex` — entry point (ctexbook)
- `style/custom.sty` — project-specific style (fonts, colors, boxes, code)
- `chapters/` — chapter files, included by `main.tex`
- `figures/` — images (placeholder)
- `generated/` — build-generated snippets/constants (placeholder)

## Code Snippets

- Inline code blocks:

```tex
\begin{codeblock}{nasm}
; your asm here
\end{codeblock}
```

- Include a file with language hint:

```tex
% options are optional (e.g. firstline/lastline)
\codefile[firstline=1,lastline=60]{c++}{../kernel/main.cpp}
```

Note: `minted` fallback is disabled. If `minted`/Pygments is missing, compilation fails by design.

## Documentation Roadmap

The book follows the implementation order of this repository: each chapter documents the code paths that are already present in the boot sector, loader, kernel core, and modules. Keep this roadmap in sync whenever the codebase evolves or the documentation plan changes.

| Chapter | Focus | Key Source Files/Modules | Status |
| --- | --- | --- | --- |
| 前言 | 项目背景、构建方式与阅读指引 | `doc/chapters/00-preface.tex` | ✅ 已完成 |
| 01 引导程序 | 磁盘布局、MBR、Loader、进入保护模式 | `boot/mbr.asm`, `boot/loader.asm`, `boot/include/boot.inc` | ✅ 草稿完成 |
| 02 基础输出（stdio） | `putchar`/`puthex`/`puts`、VGA 文本模式、光标 | `kernel/src/stdio.asm` | ✅ 新增 |
| 03 中断与异常 | IDT 构建、8259A、异常处理与装载 | `kernel/interrupt.c`, `kernel/interrupt.asm` | ✅ 新增 |
| 04 物理内存探测与分页 | BIOS 内存检测、页目录/页表构建、内核高半区映射 | `boot/loader.asm` (`setup_page`), `kernel/module/memory/pgtable.cppm`, `kernel/module/memory/alloc` | ✅ 已迁移 |
| 05 中断与异常子系统 | IDT 构建、可编程中断控制器、键盘与定时器中断 | `kernel/interrupt.asm`, `kernel/interrupt.c`, `device/keyboard`, `device/time` | ⏳ 计划中 |
| 05 线程与调度 | 线程控制块、上下文切换、定时器驱动调度 | `kernel/module/thread`, `kernel/module/tss.cppm` | ⏳ 计划中 |
| 06 内存分配器 | 物理页分配、内核堆、bitmap 工具 | `kernel/module/memory/alloc`, `kernel/module/bitmap.cppm` | ⏳ 计划中 |
| 07 同步与系统调用 | 自旋锁/信号量、系统调用入口与分发 | `kernel/module/thread`, `kernel/module/syscall`, `kernel/include/interrupt.h` | ⏳ 计划中 |
| 08 文件系统与磁盘驱动 | IDE 驱动、ioqueue、文件系统初始化及 API | `device/ide`, `device/ioqueue`, `kernel/module/filesystem` | ⏳ 计划中 |
| 09 用户态与 Shell | 用户态加载器、简单 Shell、示例程序 | `kernel/module/user`, `kernel/module/shell` | ⏳ 计划中 |
| 附录 | 工具链、调试脚本、Bochs/QEMU 配置 | `bochsrc*.disk`, `device/` & `doc/figures/` (待补充) | ⏳ 计划中 |

### 使用说明

- 在修改 `doc/` 目录下的任何文件前，请重新阅读本 README，确认路线图与维护规则仍然适用。
- 若对章节结构、撰写顺序或规划有调整，务必同步更新本文件与对应的章节源文件，使路线图与正文保持一致。
- 撰写或修改章节时，请记录涉及的源代码文件，必要时扩充上表的 “Key Source Files/Modules” 以便交叉引用。
- 完成某章节后，将状态列改为 ✅ 并注明完成阶段（草稿/审阅/定稿）。
- 若章节需要跨越多个模块，请在“Key Source Files/Modules”列中列出关键入口，帮助读者快速定位实现。
- 保持叙述性与客观性：章节应直接描述已实现的功能、实现方法与相关源码位置，避免“读者应该……”此类引导语或假定读者水平的措辞。
