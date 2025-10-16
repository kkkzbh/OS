# OS —— 自研 32 位内核与文件系统（Bochs 可运行/可调试）

一个从零开始实现的 32 位 i386 内核项目：自制引导、加载器、分页与高半内核、可编程中断控制、基础设备驱动（键盘、IDE 硬盘）、自研文件系统（带超级块/索引结点/目录等）、线程与内存子系统雏形，并在 Bochs 下运行与通过 GDB 远程调试。

本仓库采用 C/C++ 混合实现，并使用 C++ Modules（C++26），构建系统为 CMake，硬件模拟与调试使用 Bochs + GDB。

> 温馨提示：本文面向 Linux 开发环境（仓库作者在 Linux 下开发）。若在其他系统，请参考等价安装包与路径。

---

## 项目亮点与特性

- 自制引导链
  - MBR（`boot/mbr.asm`）：加载第二阶段加载器
  - Loader（`boot/loader.asm`）：切换保护模式，搭建分页（低端 1MB + 内核高半映射），加载 ELF 内核到高端，跳转内核入口
- 32 位 i386 内核（高半内核）
  - 链接地址 0xC0001500，入口符号为 `kkkzbh`（见 `kernel/CMakeLists.txt` 的 `-Ttext` 与 `-e` 设置）
  - 开启分页，虚拟内存布局：用户 3G + 内核 1G（0xC0000000~0xFFFFFFFF）
- 中断与时钟
  - IDT 建立与中断框架（`kernel/interrupt.*`）
  - PIT 定时器初始化（在设备侧 `device/time/` 与 `kernel/init.cpp` 中调用）
  - 键盘中断驱动（扫描码解析、Shift/Ctrl/Alt/Caps 状态机，`device/keyboard/keyboard.c`）
- IDE 硬盘与分区扫描
  - IDE 通道初始化、中断处理与分区扫描（主/扩展/逻辑分区），见 `device/ide/*`
  - 自动识别并打印分区信息
- 自研文件系统
  - 超级块魔数：`0x19590318`（`kernel/module/filesystem/init.cpp`）
  - 具备目录/文件/读写/遍历/删除等基本操作与测试用例（`kernel/module/filesystem/*`）
  - 默认挂载示例分区如 `sdb1`（见 `filesystem_init`）
- 线程与内存模块雏形
  - `kernel/module/thread/*`、`kernel/module/memory/*` 与 `init_all()` 中的启动顺序
- 精简运行时与自制“标准库”
  - 自带 `stdio.asm` 与 `string.c` 等（`kernel/src`），内核以 freestanding 方式构建（无 libc++/libstdc++）
- 可在 Bochs 下运行与通过 GDB 远程调试
  - 仓库提供 `bochsrc.disk` 与 `bochsrc-gdb.disk` 调试配置

---

## 总览：启动与执行流程

1) BIOS 加载 MBR（LBA 0）→ `mbr.asm` 使用 0x1F0 端口从硬盘继续读取 Loader（默认 4 个扇区）到内存并跳转。
2) Loader（实模 → 保护模式）：
   - 构造 GDT，打开 A20，`CR0.PE=1` 切换保护模式
   - 构建页目录/页表：前 4MB 恒等映射 + 内核高半映射（0xC0000000 起）
   - 从磁盘读取内核 ELF，解析 Program Header，拷贝到指定虚拟地址
   - 开启分页（`CR0.PG=1`），修正 GDT 基址至高半地址，跳入内核入口
3) 内核 `start()`（`kernel/start.cpp`）:
   - `init_all()` 初始化：中断/内存/线程/PIT/键盘/TSS/系统调用/IDE/文件系统
   - 调用 C++ `main()`（`kernel/main.cpp`），当前默认运行文件系统测试 `test::t7()`

磁盘布局（以 CMake 的写入规则为准）：

- MBR：LBA 0，大小 1 扇区
- Loader：从 LBA 2 开始，大小 4 扇区（可在 `boot/CMakeLists.txt` 中看到 `add_disk_target(write_loader ..., 2, 4)`）
- 内核：从 LBA 9 开始，默认预留 300 扇区（见 `kernel/CMakeLists.txt` 中 `add_disk_target(write_kernel ..., 9, 300)`）

---

## 目录结构（节选）

```
boot/                  # MBR/Loader 汇编与构建脚本
kernel/                # 内核核心代码与模块
  src/                 # 精简 stdio/string/assert/os 等
  module/              # C++ Modules：memory/thread/filesystem/std/user 等
  test/                # 内核内置测试（当前聚焦文件系统）
device/                # 设备与驱动（keyboard/ide/time/console/ioqueue 等）
bochsrc*.disk          # Bochs 运行/调试配置（含 gdbstub）
CMakeLists.txt         # 顶层构建：定义 osbuild、磁盘写入等
```

