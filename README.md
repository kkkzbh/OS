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
- `write_program_no_arg`：写入无参用户程序到扇区 `1000`
- `write_program_arg`：写入带参用户程序到扇区 `1100`
- `write_cat`：写入 `cat` 用户程序到扇区 `1300`

如需额外写入用户程序（默认 `osbuild` 不包含后两者）：

```bash
cmake --build build --target write_program_arg write_cat
```

## 用户程序加载机制

`knix` 的用户程序分两步落地：

1. 宿主机构建后写入裸盘固定扇区（`add_disk_target`）
2. 内核启动时由 `kernel/wexe.cpp` 的 `write_execution()` 把 ELF 导入文件系统 `/bin/*`

当前仓库默认状态下，`write_execution()` 里的调用是注释掉的：

- `std_print()` -> `/bin/a`
- `prog_arg()` -> `/bin/eb`
- `write_cat()` -> `/bin/cat`

如果你要演示完整用户程序链路，需要按 `kernel/wexe.cpp` 的注释手动开启并重新构建。

## 调试

### Bochs + GDB

1. 启动带 GDB stub 的模拟器：

```bash
cmake --build build --target bochs-gdb
```

2. 在另一个终端连接：

```bash
gdb build/bin/kernel
(gdb) target remote :1234
(gdb) b kkkzbh
(gdb) c
```

日志文件：

- 普通运行：`bochs.out`
- GDB 运行：`bochs-gdb.out`
- 串口输出（gdb 配置）：`serial.txt`

## 项目结构

```text
.
├── boot/                 # MBR 与 Loader
├── kernel/               # 内核主体（中断/内存/线程/系统调用/文件系统/Shell）
│   ├── module/
│   ├── include/
│   ├── crt/
│   └── test/
├── device/               # 设备层（IDE/时间/键盘/控制台/ioqueue）
├── bochsrc.disk          # Bochs 普通运行配置
├── bochsrc-gdb.disk      # Bochs + GDB 配置
└── CMakeLists.txt
```

## 已知限制

- 与本地环境耦合较强（Bochs 路径、镜像文件、分区布局）。
- `CMake` 最低版本当前设置为 `4.0.1`，旧版本无法直接配置。
- 目前没有 CI 与自动化回归测试，主要依赖 Bochs 手工验证。
- 用户程序导入文件系统默认关闭（需手动启用 `write_execution()`）。

## 路线图

- [ ] 降低/整理构建环境耦合（工具链与路径可配置化）
- [ ] 增加一键初始化镜像与分区脚本
- [ ] 增加自动化测试（至少覆盖关键 syscall/fs 流程）
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
