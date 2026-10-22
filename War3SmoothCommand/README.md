# War3 流畅指挥（SmoothCommand）

魔兽争霸 III 1.24b / 1.24e / 1.25b / 1.26 / 1.27a 的**槽位散列补丁**。
不改游戏文件（除代理 DLL 外），纯内存补丁，可随时卸载还原。

---

## 目录结构

```
War3SmoothCommand/
├─ build.bat                 一键编译（需要 Visual Studio + C++ 工作负载）
├─ package.bat               编译 + 同步到 payload + 可选打包安装程序
├─ src/
│  ├─ slotpatch/
│  │   └─ SlotPatch.cpp      ★ 核心：槽位散列补丁（注入 Game.dll）
│  ├─ proxy/
│  │   └─ ijl15_proxy.cpp    ijl15 代理 DLL（让直接双击 War3.exe 也生效）
│  └─ injector/
│      └─ CavInject.cpp      便携注入器（启动游戏 + 注入 DLL）
├─ installer/
│  ├─ War3Unlock.iss         Inno Setup 脚本
│  ├─ pack_zip.ps1           便携版 zip 打包脚本
│  └─ payload/               安装包要释放的文件（脚本 + 编译产物）
├─ build/                    编译输出（git 忽略）
└─ docs/
   └─ DESIGN.md              原理与逆向说明
```

---

## 快速开始

### 1. 编译

双击 `build.bat`，产物在 `build\`：

| 文件 | 说明 |
|---|---|
| `SlotPatch.dll` | 槽位散列补丁本体 |
| `ijl15_proxy.dll` | 代理 DLL（转发 ijl15 导出 + 加载 SlotPatch） |
| `CavInject.exe` | 便携注入器 |

要求：Visual Studio 2019/2022/2026（任意版本，需「使用 C++ 的桌面开发」工作负载）。
脚本会自动用 `vswhere` 或常见路径定位 `vcvarsall.bat`。

### 2. 打包安装程序（可选）

双击 `package.bat`。它会先编译、把产物同步进 `installer\payload\`，再调用 Inno Setup 6 的
`ISCC.exe` 生成 `installer\dist\War3_SmoothCommand_Setup.exe`。
未安装 Inno Setup 时会跳过打包，只做编译与同步。

### 3. 手动使用（不打包）

把 `build\` 里的三个文件，和 `installer\payload\` 里的脚本一起放进游戏目录：

```
<游戏目录>\
├─ Start-War3-Unlock.bat        ← 双击这个启动
├─ ijl15.dll                    ← 用 build\ijl15_proxy.dll（原版改名为 ijl15_orig.dll）
└─ patch\
   ├─ SlotPatch.dll             ← build\SlotPatch.dll
   ├─ CavInject.exe             ← build\CavInject.exe
   ├─ Launch-War3.ps1
   └─ 说明.txt
```

---

## 原理（简述）

引擎把「移动微步」放进按槽位组织的 FIFO 队列，槽号原本取
`(flags >> 16) & 0xF` —— **同一玩家共用 1 个槽，每 tick 只放行队头 1 个**。
本补丁把槽号计算改成 `(path >> 12) & 0xF`，让不同 path 散列到 16 个槽并行。

补丁点按 `Game.dll` 版本用**签名匹配**定位（不是版本字符串），所以同版本的
官方安装版 / 绿色版都能用。打补丁前逐字节校验原值，不匹配就整体跳过，绝不盲写。

详见 `docs/DESIGN.md`。

---

## 环境变量（调试用，正常玩不需要）

| 变量 | 作用 |
|---|---|
| `SLOT_DRY=1` | 只校验签名并写日志，**不修改内存** |

日志：`patch\SlotPatch.log`、`patch\ProxyLoad.log`

---

## 卸载

控制面板 → 程序和功能 → 卸载。安装程序会把 `ijl15.dll` 还原成 `ijl15_orig.dll`，
并删除 `patch\` 下的补丁文件。游戏本身完全不受影响。

---

## 已验证的范围

| 项目 | 结果 |
|---|---|
| 编译 | `build.bat` 三目标全部成功（MSVC x86，/O2） |
| 产物架构 | 均为 `machine=0x014C`（x86），DLL/EXE 标志正确 |
| 与发布包一致性 | `SlotPatch.dll`、`ijl15_proxy.dll` 大小完全一致 |
| 1.24e 实测 | `slot patch applied (1.24e): 4/4 sites` |

> `CavInject.exe` 体积与发布包不同（121,344 vs 105,415 字节），原因是发布包用
> MinGW 编译、本工程用 MSVC。功能等价（源码同一份），不影响使用。

---

## 已知限制

- **寻路并行不是瓶颈**：实测 pathing tick 平均约 0.9 ms、约 30 次/秒，
  合计约 3% CPU。本补丁的收益来自减少同槽排队，而不是并行寻路。
- **秒级卡顿来自路径请求突发**：当数千单位同时收到指令时，引擎在单帧内
  为它们全部寻路，会造成 1~5 秒卡顿。这与渲染方式无关，补丁层无法根治。
