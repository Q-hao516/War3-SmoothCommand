// CavInject.exe — 便携注入器
// 作用：以挂起方式启动 War3.exe，把指定 DLL 注入后再恢复运行。
//
// 用法：
//   CavInject.exe [-game <魔兽目录>] [-dll <DLL路径>] [-map <地图路径>]
//   不带参数时：-game 取本程序所在目录（若本程序在 <魔兽>\patch 下，自动上溯一级）
//               -dll  取 <魔兽>\patch\PathParallel_probe.dll
//               -map  缺省 = 不加载地图，直接进入主菜单
//
// 退出码：0 成功；1 路径/参数错误；2 启动进程失败；3 注入失败；4 目标内 LoadLibrary 失败
#include <windows.h>
#include <stdio.h>
#include <string.h>

static int FileExists(const char *p)
{
    DWORD a = GetFileAttributesA(p);
    return a != INVALID_FILE_ATTRIBUTES && !(a & FILE_ATTRIBUTE_DIRECTORY);
}

static void DirOfSelf(char *out, DWORD n)
{
    GetModuleFileNameA(NULL, out, n);
    char *s = strrchr(out, '\\');
    if (s) *s = 0;
}

static int JoinFile(char *out, DWORD n, const char *dir, const char *name)
{
    if (strlen(dir) + strlen(name) + 2 > n) return 0;
    _snprintf(out, n - 1, "%s\\%s", dir, name);
    out[n - 1] = 0;
    return 1;
}

static const char *ArgValue(int argc, char **argv, const char *key)
{
    for (int i = 1; i + 1 < argc; ++i)
        if (!_stricmp(argv[i], key)) return argv[i + 1];
    return NULL;
}

int main(int argc, char **argv)
{
    char selfDir[MAX_PATH] = {0};
    DirOfSelf(selfDir, sizeof(selfDir));

    // 游戏目录：显式指定 > 本程序父目录（含 War3.exe）> 本程序目录
    char game[MAX_PATH] = {0};
    const char *g = ArgValue(argc, argv, "-game");
    if (g) {
        strncpy(game, g, sizeof(game) - 1);
    } else {
        char up[MAX_PATH] = {0};
        char cand[MAX_PATH] = {0};
        // 上溯一级
        strncpy(up, selfDir, sizeof(up) - 1);
        char *s = strrchr(up, '\\');
        if (s) *s = 0;
        if (JoinFile(cand, sizeof(cand), up, "War3.exe") && FileExists(cand)) {
            strncpy(game, up, sizeof(game) - 1);
        } else if (JoinFile(cand, sizeof(cand), selfDir, "War3.exe") && FileExists(cand)) {
            strncpy(game, selfDir, sizeof(game) - 1);
        } else {
            strncpy(game, selfDir, sizeof(game) - 1);
        }
    }
    // 去掉尾部反斜杠
    size_t gl = strlen(game);
    while (gl > 3 && (game[gl - 1] == '\\' || game[gl - 1] == '/')) game[--gl] = 0;

    char exe[MAX_PATH] = {0};
    if (!JoinFile(exe, sizeof(exe), game, "War3.exe") || !FileExists(exe)) {
        printf("[X] 找不到 War3.exe，游戏目录无效: %s\n", game);
        return 1;
    }

    char dll[MAX_PATH] = {0};
    const char *dp = ArgValue(argc, argv, "-dll");
    if (dp) {
        strncpy(dll, dp, sizeof(dll) - 1);
    } else if (!JoinFile(dll, sizeof(dll), game, "patch\\PathParallel_probe.dll")) {
        printf("[X] DLL 路径过长\n");
        return 1;
    }
    if (!FileExists(dll)) {
        printf("[X] 找不到 DLL: %s\n", dll);
        return 1;
    }

    const char *map = ArgValue(argc, argv, "-map");

    char cmd[MAX_PATH * 3] = {0};
    if (map && map[0])
        _snprintf(cmd, sizeof(cmd) - 1, "\"%s\" -window -loadfile \"%s\"", exe, map);
    else
        _snprintf(cmd, sizeof(cmd) - 1, "\"%s\" -window", exe);
    cmd[sizeof(cmd) - 1] = 0;

    printf("[i] 游戏目录: %s\n", game);
    printf("[i] 注入目标: %s\n", dll);
    printf("[i] 命令行  : %s\n", cmd);

    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    // 环境变量从本进程继承：PATH_HOOK / PATH_SLOT / SLOT_DRY
    if (!CreateProcessA(NULL, cmd, NULL, NULL, FALSE, CREATE_SUSPENDED, NULL, game, &si, &pi)) {
        printf("[X] 启动失败 (CreateProcess err=%lu)\n", GetLastError());
        return 2;
    }

    SIZE_T len = strlen(dll) + 1;
    void *mem = VirtualAllocEx(pi.hProcess, NULL, len, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!mem || !WriteProcessMemory(pi.hProcess, mem, dll, len, NULL)) {
        printf("[X] 写入目标进程失败 (err=%lu)\n", GetLastError());
        TerminateProcess(pi.hProcess, 1);
        return 3;
    }

    HMODULE k32 = GetModuleHandleA("kernel32.dll");
    FARPROC load = GetProcAddress(k32, "LoadLibraryA");
    HANDLE th = CreateRemoteThread(pi.hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)load, mem, 0, NULL);
    if (!th) {
        printf("[X] 创建远程线程失败 (err=%lu)\n", GetLastError());
        TerminateProcess(pi.hProcess, 1);
        return 3;
    }

    WaitForSingleObject(th, 10000);
    DWORD libRet = 0;
    GetExitCodeThread(th, &libRet);
    CloseHandle(th);

    ResumeThread(pi.hThread);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    if (libRet == 0) {
        printf("[X] DLL 在目标进程内加载失败（可能是版本不匹配或权限不足）\n");
        return 4;
    }

    printf("[OK] 已注入并启动，pid=%lu\n", pi.dwProcessId);
    return 0;
}
