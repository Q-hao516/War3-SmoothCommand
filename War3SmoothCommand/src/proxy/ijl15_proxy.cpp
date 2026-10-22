// ============================================================================
//  ijl15.dll 代理（proxy / 转发 DLL）
//
//  目的：让玩家**直接双击 War3.exe 或原版快捷方式**就能用上补丁，不再需要启动脚本。
//
//  做法：
//    1. 把游戏目录原来的 ijl15.dll 改名为 ijl15_orig.dll；
//    2. 本 DLL 命名为 ijl15.dll 放在原位置，导出表把全部 6 个函数转发给 ijl15_orig；
//    3. DllMain 启动一个线程，去加载 <游戏目录>\patch\SlotPatch.dll。
//
//  为什么选 ijl15：
//    - 它只有 6 个导出，转发面最小；
//    - 它由 Game.dll 导入，因此恰好在「Game.dll 载入」时被加载 —— 正是补丁需要的时机；
//    - War3.exe 启动阶段不依赖它，替换它不会影响启动链路。
//
//  构建：
//    cl /nologo /LD /O2 /W3 /utf-8 ijl15_proxy.cpp ^
//       /link /DEF:ijl15_proxy.def /OUT:ijl15.dll /MACHINE:X86
// ============================================================================

#include <windows.h>
#include <stdio.h>
#include <stdarg.h>

// 把全部 6 个导出转发给真正的 ijl15_orig.dll。
//
// ！！！关键：Game.dll 是按 **序号(ordinal)** 导入 ijl15 的（ordinal 2 / 4 / 3），
// 所以必须复刻原版 ijl15.dll 的序号映射，而不是按字母序分配：
//     @1 ijlGetLibVersion   @2 ijlInit   @3 ijlFree
//     @4 ijlRead            @5 ijlWrite  @6 ijlErrorStr
// 序号写错会让游戏调用到签名不同的函数，直接内存破坏 / 崩溃（已实测踩到）。
#pragma comment(linker, "/export:ijlGetLibVersion=ijl15_orig.ijlGetLibVersion,@1")
#pragma comment(linker, "/export:ijlInit=ijl15_orig.ijlInit,@2")
#pragma comment(linker, "/export:ijlFree=ijl15_orig.ijlFree,@3")
#pragma comment(linker, "/export:ijlRead=ijl15_orig.ijlRead,@4")
#pragma comment(linker, "/export:ijlWrite=ijl15_orig.ijlWrite,@5")
#pragma comment(linker, "/export:ijlErrorStr=ijl15_orig.ijlErrorStr,@6")

static void RootDir(char *out, size_t n)
{
    GetModuleFileNameA(NULL, out, (DWORD)n);   // War3.exe 的完整路径
    char *s = strrchr(out, '\\');
    if (s) *(s + 1) = 0;
}

// DllMain 里只做文件追加写，不调用 LoadLibrary（避免加载器锁死锁）
static void AppendLog(const char *fmt, ...)
{
    char root[MAX_PATH] = {0};
    RootDir(root, MAX_PATH);
    char path[MAX_PATH];
    _snprintf(path, MAX_PATH, "%spatch\\ProxyLoad.log", root);
    path[MAX_PATH - 1] = 0;

    FILE *f = fopen(path, "a");
    if (!f) return;
    SYSTEMTIME st;
    GetLocalTime(&st);
    fprintf(f, "[%02d:%02d:%02d.%03d] ", st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
    va_list ap;
    va_start(ap, fmt);
    vfprintf(f, fmt, ap);
    va_end(ap);
    fputc('\n', f);
    fclose(f);
}

// 在独立线程里加载补丁 DLL：此时不在 DllMain，LoadLibrary 是安全的
static DWORD WINAPI Worker(LPVOID)
{
    char root[MAX_PATH] = {0};
    RootDir(root, MAX_PATH);

    char dll[MAX_PATH];
    _snprintf(dll, MAX_PATH, "%spatch\\SlotPatch.dll", root);
    dll[MAX_PATH - 1] = 0;

    if (GetFileAttributesA(dll) == INVALID_FILE_ATTRIBUTES)
    {
        AppendLog("SlotPatch.dll not found (%s) - proxy idle", dll);
        return 0;
    }

    HMODULE h = LoadLibraryA(dll);
    AppendLog("LoadLibrary(%s)=%p err=%lu", dll, h, GetLastError());
    if (!h)
        AppendLog("WARNING: patch DLL failed to load; game continues unpatched");
    return 0;
}

BOOL WINAPI DllMain(HINSTANCE hinst, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(hinst);
        AppendLog("--- ijl15 proxy loaded (pid %lu) ---", GetCurrentProcessId());

        HANDLE th = CreateThread(NULL, 0, Worker, NULL, 0, NULL);
        if (th) CloseHandle(th);
        else    AppendLog("CreateThread failed err=%lu", GetLastError());
    }
    return TRUE;
}