---

## 开发环境与依赖

- 操作系统：Linux（x86_64）
- 工具链：
  - CMake（仓库使用过 4.0.1，建议使用较新的 CMake 版本）
  - GCC/Clang，需支持 C++ Modules（C++26）与 32 位编译：`-m32 -ffreestanding -nostdlib -nostdinc`
  - objcopy、nasm
  - Bochs（建议带 gdbstub 支持）与 gdb
- 32 位构建所需多架构支持（以 Debian/Ubuntu 为例）：

```bash
sudo apt update
sudo apt install -y cmake ninja-build nasm gdb bochs bochs-sdl
sudo apt install -y gcc g++ gcc-multilib g++-multilib
```

> 注意：`bochsrc*.disk` 中的 BIOS 路径默认指向 `/usr/local/bochs/share/bochs/*`。
> 若你的 Bochs 安装在系统目录（如 `/usr/share/bochs`），请自行调整 `bochsrc*.disk` 中的 `romimage` 与 `vgaromimage` 路径。

---

## 构建与产物

顶层 `CMakeLists.txt` 定义了以下关键目标：

- `mbr_bin` / `loader_bin`：生成 `mbr.bin` 与 `loader.bin`
- `kernel`：生成带符号的内核二进制（ELF）
- `kernel_stripped`：去符号版内核（写入磁盘镜像使用）
- `write_mbr` / `write_loader` / `write_kernel`：将上述二进制按扇区写入 `hd64M.img`
- `osbuild`：一次性完成上述写入（取决于 `DISK_IMAGE` 参数）

构建步骤（推荐使用 Ninja）：

```bash
# 1) 准备构建目录
cmake -S . -B cmake-build-debug -G Ninja -DCMAKE_BUILD_TYPE=Debug

# 2) 编译（会生成 bin/kernel 与 bin/kernel_stripped 等）
cmake --build cmake-build-debug -j

# 3) 准备磁盘镜像文件（首次需要，见下一节）

# 4) 将 MBR/Loader/Kernel 写入镜像
cmake --build cmake-build-debug --target osbuild
```

构建产物位置（默认）：

- 可执行/镜像：`cmake-build-debug/bin/kernel`、`cmake-build-debug/bin/kernel_stripped`
- 中间文件与库：`cmake-build-debug/lib`、子目录 `CMakeFiles` 等

---

## 准备磁盘镜像（极其重要）

Bochs 配置使用两个“扁平”磁盘镜像：

- `hd64M.img`：主盘（含 MBR/Loader/Kernel），由 CMake 的写入目标覆盖指定扇区
- `hd80M.img`：从盘（用于分区/文件系统实验），不会由 CMake 自动创建

首次运行前，请在仓库根目录创建这两个镜像（尺寸与 `bochsrc*.disk` 一致）：

```bash
# 在仓库根目录执行
dd if=/dev/zero of=hd64M.img bs=1M count=64
dd if=/dev/zero of=hd80M.img bs=1M count=80
```

随后执行 `osbuild` 目标会自动将 `mbr.bin`/`loader.bin`/`kernel_stripped` 写入 `hd64M.img` 的指定扇区。

---

## 运行（Bochs）

普通运行（不启用 GDB）：

```bash
bochs -qf bochsrc.disk
```

带 GDB 调试运行（推荐开发时使用）：

```bash
bochs -qf bochsrc-gdb.disk
```

> 若提示 BIOS 路径问题，请根据你的安装位置编辑 `bochsrc*.disk` 中的 `romimage`/`vgaromimage` 路径。

---

## 调试（GDB + Bochs gdbstub）

`bochsrc-gdb.disk` 已开启 gdbstub：`gdbstub: enabled=1, port=1234`。

典型调试会话：

```bash
# 1) 启动带 gdbstub 的 Bochs
bochs -qf bochsrc-gdb.disk

# 2) 另开一个终端，加载带符号内核并连接 gdbstub
gdb cmake-build-debug/bin/kernel \
  -ex 'set disassemble-next-line on' \
  -ex 'target remote :1234' \
  -ex 'continue'
```

说明：

- 镜像中加载的是 `kernel_stripped`，但我们让 GDB 读取 `cmake-build-debug/bin/kernel` 以获得符号。
- 链接选项指定了入口与加载地址（`-Ttext,0xC0001500`、`-e,kkkzbh`），Bochs/gdbstub 会报告 PC 所在的高半地址，GDB 能正确解析。

