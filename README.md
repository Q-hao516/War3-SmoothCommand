# Warcraft III · Smooth Command

<p align="center">
  <img src="https://img.shields.io/badge/game%20version-1.24b%20~%201.27a-8a2be2" alt="Supported game versions 1.24b ~ 1.27a">
  <img src="https://img.shields.io/badge/mode-single--player%20only-orange" alt="Single-player only">
</p>

<p align="center">
  <b>English</b> · <a href="./README.zh.md">中文</a>
</p>

**Select a large group of units and order them across the map. They used to start trickling out over several seconds; now they respond much sooner.**

**A single unit command with 240 units already moving: latency drops from 4.2 s to 0.3 s.**

> ⚠️ Single-player only. Do **not** use it on Battle.net or any online/ladder game.

Supported versions: **1.24b / 1.24e / 1.25b / 1.26 / 1.27a** · Experimental · Windows

---

## Contents

- [What it does](#what-it-does)
- [Measured results](#measured-results)
- [Install](#install)
- [How to verify](#how-to-verify)
- [Source code](#source-code)
- [When not to use it](#when-not-to-use-it)
- [FAQ](#faq)
- [Compliance and disclaimer](#compliance-and-disclaimer)

---

## What it does

When a large number of units move at once, the game funnels their move orders through a single queue and releases them one at a time. That is why a big attack order feels like "I clicked ages ago, and units are only now starting to move".

Smooth Command removes that funnel. It does **not** make the game faster and it does **not** make the framerate smoother — it simply lets your units start moving earlier.

## Measured results

| Scenario | Stock game | With this patch |
|---|---|---|
| Single unit command, 240 units already moving | 4.2 s | **0.3 s** |
| Mass attack order · most units start moving | 3.3 s | **1.2 s** |
| Mass attack order · last unit starts moving | 19.9 s | **10.3 s** |
| Realistic battle map (236 units) · most units start moving | 2.8 s | **0.7 s** |

Numbers come from purpose-built test maps; results from different scenarios are not interchangeable.

## Install

1. Download the release files from [Releases](https://github.com/Q-hao516/War3-SmoothCommand/releases/latest).
   Browsers may warn that a `.exe` is "not trusted", so a `_portable.zip` is published alongside it.
   Unpack it — it contains the installer plus a `安装说明.txt` walkthrough (in Chinese).
2. Run the installer and point the wizard at your **Warcraft III root directory** (the folder that contains `War3.exe`).
3. After setup, just launch the game with your normal shortcut — nothing else to run.

> Setup replaces one file inside the game folder (the original is backed up and restored on uninstall). Your game itself is never modified.

## How to verify

- The simplest check: open a map with many units, select a big group, order them across the map, and see whether they react much sooner.
- No difference? Quit the game **completely**, then start it again.

## Source code

The complete, buildable source is published in [`War3SmoothCommand/`](./War3SmoothCommand):

| Path | What it is |
|---|---|
| `src/slotpatch/SlotPatch.cpp` | The patch itself — version-dispatched byte patches, ~400 lines |
| `src/proxy/ijl15_proxy.cpp` | The `ijl15.dll` proxy that auto-loads the patch, so the normal shortcut works |
| `src/injector/CavInject.cpp` | The portable launcher/injector (`-game` / `-dll` / `-map`) |
| `installer/War3Unlock.iss` | Inno Setup script used to build the installer |
| `docs/DESIGN.md` | How the queue works, and what was actually measured |

Build with `build.bat` (needs Visual Studio with the "Desktop development with C++" workload);
`package.bat` additionally produces the installer EXE if Inno Setup 6 is installed.
No Blizzard binaries are included — you build against your own game installation.

## When not to use it

> In the cases below, please simply do not use it.

- **Battle.net, ladder, or any online match.** It modifies game memory at runtime, which such platforms may treat as tampering.
- **Watching or studying replays.** Replays made with this patch will not match the stock game.
- **If you want a "fair" match.** AI-controlled units also get faster.

## FAQ

**Q1: Can I get banned?**
It does not go online by itself, but platforms may flag memory modification as tampering. **There is a risk, so do not use it there.**

**Q2: Which game versions are supported?**
1.24b / 1.24e / 1.25b / 1.26 / 1.27a. A single installer detects the version by itself. Other versions can be installed safely but the patch will not apply.

**Q3: It does not seem to work.**
Quit Warcraft III completely and launch it again. If it still does nothing, start the game with `Start-War3-Unlock.bat` in the Warcraft III folder.

**Q4: Will spamming move orders make units faster?**
No — it makes them slower. Repeating an order puts the unit back in the queue.

**Q5: How many units can I handle?**
500–700 units feels comfortable; around 1000 units the queueing becomes noticeable again.

**Q6: How do I uninstall?**
Control Panel → Programs and Features → find this entry. It restores the files changed during setup and leaves the game itself untouched.

**Q7: Can it crash the game?**
This is an experimental tool. Complex terrain, complex abilities, and multiplayer maps have not been thoroughly validated — evaluate for yourself.

## Compliance and disclaimer

- This repository and its release **do not contain or provide** any Blizzard Entertainment binaries (`Game.dll`, `War3.exe`, `*.mpq`, etc.), nor links to download them.
- Warcraft III and related assets are the property of Blizzard Entertainment. This project is not affiliated with, endorsed by, or sponsored by Blizzard Entertainment.
- **Single-player use only, at your own risk** (possible crashes, save/replay anomalies, or platform tampering flags).
- If a rights holder believes this project infringes their rights, please open an Issue and we will take the relevant resources down immediately.
