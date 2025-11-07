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