常用断点位置建议：

- Loader 阶段（Bochs 内部调试器断点或查看寄存器/内存）
- 内核入口 `kkkzbh` / `start` / `init_all`
- 关键初始化点：`idt_init`、`ide_init`、`filesystem_init`、测试函数 `test::t7`

---

## 关键模块速览

- 引导与加载器
  - `boot/mbr.asm`：清屏+签名后，从 LBA 读取 Loader 并跳转
  - `boot/loader.asm`：获取内存布局、打开 A20、加载 GDT、分页、加载内核 ELF、进入高半内核
- 中断与系统
  - `kernel/interrupt.*` 定义 IDT、中断开关/注册框架
  - `kernel/include/global.h` 定义 GDT/IDT/TSS/特权级/段选择子/标志位属性等常量与结构
- 设备与驱动
  - 键盘：`device/keyboard/keyboard.c`（扫描码表与状态机，注册到中断向量 0x21）
  - IDE：`device/ide/*`（通道初始化/识别/分区扫描/中断处理/打印分区信息）
  - 时间：`device/time/*`（PIT/时间工具）
- 文件系统
  - 初始化与自动挂载：`kernel/module/filesystem/init.cpp`（魔数 `0x19590318`）
  - 目录/文件/路径/超级块/索引结点等：`kernel/module/filesystem/*`
  - 测试：`kernel/test/filesystem.cpp`（示例：创建/写入/读取/遍历/删除/统计等）
- 线程/内存/TSS/系统调用
  - `kernel/init.cpp` 中 `init_all()` 按顺序调用相关 init 例程
- 自制“标准库”与工具
  - `kernel/src/stdio.asm`、`kernel/src/string.c`、`kernel/src/assert.c` 等

---

## 如何修改/扩展

- 调整磁盘写入扇区：修改 `CMakeLists.txt` 中的 `add_disk_target(write_* ...)` 参数
- 修改内核加载地址或入口：编辑 `kernel/CMakeLists.txt` 中的 `target_link_options(kernel ...)`
- 新增设备驱动：在 `device/` 下添加库与 `target_link_libraries` 依赖
- 新增/调整文件系统能力：在 `kernel/module/filesystem/` 下扩展实现，并在 `filesystem_init()` 中挂载/格式化策略

---

## 常见问题与排错

- 构建时报 32 位相关错误（找不到 32 位 crt 或头文件）
  - 需安装 `gcc-multilib g++-multilib`，并确认 `-m32` 能正常编译
- 运行 Bochs 报 BIOS 路径错误
  - 根据你本机 Bochs 安装位置，调整 `bochsrc*.disk` 的 `romimage`/`vgaromimage` 路径
- Bochs 启动后黑屏/重启
  - 优先查看 `bochs*.out` 与 `bochs_debugger.txt`，确认 Loader 是否正确读取内核（关注 LBA）
  - 检查 `osbuild` 是否已将二进制写入 `hd64M.img` 正确扇区（LBA0/2/9 等）
- GDB 无法连接
  - 确认使用 `bochsrc-gdb.disk` 启动，且端口 `1234` 未被占用
- 文件系统未挂载/找不到分区
  - 查看 `ide_init` 与 `filesystem_init` 的日志，确认 `hd80M.img` 已创建并存在有效（或将被格式化）的分区

---

## 开发与测试

- 运行内置测试：当前 `kernel/main.cpp` 默认调用 `test::t7()`（查看根与 `/kkkzbh` 目录的 `stat/ls` 输出）
- 你可以在 `kernel/test/filesystem.cpp` 中切换至 `t1`~`t6` 等其他测试项，重新构建并运行观察行为

---

## 贡献与许可

- 欢迎提交 Issue/PR 交流内核/文件系统/驱动实现思路与优化建议
- License：未指定（如需开放授权，请在仓库根目录添加 `LICENSE` 文件）

---

## 参考信息（文件与配置）

- 构建总控：`CMakeLists.txt` 顶层（`add_disk_target`、`osbuild`、输出目录等）
- 引导构建：`boot/CMakeLists.txt`（生成 `mbr.bin`、`loader.bin` 并写入镜像）
- 内核构建：`kernel/CMakeLists.txt`（`kernel` → `kernel_stripped` → `write_kernel`）
- Bochs 配置：`bochsrc.disk`、`bochsrc-gdb.disk`
- 入口与初始化：`kernel/main.c`（入口符号 `kkkzbh`）→ `kernel/start.cpp` → `kernel/init.cpp`

祝玩得开心，hack the kernel!
