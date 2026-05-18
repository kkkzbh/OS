# knix

`knix` 是一个基于 `x86 (i386)` 的实验型操作系统项目，使用 `C / C++ / NASM` 从引导扇区一路实现到内核、文件系统、系统调用与用户态 Shell。

项目当前重点是“把核心链路跑通并可调试”，而不是做完整的 POSIX 兼容系统。

## 目录

- [项目概览](#项目概览)
- [当前能力](#当前能力)
- [架构总览](#架构总览)
- [环境要求](#环境要求)
- [快速开始](#快速开始)
- [启动后验证](#启动后验证)
- [构建目标说明](#构建目标说明)
- [用户程序加载机制](#用户程序加载机制)
- [调试](#调试)
- [QEMU 调试](#qemu-调试)
- [项目结构](#项目结构)
- [已知限制](#已知限制)
- [路线图](#路线图)
- [贡献](#贡献)
- [License](#license)

## 项目概览

- 操作系统名称：`knix`
- 架构：`32-bit x86`（`-m32`，`freestanding`）
- 启动方式：`MBR -> Loader -> Kernel`
- 运行环境：`Bochs`
- 构建系统：`CMake + Ninja`

## 当前能力

- 启动链路
  - `boot/mbr.asm` + `boot/loader.asm`
  - 保护模式与分页切换
  - 内核入口链接到高地址（见 `kernel/CMakeLists.txt`）
- 内核基础设施
  - 中断管理：IDT、8259A、时钟中断、键盘中断
  - 内存管理：页表、位图、页分配、`malloc/free`（arena + page）
  - 线程与进程：调度、上下文切换、`fork/exec/wait/exit`
- 文件系统
  - 超级块、inode、目录项、路径解析
  - 文件/目录操作：`open/read/write/lseek/close`、`mkdir/rmdir`、`opendir/readdir`
- 用户态接口
  - 系统调用表（`int 0x80` 路径）
  - Shell 内建命令：`ls cd pwd ps clear mkdir rmdir rm`
  - 支持外部程序执行（`fork + exec + wait`）

## 架构总览

```text
boot/mbr.asm
  -> boot/loader.asm
     -> kernel entry (kkkzbh)
        -> start()
           -> init_all()
              -> idt/memory/thread/timer/keyboard/tss/syscall/ide/filesystem init
                 -> init user process
                    -> shell()
```

关键初始化顺序在 `kernel/init.cpp`，Shell 启动逻辑在 `kernel/module/thread/init.cppm`。

## 环境要求

以下要求是按当前仓库配置整理的“实际需求”：

- Linux（优先 Fedora）
- `CMake >= 4.0.1`（`CMakeLists.txt` 当前写死）
- `Ninja`
- `GCC/G++`（需支持 `C++ modules` 与 `-m32` 目标）
- `NASM`
- `dd`（用于写入磁盘镜像扇区）
- `Bochs` 两套可执行（按仓库默认路径）
  - `/usr/local/bochs/bin/bochs`
  - `/usr/local/bochs-gdb/bin/bochs`
- 可选：`gdb`

> 注意：仓库 `.gitignore` 忽略了 `hd*.img`，请确保本地存在 `hd64M.img` 与 `hd80M.img`。  
> 另外，文件系统默认挂载 `sdb1`（来自 `hd80M.img`），若该分区不存在，文件系统初始化会失败。

## 快速开始

### 1) 配置构建

```bash
cmake -S . -B build -G Ninja
```

如需正式启用内核源码级调试，请使用仓库约定的 `Debug` 配置：

```bash
/home/kkkzbh/.local/share/JetBrains/Toolbox/apps/clion/bin/cmake/linux/x64/bin/cmake \
  -S . -B build-debug -G Ninja -DCMAKE_BUILD_TYPE=Debug
```

### 2) 生成可启动镜像内容

```bash
cmake --build build --target osbuild
```

`osbuild` 会写入：

- MBR（`write_mbr`）
- Loader（`write_loader`）
- Kernel（`write_kernel`）
- 用户程序 `program_no_arg`（`write_program_no_arg`）

### 3) 启动系统

```bash
cmake --build build --target bochs
```

### 4) 运行 boot 回归测试

```bash
ctest --test-dir build --output-on-failure -L boot
```

这组测试会在 clean build 下自动准备镜像，并执行：

- `boot.image.layout`
- `boot.qemu.milestones`
- `boot.qemu.prompt_clean`

### 5) 运行文件系统回归测试

```bash
ctest --test-dir build --output-on-failure -L fs
```

这组测试会在 build 目录准备独立镜像副本，并覆盖两层场景：

- shell 黑盒：`pwd/ls/mkdir/rmdir/cd`
- syscall/selftest：文件、目录、元数据、错误路径、持久化

## 启动后验证

系统进入 Shell 后，可以先用内建命令做冒烟验证：

```text
pwd
ls
mkdir /tmp
cd /tmp
pwd
ps
```

若你已经启用 `write_execution()` 并写入了用户程序，可以尝试：

```text
cd /bin
ls
./a
```

## 构建目标说明

常用自定义目标：

- `osbuild`：构建并写入完整启动链
- `bochs`：普通运行
- `bochs-gdb`：开启 `gdbstub`（端口 `1234`）
- `bochs-smoke`：Bochs 图形窗口启动后自动发键并做 shell 冒烟验证
- `qemu`：用 QEMU 图形模式启动
- `qemu-gdb`：用 QEMU 启动并在 `tcp::1234` 等待 GDB
- `qemu-smoke`：保留的 legacy QEMU shell smoke，自动驱动 `ps` / `mkdir` / `cd` / `pwd`
- `fs_test_runner`：文件系统 syscall/selftest 用户程序
- `write_program_no_arg`：写入无参用户程序到扇区 `1000`
- `write_program_arg`：写入带参用户程序到扇区 `1100`
- `write_cat`：写入 `cat` 用户程序到扇区 `1300`
- `write_fs_test_runner`：写入 `fs_test_runner` 到扇区 `1500`

如需额外写入用户程序（默认 `osbuild` 不包含后两者）：

```bash
cmake --build build --target write_program_arg write_cat
```

## 用户程序加载机制

`knix` 的用户程序分两步落地：

1. 宿主机构建后写入裸盘固定扇区（`add_disk_target`）
2. 内核启动时由 `kernel/wexe.cpp` 的 `write_execution()` 把 ELF 导入文件系统 `/bin/*`

当前仓库默认会在 boot 时尝试把 `fs_test_runner` 导入 `/bin/fs_test_runner`，用于 `CTest` 文件系统测试；其余演示程序仍保留在 `kernel/wexe.cpp` 注释里，可按需手动开启：

- `std_print()` -> `/bin/a`
- `prog_arg()` -> `/bin/eb`
- `write_cat()` -> `/bin/cat`

## 调试

### Bochs + GDB

先用 `Debug` 配置构建：

```bash
/home/kkkzbh/.local/share/JetBrains/Toolbox/apps/clion/bin/cmake/linux/x64/bin/cmake \
  --build build-debug --target osbuild
```

1. 启动带 GDB stub 的模拟器：

```bash
cmake --build build-debug --target bochs-gdb
```

2. 在另一个终端连接：

```bash
gdb build-debug/bin/kernel
(gdb) target remote :1234
(gdb) list kernel/init.cpp:37
(gdb) b kernel/init.cpp:39
(gdb) c
```

`build-debug/bin/kernel` 是供 `gdb` 使用的主机侧 ELF；写入镜像的仍然是 `build-debug/bin/kernel_stripped`。

日志文件：

- 普通运行：`bochs.out`
- GDB 运行：`bochs-gdb.out`
- 串口输出（gdb 配置）：`serial.txt`

也可以直接跑 Bochs 冒烟测试：

```bash
cmake --build build --target bochs-smoke
```

它会启动 Bochs、等待 shell prompt、自动发送 `ps` 和 `pwd`，并把截图/OCR 工件写到 `.cache/bochs-smoke/latest/`。

## QEMU 调试

仓库现在提供了一套基于 `QEMU + QMP` 的调试和自动化验证路径：

```bash
ctest --test-dir build --output-on-failure -L boot
ctest --test-dir build --output-on-failure -L fs
cmake --build build --target qemu
cmake --build build --target qemu-gdb
cmake --build build --target qemu-smoke
```

- `ctest -L boot`：正式的 boot 回归测试入口，输出工件位于 `build/test/artifacts/*`
- `ctest -L fs`：正式的文件系统回归测试入口，覆盖 shell 场景与 `fs_test_runner`
- `qemu`：图形模式启动，适合日常交互验证；默认使用 `--accel auto --profile interactive-lowlatency --gtk-backend x11`，优先尝试 `KVM` 并通过 `X11/Xwayland GTK` 走低延迟交互配置
- `qemu-gdb`：QEMU 在 `tcp::1234` 等待；保留保守配置，仅默认使用 `--accel auto`；正式源码级调试请配合 `Debug` 构建并连接 `gdb <build-dir>/bin/kernel`
- `qemu-smoke`：兼容保留的 shell smoke，保留保守配置，仅默认使用 `--accel auto`，会在发现 fatal screen pattern 时直接失败

`scripts/qemu_debug.py` 现在支持统一的加速器选择：

- `--accel auto`：使用 `-machine pc,accel=kvm:tcg`，优先尝试 `KVM`，不可用时自动回退到 `TCG`
- `--accel kvm`：使用 `-machine pc,accel=kvm`，要求宿主机支持 `/dev/kvm`
- `--accel tcg`：使用 `-machine pc,accel=tcg`，适合复现纯软件模拟路径下的问题

交互式 `run` 还支持 host-side 低延迟选项：

- `--profile interactive-lowlatency`：将 machine 固定为 `pc-i440fx-10.1,accel=...,kernel-irqchip=on,hpet=off,usb=off`
- `--gtk-backend x11`：为 QEMU 注入 `GDK_BACKEND=x11`，优先走 `X11/Xwayland`
- `--gtk-backend wayland`：为 QEMU 注入 `GDK_BACKEND=wayland`
- `--gtk-backend auto`：不注入 host GTK 后端，由宿主 GTK 自行选择

也可以直接使用脚本：

```bash
python3 scripts/qemu_debug.py run --accel auto --profile interactive-lowlatency --gtk-backend x11
python3 scripts/qemu_debug.py run --accel auto --profile interactive-lowlatency --gtk-backend wayland
python3 scripts/qemu_debug.py run --accel kvm --gdb 1234 --paused
python3 scripts/qemu_debug.py smoke --accel auto
python3 scripts/qemu_debug.py test --scenario boot_milestones --accel auto
python3 scripts/qemu_debug.py test --scenario shell_fs_cwd --accel tcg
```

正式的 `QEMU` 内核源码级调试流程：

```bash
/home/kkkzbh/.local/share/JetBrains/Toolbox/apps/clion/bin/cmake/linux/x64/bin/cmake \
  -S . -B build-debug -G Ninja -DCMAKE_BUILD_TYPE=Debug
/home/kkkzbh/.local/share/JetBrains/Toolbox/apps/clion/bin/cmake/linux/x64/bin/cmake \
  --build build-debug --target osbuild qemu-gdb
gdb build-debug/bin/kernel
(gdb) target remote :1234
(gdb) list kernel/init.cpp:37
(gdb) b kernel/init.cpp:39
(gdb) c
```

`ctest -L boot` 和 `ctest -L fs` 的工件默认放在 `build/test/artifacts/`，`qemu-smoke` 的工件默认放在 `.cache/qemu-smoke/latest/`。其中 VGA 路径会保存：

- `*.vram.bin`：QEMU 文本模式显存快照
- `*.txt`：从显存解码出的屏幕文本

## 项目结构

```text
.
├── boot/                 # MBR 与 Loader
├── kernel/               # 内核主体（中断/内存/线程/系统调用/文件系统/Shell）
│   ├── module/
│   ├── include/
│   ├── crt/
│   └── test/
├── tests/                # CTest 顶层测试入口（boot/system）
├── device/               # 设备层（IDE/时间/键盘/控制台/ioqueue）
├── bochsrc.disk          # Bochs 普通运行配置
├── bochsrc-gdb.disk      # Bochs + GDB 配置
└── CMakeLists.txt
```

## 已知限制

- 与本地环境耦合较强（Bochs 路径、镜像文件、分区布局）。
- `CMake` 最低版本当前设置为 `4.0.1`，旧版本无法直接配置。
- 当前还没有 CI，但仓库已经接入 `CTest` 的 `boot` 与 `fs` 回归测试。
- 当前文件系统运行期存在真实缺陷：`mkdir` 与 `/bin/fs_test_runner` 执行路径在 QEMU 下会触发 `#PF`，`ctest -L fs` 会稳定暴露该问题。
- 用户程序导入文件系统目前默认只启用 `fs_test_runner`，其余示例程序仍需手动开启。
- 正式支持的源码级调试目前仅覆盖内核，并要求使用 `Debug` 构建；用户程序仍不在本阶段保证范围内。

## 路线图

- [ ] 降低/整理构建环境耦合（工具链与路径可配置化）
- [ ] 增加一键初始化镜像与分区脚本
- [ ] 修复 `mkdir` 与用户程序 `exec` 路径上的文件系统/进程回归
- [ ] 提供 QEMU 运行路径以提升开发效率

## 贡献

欢迎通过 Issue / PR 参与改进。建议在提交前：

1. 保持提交聚焦（单一主题）
2. 附上 Bochs 复现步骤或日志片段
3. 对涉及启动链路或文件系统的变更，补充验证命令

## License

当前仓库未提供明确许可证文件（如 `LICENSE`）。在复用代码前，请先与维护者确认授权范围。

---

如果你想先看实现细节，建议从 `kernel/init.cpp`、`kernel/module/thread/init.cppm`、`kernel/module/filesystem/` 和 `kernel/module/user/` 开始阅读。
