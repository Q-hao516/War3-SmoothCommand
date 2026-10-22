# 打包「便携版 zip」—— 供 GitHub Release 使用
#
# 为什么要 zip：
#   Edge / Chrome 直接下载未签名的 .exe 会被 SmartScreen 拦（"不受信任"），
#   而 zip 不触发该检查。用户解压后按里面的说明操作即可。
#
# 产物：installer\dist\War3_SmoothCommand_<版本>_portable.zip
#   ├─ War3_SmoothCommand_Setup.exe   安装包（和单独发布的是同一个文件）
#   ├─ 安装说明.txt                    图文步骤 + 校验方法 + 卸载方法
#   └─ SHA256SUMS.txt                  安装包哈希，供用户核对

param(
    [string]$Version = '1.0.0'
)

$ErrorActionPreference = 'Stop'
$root = (Get-Location).Path
$dist = Join-Path $root 'installer\dist'
$exe  = Join-Path $dist ("War3_SmoothCommand_Setup.exe")

if (-not (Test-Path $exe)) {
    Write-Error "找不到安装包：$exe"
    exit 1
}

$stage = Join-Path $root "tmp_build\zip_stage_$Version"
Remove-Item $stage -Recurse -Force -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force $stage | Out-Null

# 1) 安装包本体（保持原名，便于用户识别）
Copy-Item $exe (Join-Path $stage 'War3_SmoothCommand_Setup.exe')

# 2) 安装说明
Copy-Item (Join-Path $root 'installer\payload\zip_安装说明.txt') (Join-Path $stage '安装说明.txt')

# 3) 哈希清单
$h = Get-FileHash $exe -Algorithm SHA256
$lines = @(
    '# War3 流畅指挥 安装包 SHA-256 校验值',
    '#',
    '# PowerShell 校验方法：',
    '#   Get-FileHash War3_SmoothCommand_Setup.exe -Algorithm SHA256',
    '# 与下面的值一致即文件完好。',
    '',
    ('{0}  War3_SmoothCommand_Setup.exe' -f $h.Hash.ToLower())
)
Set-Content -Path (Join-Path $stage 'SHA256SUMS.txt') -Value $lines -Encoding UTF8

# 4) 打包
$zipName = "War3_SmoothCommand_${Version}_portable.zip"
$zipPath = Join-Path $dist $zipName
if (Test-Path $zipPath) { Remove-Item $zipPath -Force }
Compress-Archive -Path (Join-Path $stage '*') -DestinationPath $zipPath -CompressionLevel Optimal

Remove-Item $stage -Recurse -Force -ErrorAction SilentlyContinue

Write-Output '=== 打包完成 ==='
$z = Get-Item $zipPath
"文件   : $($z.Name)"
"大小   : {0:N2} MB" -f ($z.Length / 1MB)
"SHA-256: $((Get-FileHash $z.FullName -Algorithm SHA256).Hash)"
Write-Output '=== 内容清单 ==='
Add-Type -AssemblyName System.IO.Compression.FileSystem
$arc = [System.IO.Compression.ZipFile]::OpenRead($zipPath)
try {
    $arc.Entries | Sort-Object FullName | ForEach-Object {
        "  {0,-32} {1,9:N0} B" -f $_.FullName, $_.Length
    }
}
finally { $arc.Dispose() }
