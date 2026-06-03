# knix

`knix` 是一个从引导扇区一路写到用户态 Shell 的 32-bit x86 实验操作系统。

它把一条操作系统主链路完整接了起来：从 MBR、Loader、内核初始化，到中断、内存、线程、进程、文件系统、系统调用和用户程序执行。

![knix shell running cat in Bochs](assets/readme/cat-demo.png)

## 项目现在能看到什么

| Shell 交互 | 文件系统测试 |
| --- | --- |
| ![Shell builtin commands](assets/readme/shell-builtins.png) | ![Filesystem test run](assets/readme/filesystem-test.png) |

## 这个项目的重点

`knix` 的重点很简单：把一个最小但真实的操作系统跑起来，并且让启动、Shell、进程和文件系统这些环节都能实际观察到。

- 能从裸磁盘镜像启动，经过 `MBR -> Loader -> Kernel` 进入内核。
- 能完成保护模式、分页、中断、时钟、键盘、线程调度等基础初始化。
- 能在用户态运行 Shell，并通过系统调用访问内核能力。
- 能创建目录、读写文件、遍历目录，并运行文件系统回归场景。
- 能执行用户程序，覆盖 `fork / exec / wait / exit` 这一类进程生命周期。

![Process list in knix shell](assets/readme/process-list.png)

## 快速体验

当前仓库的构建入口是 `CMake + Ninja`。本机建议使用 CLion Toolbox 附带的 CMake：

```bash
/home/kkkzbh/.local/share/JetBrains/Toolbox/apps/clion/bin/cmake/linux/x64/bin/cmake \
  -S . -B build -G Ninja

/home/kkkzbh/.local/share/JetBrains/Toolbox/apps/clion/bin/cmake/linux/x64/bin/cmake \
  --build build --target osbuild

/home/kkkzbh/.local/share/JetBrains/Toolbox/apps/clion/bin/cmake/linux/x64/bin/cmake \
  --build build --target bochs
```

进入 Shell 后，可以尝试：

```text
ls
pwd
ps
mkdir /tmp
cd /tmp
```
