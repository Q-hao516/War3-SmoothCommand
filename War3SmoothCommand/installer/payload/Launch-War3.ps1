# -*- coding: utf-8 -*-
# 魔兽争霸III 启动器：帧率解锁 + 流畅指挥
# 由 Start-War3-Unlock.bat 调用；也可以单独运行本脚本。
#
# 补丁只有一个 SlotPatch.dll，它自己按 Game.dll 版本分派：
#   1.24e  1.24.4.6387   -> 4 处槽位补丁
#   1.27a  1.27.0.52240  -> 8 处槽位补丁
$ErrorActionPreference = 'Continue'

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path   # <游戏目录>\patch
$root      = Split-Path -Parent $scriptDir                      # <游戏目录>
$hook      = Join-Path $scriptDir 'SlotPatch.dll'
$inj       = Join-Path $scriptDir 'CavInject.exe'
$log       = Join-Path $scriptDir 'SlotPatch.log'

function Say($m) { Write-Host $m }

Say ''
Say '============================================================'
Say '  魔兽争霸III   帧率解锁 + 流畅指挥'
Say '============================================================'
Say ''

if (-not (Test-Path (Join-Path $root 'War3.exe'))) {
    Say "  [错误] 在 $root 下找不到 War3.exe。"
    Say '         请重新运行安装程序并选择正确的魔兽争霸III根目录。'
    Read-Host '按回车退出'
    exit 1
}
if (-not (Test-Path $hook)) {
    Say '  [错误] 缺少 patch\SlotPatch.dll，请重新运行安装程序。'
    Read-Host '按回车退出'
    exit 1
}
if (-not (Test-Path $inj)) {
    Say '  [错误] 缺少 patch\CavInject.exe，请重新运行安装程序。'
    Read-Host '按回车退出'
    exit 1
}

# ---- 识别 Game.dll 版本 ----
$ver = ''
$gdll = Join-Path $root 'Game.dll'
if (Test-Path $gdll) {
    try { $ver = (Get-Item $gdll).VersionInfo.FileVersion } catch { $ver = '' }
}
$supported = $false
if ($ver -match '1,?\s*24,\s*4' -or $ver -match '1\.24\.4') { $supported = $true }
elseif ($ver -match '1,?\s*27,\s*0' -or $ver -match '1\.27\.0') { $supported = $true }

Say "  检测到 Game.dll 版本：$ver"
if ($supported) {
    Say '  （该版本受「流畅指挥」支持）'
} else {
    Say ''
    Say '  [警告] 该版本不在支持列表内。'
    Say '         「流畅指挥」只适配 1.24e（1.24.4.6387）与 1.27a（1.27.0.52240）。'
    Say '         游戏仍可启动，但补丁不会生效。'
}
Say ''

# 只把“有游戏窗口”的进程视为正在运行，避免被残留的无窗口僵死进程误拦
$running = @(Get-Process War3 -ErrorAction SilentlyContinue | Where-Object { $_.MainWindowHandle -ne 0 })
if ($running.Count -gt 0) {
    Say '  [提示] 检测到魔兽正在运行，请先完全退出游戏再运行本脚本。'
    Read-Host '按回车退出'
    exit 1
}

# ---- 注入所需的环境变量 ----
$env:PATH_FPSMIX = '1'   # 让 SlotPatch 尝试加载游戏根目录的 FPSUnlocker.mix
foreach ($k in 'PATH_HOOK','PATH_SLOT','PATH_RLS','PATH_FPS','PATH_AV','PATH_WQ','PATH_TM','PATH_PH','PATH_RET','PATH_BR','PATH_STAT','PATH_FD','PATH_SFD') {
    Remove-Item "Env:$k" -ErrorAction SilentlyContinue
}

if (Test-Path $log) { Remove-Item $log -Force -ErrorAction SilentlyContinue }

Say '  正在启动游戏...'
$out = & $inj -game $root -dll $hook 2>&1 | Out-String
$rc  = $LASTEXITCODE
if ($out.Trim()) { $out.Trim() -split "`r?`n" | ForEach-Object { Say ('  ' + $_) } }

if ($rc -ne 0) {
    Say ''
    Say "  [错误] 启动失败，错误码 $rc"
    if ($rc -eq 4) { Say '         补丁库未能加载，请确认 patch\SlotPatch.dll 未被杀毒软件拦截。' }
    Read-Host '按回车退出'
    exit $rc
}

# ---- 等待并自动校验 ----
Say ''
Say '  正在校验补丁状态...'
$fpsOk = $false; $slotOk = $false; $unsupported = $false; $menuWait = $false
for ($i = 0; $i -lt 30; $i++) {
    Start-Sleep -Milliseconds 500
    if (Test-Path $log) {
        $txt = Get-Content $log -Raw -ErrorAction SilentlyContinue
        if ($txt -match 'slot patch applied') { $slotOk = $true }
        if ($txt -match 'no profile matched') { $unsupported = $true }
        if ($txt -match 'fpsmix LoadLibrary\(.*\)=1?[0-9A-Fa-f]{6,}\s+err=0') { $fpsOk = $true }
        if ($txt -match 'SlotPatch\.dll loaded' -and $txt -notmatch 'attached') { $menuWait = $true }
        if ($slotOk -and $fpsOk) { break }
        if ($unsupported) { break }
    }
}

Say ''
if ($slotOk) { Say '  [OK] 流畅指挥    已生效' }
elseif ($unsupported) { Say '  [--] 流畅指挥    该 Game.dll 版本不受支持，未打补丁' }
elseif ($menuWait) {
    Say '  [..] 流畅指挥    已注入，等待进入地图'
    Say '                   （Game.dll 在地图加载时才载入，进入一张地图后自动生效）'
}
else { Say '  [--] 流畅指挥    未确认（可完全退出游戏后再启动一次）' }
if ($fpsOk)  { Say '  [OK] 帧率解锁    已生效（游戏内输入 /fps 可查看帧率）' }
elseif ($menuWait) { Say '  [..] 帧率解锁    将随地图加载一并生效' }
else { Say '  [--] 帧率解锁    未确认（请确认游戏根目录存在 FPSUnlocker.mix，或上游无许可未随包提供）' }
Say ''
Say '  提示：进入地图后可再查看 patch\SlotPatch.log 确认补丁已打上。'
Say '  游戏窗口已打开，本窗口 6 秒后自动关闭。'
Start-Sleep -Seconds 6
exit 0
