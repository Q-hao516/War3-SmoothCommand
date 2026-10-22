; ============================================================
;  魔兽争霸III 流畅指挥 —— 安装脚本
;  用 Inno Setup 6 编译：ISCC.exe War3Unlock.iss
; ============================================================

#define MyAppName "魔兽争霸III 流畅指挥"
#define MyAppVer  "1.0.0"
#define MyAppId  "{{8F3C2E51-7A64-4C1B-9E2D-5B7A1C4E9D30}"

[Setup]
AppId={#MyAppId}
AppName={#MyAppName}
AppVersion={#MyAppVer}
AppVerName={#MyAppName} {#MyAppVer}
VersionInfoVersion={#MyAppVer}
AppPublisher=Local Build
; {app} 直接就是魔兽目录：用标准目录页承载，避免 “app 常量未初始化” 报错
DefaultDirName={code:DetectWc3Dir}
DisableDirPage=no
DisableProgramGroupPage=yes
AllowNoIcons=yes
PrivilegesRequired=admin
OutputDir=dist
OutputBaseFilename=War3_SmoothCommand_Setup
Compression=lzma2/max
SolidCompression=yes
WizardStyle=modern
UninstallDisplayName={#MyAppName}
MinVersion=6.1sp1

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Messages]
SelectDirLabel3=安装程序将把「流畅指挥」写入下面这个文件夹。请选择魔兽争霸III的根目录（目录中应包含 War3.exe）。
SelectDirBrowseLabel=点击“下一步”继续。若路径不对，可点“浏览”重新选择魔兽争霸III的根目录。
SetupAppTitle=安装程序
SetupWindowTitle=安装 - %1
ButtonNext=下一步(&N)
ButtonBack=上一步(&B)
ButtonCancel=取消
ButtonInstall=安装(&I)
ButtonFinish=完成(&F)
ButtonBrowse=浏览(&R)
WelcomeLabel1=欢迎使用 魔兽争霸III 流畅指挥
WelcomeLabel2=本程序将为你的魔兽争霸III 安装：%n%n  · 流畅指挥（多单位操控不卡顿，支持 1.24b / 1.24e / 1.25b / 1.26 / 1.27a）%n  · 装好之后直接用游戏本来的图标启动即可%n%n点“下一步”继续。

[Files]
; 启动脚本放游戏根目录，方便直接双击
Source: "payload\Start-War3-Unlock.bat";     DestDir: "{app}";       Flags: ignoreversion
; 补丁与注入器（单 DLL：自动识别 1.24b / 1.24e / 1.25b / 1.26 / 1.27a）
Source: "payload\SlotPatch.dll";             DestDir: "{app}\patch"; Flags: ignoreversion
Source: "payload\CavInject.exe";            DestDir: "{app}\patch"; Flags: ignoreversion
Source: "payload\Launch-War3.ps1";          DestDir: "{app}\patch"; Flags: ignoreversion
Source: "payload\说明.txt";                  DestDir: "{app}\patch"; Flags: ignoreversion
; 代理 DLL：替换 ijl15.dll，使直接双击 War3.exe 也能生效（原版会被改名为 ijl15_orig.dll）
; 代理 DLL：dontcopy 提取到临时目录，再由 [Code] 复制为 ijl15.dll，避免进入卸载清单
Source: "payload\ijl15_proxy.dll";          Flags: dontcopy

[Icons]
; 不再创建桌面快捷方式（代理 DLL 就位后，直接用游戏自己的图标启动即可）
Name: "{autoprograms}\魔兽争霸III"; Filename: "{app}\War3.exe"; WorkingDir: "{app}"

[Run]
Filename: "{app}\War3.exe"; Description: "立即启动魔兽"; Flags: postinstall nowait skipifsilent unchecked

[UninstallDelete]
Type: files; Name: "{app}\Start-War3-Unlock.bat"
Type: files; Name: "{app}\patch\SlotPatch.dll"
Type: files; Name: "{app}\patch\CavInject.exe"
Type: files; Name: "{app}\patch\Launch-War3.ps1"
Type: files; Name: "{app}\patch\说明.txt"
Type: files; Name: "{app}\patch\SlotPatch.log"
Type: files; Name: "{app}\patch\PathParallel.log"
Type: files; Name: "{app}\PathParallel.log"

[Code]
function TryDir(const D: String): Boolean;
begin
  Result := (D <> '') and FileExists(AddBackslash(D) + 'War3.exe');
end;

function DetectWc3Dir(Param: String): String;
var
  S: String;
  I, J: Integer;
  Keys: array[0..2] of String;
  Vals: array[0..3] of String;
  Cand: array[0..7] of String;
begin
  Result := '';
  Keys[0] := 'SOFTWARE\Blizzard Entertainment\Warcraft III';
  Keys[1] := 'SOFTWARE\WOW6432Node\Blizzard Entertainment\Warcraft III';
  Keys[2] := 'SOFTWARE\Blizzard Entertainment\Warcraft III\Capabilities';
  Vals[0] := 'InstallPath';
  Vals[1] := 'Program';
  Vals[2] := 'War3Path';
  Vals[3] := 'Path';

  for I := 0 to 2 do
    for J := 0 to 3 do
      if (Result = '') and RegQueryStringValue(HKLM, Keys[I], Vals[J], S) and TryDir(S) then
        Result := S;
  for I := 0 to 2 do
    for J := 0 to 3 do
      if (Result = '') and RegQueryStringValue(HKCU, Keys[I], Vals[J], S) and TryDir(S) then
        Result := S;

  Cand[0] := 'C:\Program Files (x86)\Warcraft III';
  Cand[1] := 'C:\Program Files\Warcraft III';
  Cand[2] := 'C:\Warcraft III';
  Cand[3] := 'D:\Warcraft III';
  Cand[4] := 'D:\War3';
  Cand[5] := 'E:\Warcraft III';
  Cand[6] := 'E:\War3';
  Cand[7] := 'F:\Warcraft III';
  if Result = '' then
    for I := 0 to 7 do
      if (Result = '') and TryDir(Cand[I]) then
        Result := Cand[I];

  if Result = '' then
    Result := 'C:\Program Files (x86)\Warcraft III';
end;

function NextButtonClick(CurPageID: Integer): Boolean;
var
  D, V: String;
begin
  Result := True;
  // 静默安装不做路径校验，避免无人交互时中止
  if WizardSilent then Exit;
  if CurPageID = wpSelectDir then
  begin
    D := WizardDirValue;
    if Trim(D) = '' then
    begin
      MsgBox('请先选择魔兽争霸III的根目录。', mbError, MB_OK);
      Result := False;
      Exit;
    end;
    if not FileExists(AddBackslash(D) + 'War3.exe') then
    begin
      MsgBox('在所选目录下找不到 War3.exe。' + #13#10 +
             '请选择魔兽争霸III的根目录（不是 Maps 或其它子目录）。', mbError, MB_OK);
      Result := False;
      Exit;
    end;
    if not FileExists(AddBackslash(D) + 'Game.dll') then
    begin
      if MsgBox('所选目录下没有 Game.dll，可能不是完整的游戏目录。' + #13#10 + '仍要继续吗？',
                mbConfirmation, MB_YESNO) = IDNO then
      begin
        Result := False;
        Exit;
      end;
    end
    else if GetVersionNumbersString(AddBackslash(D) + 'Game.dll', V) then
    begin
      if (Pos('1.24', V) = 0) and (Pos('1.25', V) = 0) and
         (Pos('1.26', V) = 0) and (Pos('1.27', V) = 0) then
        MsgBox('注意：检测到 Game.dll 版本为 ' + V + '。' + #13#10 +
               '「流畅指挥」适配 1.24b / 1.24e / 1.25b / 1.26 / 1.27a。' + #13#10 +
               '其它版本仍可安装，但不会生效，也不会弄坏游戏。',
               mbInformation, MB_OK);
    end;
  end;
end;

{ ==================================================================
  代理 DLL：让「直接双击 War3.exe / 原版快捷方式」也能生效

  安装时把原版 ijl15.dll 改名为 ijl15_orig.dll，
  再由 [Files] 把 payload 里的代理 DLL 放置为 ijl15.dll。
  代理只做导出转发（序号 1..6 与原版一致），并加载 patch\SlotPatch.dll。
  ================================================================== }
procedure CurStepChanged(CurStep: TSetupStep);
var
  OrigFile, ProxyFile: String;
begin
  if CurStep = ssInstall then
  begin
    OrigFile  := ExpandConstant('{app}\ijl15_orig.dll');
    ProxyFile := ExpandConstant('{app}\ijl15.dll');
    if FileExists(ProxyFile) and (not FileExists(OrigFile)) then
    begin
      if not RenameFile(ProxyFile, OrigFile) then
      begin
        MsgBox('无法把 ' + ProxyFile + ' 重命名为 ijl15_orig.dll。' + #13#10 +
               '「直接用游戏图标启动」将不可用，但启动脚本方式仍可正常工作。',
               mbError, MB_OK);
        Exit;
      end;
    end;
    { 从安装包临时目录取出代理 DLL，复制为 ijl15.dll（不进入卸载清单） }
    if FileExists(OrigFile) and (not FileExists(ProxyFile)) then
    begin
      ExtractTemporaryFile('ijl15_proxy.dll');
      if not FileCopy(ExpandConstant('{tmp}\ijl15_proxy.dll'), ProxyFile, False) then
        MsgBox('复制代理 ijl15.dll 失败：' + ProxyFile + #13#10 +
               '「直接用游戏图标启动」将不可用，但启动脚本方式仍可正常工作。',
               mbError, MB_OK);
    end;
  end;
end;

{ 卸载时把原版 ijl15.dll 恢复回去 }
procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
var
  OrigFile, ProxyFile: String;
begin
  if CurUninstallStep = usUninstall then
  begin
    OrigFile  := ExpandConstant('{app}\ijl15_orig.dll');
    ProxyFile := ExpandConstant('{app}\ijl15.dll');
    if FileExists(OrigFile) then
    begin
      if FileExists(ProxyFile) then DeleteFile(ProxyFile);
      RenameFile(OrigFile, ProxyFile);
    end;
  end;
end;
