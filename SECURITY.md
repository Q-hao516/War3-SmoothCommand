# 安全说明 / Security Policy

## 这个项目做什么

War3-SmoothCommand 是一个**单机游戏补丁**：它通过注入《魔兽争霸III》
进程、修改单位命令队列的相关内存，让大批单位能更早响应移动指令。

它**仅适用于单机**，不适用于对战平台或任何联机对战。

## 为什么杀毒软件可能报警

本项目的组件需要读写游戏进程内存，这是补丁生效的唯一方式，
同时也符合"内存注入类工具"的通用特征，因此可能触发
SmartScreen 或杀毒软件的启发式扫描误报。

项目本身：

- 不含任何网络通信代码
- 不含后门、木马或数据收集行为
- 不需要常驻后台，游戏退出后即结束

## 如果你怀疑这个项目

你可以自行验证：

- 仓库中随发布提供补丁源码（`SlotPatch.cpp`、`ijl15_proxy.cpp`）
- Release 中提供 `SHA256SUMS.txt`，可核对下载文件是否与发布一致
- 项目不含任何暴雪娱乐的二进制文件

PowerShell 核对方法：

```powershell
Get-FileHash War3_SmoothCommand_Setup.exe -Algorithm SHA256
```

## 第三方质询 / 联系方式

如果你是安全厂商、平台方或任何第三方，认为本仓库存在滥用行为，
**请先通过 Issue 联系维护者**，我们会响应并配合处理：

https://github.com/Q-hao516/War3-SmoothCommand/issues

如确有问题，我们会立即下架相关资源。

---

## English

### What this project does

War3-SmoothCommand is a **single-player game patch**. It injects into the
Warcraft III process and modifies the in-memory unit command queue so that
large groups of units respond to move orders sooner.

**Single-player use only.** Do not use it on Battle.net or in any online match.

### Why antivirus software may flag it

The patch needs to read and write game process memory — that is the only way
it can work, and it matches the generic profile of memory-injection tools,
which can trigger SmartScreen or heuristic-scanning false positives.

The project itself:

- contains no network communication code
- contains no backdoor, trojan, or data collection
- does not stay resident after the game exits

### How to verify

- Patch source is published with each release (`SlotPatch.cpp`, `ijl15_proxy.cpp`)
- Each release includes `SHA256SUMS.txt` so you can verify your download
- No Blizzard Entertainment binaries are included

```powershell
Get-FileHash War3_SmoothCommand_Setup.exe -Algorithm SHA256
```

### Third-party inquiries

If you are a security vendor, platform operator, or any third party who
believes this repository is being abused, **please open an Issue first** and
we will respond and cooperate:

https://github.com/Q-hao516/War3-SmoothCommand/issues

If there is a genuine problem, we will take the relevant resources down
immediately.
