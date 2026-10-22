// ============================================================================
//  SlotPatch.dll —— War3「流畅指挥」槽位散列补丁（Game.dll 版本分派版）
//
//  支持两个版本：
//    1.24e  Game.dll 1.24.4.6387   2 处补丁点，槽表 RVA 0x00A9B018
//    1.27a  Game.dll 1.27.0.52240  4 处补丁点，槽表 RVA 0x00BC5FF8
//
//  原理：引擎把「移动微步」放进按槽位组织的 FIFO 队列，槽号原本取
//        (flags >> 16) & 0xF（玩家/路径分组号）→ 同一玩家共用 1 个槽，
//        每 tick 只放行队头 1 个。
//        本补丁改为 (path >> 12) & 0xF，让不同 path 散列到 16 个槽并行。
//        槽表结构（16 槽、步长 0x70、子块 +0x54、标志 0x2000000）不需改动。
//
//  安全设计：
//    * 所有地址以 **RVA**（VA - 0x6F000000）存储，按实际模块基址换算，
//      不假设 ImageBase 一定是 0x6F000000。
//    * 打补丁前逐字节校验原值（signature check），不匹配则整体跳过，
//      绝不盲写。
//    * 检测方式用「签名匹配」而非版本字符串，因此对同版本的其它构建
//      （官方安装版 / 绿色版）只要签名一致即可工作。
//    * 设 SLOT_DRY=1 时只校验并写日志，不修改内存。
//
//  构建（32 位）：
//    g++ -m32 -shared -O2 -static-libgcc -static-libstdc++ \
//        -o SlotPatch.dll SlotPatch.cpp
// ============================================================================

#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

// ---------------------------------------------------------------- 数据结构

struct BytePatch
{
    DWORD       rva;        // 相对 Game.dll 模块基址的 RVA
    BYTE        expect[4];  // 期望的原字节
    BYTE        repl[4];    // 替换后的字节
    BYTE        len;        // 有效字节数
    const char *desc;
};

struct Profile
{
    const char     *name;
    DWORD           tableRva;   // 槽表基址 RVA（仅用于日志/健全性检查）
    const BytePatch *patches;
    int             count;
};

// ---- 1.24b ~ 1.26：2 处槽位计算，结构一致（path 在 ecx / esi）----


static const BytePatch kPatches124b[] = {
    { 0x0049BD86, {0x8B, 0xC2, 0x00, 0x00}, {0x8B, 0xC1, 0x00, 0x00}, 2,
      "site1: mov eax,edx -> mov eax,ecx  (path 在 ecx)" },
    { 0x0049BD88, {0xC1,0xE8,0x10,0x00}, {0xC1,0xE8,0x0C,0x00}, 3,
      "shr eax,0x10 -> shr eax,0xC" },
    { 0x0049C039, {0x8B, 0xC1, 0x00, 0x00}, {0x8B, 0xC6, 0x00, 0x00}, 2,
      "site2: mov eax,ecx -> mov eax,esi  (path 在 esi)" },
    { 0x0049C03B, {0xC1,0xE8,0x10,0x00}, {0xC1,0xE8,0x0C,0x00}, 3,
      "shr eax,0x10 -> shr eax,0xC" },
};

static const BytePatch kPatches124e[] = {
    { 0x0049BE06, {0x8B, 0xC2, 0x00, 0x00}, {0x8B, 0xC1, 0x00, 0x00}, 2,
      "site1: mov eax,edx -> mov eax,ecx  (path 在 ecx)" },
    { 0x0049BE08, {0xC1,0xE8,0x10,0x00}, {0xC1,0xE8,0x0C,0x00}, 3,
      "shr eax,0x10 -> shr eax,0xC" },
    { 0x0049C0B9, {0x8B, 0xC1, 0x00, 0x00}, {0x8B, 0xC6, 0x00, 0x00}, 2,
      "site2: mov eax,ecx -> mov eax,esi  (path 在 esi)" },
    { 0x0049C0BB, {0xC1,0xE8,0x10,0x00}, {0xC1,0xE8,0x0C,0x00}, 3,
      "shr eax,0x10 -> shr eax,0xC" },
};

static const BytePatch kPatches125b[] = {
    { 0x0049B0C6, {0x8B, 0xC2, 0x00, 0x00}, {0x8B, 0xC1, 0x00, 0x00}, 2,
      "site1: mov eax,edx -> mov eax,ecx  (path 在 ecx)" },
    { 0x0049B0C8, {0xC1,0xE8,0x10,0x00}, {0xC1,0xE8,0x0C,0x00}, 3,
      "shr eax,0x10 -> shr eax,0xC" },
    { 0x0049B379, {0x8B, 0xC1, 0x00, 0x00}, {0x8B, 0xC6, 0x00, 0x00}, 2,
      "site2: mov eax,ecx -> mov eax,esi  (path 在 esi)" },
    { 0x0049B37B, {0xC1,0xE8,0x10,0x00}, {0xC1,0xE8,0x0C,0x00}, 3,
      "shr eax,0x10 -> shr eax,0xC" },
};

static const BytePatch kPatches126[] = {
    { 0x0049B2F6, {0x8B, 0xC2, 0x00, 0x00}, {0x8B, 0xC1, 0x00, 0x00}, 2,
      "site1: mov eax,edx -> mov eax,ecx  (path 在 ecx)" },
    { 0x0049B2F8, {0xC1,0xE8,0x10,0x00}, {0xC1,0xE8,0x0C,0x00}, 3,
      "shr eax,0x10 -> shr eax,0xC" },
    { 0x0049B5A9, {0x8B, 0xC1, 0x00, 0x00}, {0x8B, 0xC6, 0x00, 0x00}, 2,
      "site2: mov eax,ecx -> mov eax,esi  (path 在 esi)" },
    { 0x0049B5AB, {0xC1,0xE8,0x10,0x00}, {0xC1,0xE8,0x0C,0x00}, 3,
      "shr eax,0x10 -> shr eax,0xC" },
};

// ---- 1.27a：4 处 × 2 字节（注意 site3/site4 编码互为镜像）----
static const BytePatch kPatches127a[] = {
    { 0x001133C6, {0x8B,0xC2,0x00,0x00}, {0x8B,0xC3,0x00,0x00}, 2,
      "site1: mov eax,edx -> mov eax,ebx  (path 在 ebx, 内联副本)" },
    { 0x001133C8, {0xC1,0xE8,0x10,0x00}, {0xC1,0xE8,0x0C,0x00}, 3,
      "site1: shr eax,0x10 -> shr eax,0xC" },
    { 0x0011363C, {0x8B,0xC6,0x00,0x00}, {0x8B,0xC3,0x00,0x00}, 2,
      "site2: mov eax,esi -> mov eax,ebx  (path 在 ebx, 内联副本)" },
    { 0x0011363E, {0xC1,0xE8,0x10,0x00}, {0xC1,0xE8,0x0C,0x00}, 3,
      "site2: shr eax,0x10 -> shr eax,0xC" },
    { 0x0011411A, {0x8B,0xC6,0x00,0x00}, {0x8B,0xC7,0x00,0x00}, 2,
      "site3: mov eax,esi -> mov eax,edi  (path 在 edi, 查表 helper)" },
    { 0x0011411C, {0xC1,0xE8,0x10,0x00}, {0xC1,0xE8,0x0C,0x00}, 3,
      "site3: shr eax,0x10 -> shr eax,0xC" },
    { 0x00114F5A, {0x8B,0xC7,0x00,0x00}, {0x8B,0xC6,0x00,0x00}, 2,
      "site4: mov eax,edi -> mov eax,esi  (path 在 esi, 出队 helper)" },
    { 0x00114F5C, {0xC1,0xE8,0x10,0x00}, {0xC1,0xE8,0x0C,0x00}, 3,
      "site4: shr eax,0x10 -> shr eax,0xC" },
};

static const Profile kProfiles[] = {
    { "1.24b (Game.dll 1.24.1.6374)", 0x00A9B018, kPatches124b, 4 },
    { "1.24e (Game.dll 1.24.4.6387)", 0x00A9B018, kPatches124e, 4 },
    { "1.25b (Game.dll 1.25.1.6397)", 0x00A84FF8, kPatches125b, 4 },
    { "1.26  (Game.dll 1.26.0.6401)", 0x00A84FF8, kPatches126, 4 },
    { "1.27a (Game.dll 1.27.0.52240)", 0x00BC5FF8, kPatches127a, 8 },
};


static const int kProfileCount = (int)(sizeof(kProfiles) / sizeof(kProfiles[0]));

// ---------------------------------------------------------------- 日志

static CRITICAL_SECTION g_logLock;
static char             g_logPath[MAX_PATH] = {0};
static bool             g_logReady = false;

static void LogInit(HINSTANCE hinst)
{
    InitializeCriticalSection(&g_logLock);
    char path[MAX_PATH] = {0};
    if (GetModuleFileNameA(hinst, path, MAX_PATH))
    {
        char *slash = strrchr(path, '\\');
        if (slash) *(slash + 1) = 0;
        _snprintf(g_logPath, MAX_PATH, "%sSlotPatch.log", path);
        g_logReady = true;
    }
}

static void LogA(const char *fmt, ...)
{
    if (!g_logReady) return;
    EnterCriticalSection(&g_logLock);
    FILE *f = fopen(g_logPath, "a");
    if (f)
    {
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
    LeaveCriticalSection(&g_logLock);
}

// ---------------------------------------------------------------- 内存写入

static bool WriteBytes(void *addr, const void *src, size_t len)
{
    DWORD oldProtect = 0;
    if (!VirtualProtect(addr, len, PAGE_EXECUTE_READWRITE, &oldProtect))
    {
        LogA("VirtualProtect(RWX) failed at %p, err=%lu", addr, GetLastError());
        return false;
    }
    memcpy(addr, src, len);
    FlushInstructionCache(GetCurrentProcess(), addr, len);
    DWORD tmp = 0;
    VirtualProtect(addr, len, oldProtect, &tmp);
    return true;
}

// ---------------------------------------------------------------- 检测与打补丁

// 从已加载模块的 PE 头读取 SizeOfImage，用于越界校验。
// 返回 0 表示不是有效 PE。
static DWORD ImageSize(BYTE *base)
{
    if (*(WORD *)base != 0x5A4D) return 0;                 // 'MZ'
    DWORD e_lfanew = *(DWORD *)(base + 0x3C);
    BYTE *nt = base + e_lfanew;
    if (*(DWORD *)nt != 0x00004550) return 0;              // 'PE\0\0'
    // PE32: OptionalHeader 起点 = nt + 24；SizeOfImage 偏移 0x38
    return *(DWORD *)(nt + 24 + 0x38);
}

// 判断某个 profile 的全部签名是否与内存一致
static bool ProfileMatches(BYTE *base, DWORD imgSize, const Profile *p)
{
    for (int i = 0; i < p->count; ++i)
    {
        const BytePatch *b = &p->patches[i];
        // 越界保护：绝不读取模块映像之外的内存
        if (b->rva + b->len > imgSize) return false;
        if (memcmp(base + b->rva, b->expect, b->len) != 0)
            return false;
    }
    return true;
}

// 统计某个 profile 已打过补丁的条数（用于识别「已重复注入」的情况）
static int ProfilePatchedCount(BYTE *base, DWORD imgSize, const Profile *p)
{
    int n = 0;
    for (int i = 0; i < p->count; ++i)
    {
        const BytePatch *b = &p->patches[i];
        if (b->rva + b->len > imgSize) return 0;
        if (memcmp(base + b->rva, b->repl, b->len) == 0) ++n;
    }
    return n;
}

// 等待 Game.dll 载入。注意：1.24e/1.27a 的 Game.dll 在**主菜单阶段还没加载**，
// 要等玩家进入地图才载入，因此这里最多等 60 秒；超时则本次不做事。
static HMODULE WaitForGame(void)
{
    HMODULE game = NULL;
    for (int i = 0; i < 600; ++i)          // 最多 60 秒
    {
        game = GetModuleHandleA("Game.dll");
        if (game) break;
        Sleep(100);
    }
    return game;
}

static void ApplySlotPatch(HMODULE game)
{
    BYTE *base = (BYTE *)game;
    DWORD imgSize = ImageSize(base);
    LogA("================================================================");
    LogA("SlotPatch attached. Game.dll base = %p (preferred 0x6F000000)", base);
    LogA("Game.dll SizeOfImage = 0x%08X", imgSize);
    if (imgSize == 0)
    {
        LogA("ERROR: Game.dll PE header invalid, aborting (nothing modified)");
        return;
    }

    char dry[8] = {0};
    bool bDryRun = GetEnvironmentVariableA("SLOT_DRY", dry, sizeof(dry)) > 0;
    if (bDryRun)
        LogA("SLOT_DRY is set: verify only, memory will NOT be modified");

    const Profile *hit = NULL;
    for (int i = 0; i < kProfileCount; ++i)
    {
        if (ProfileMatches(base, imgSize, &kProfiles[i]))
        {
            hit = &kProfiles[i];
            break;
        }
    }

    if (!hit)
    {
        // 可能已经打过补丁（重复注入）
        for (int i = 0; i < kProfileCount; ++i)
        {
            if (ProfilePatchedCount(base, imgSize, &kProfiles[i]) == kProfiles[i].count)
            {
                LogA("already patched: %s (%d/%d bytes match patched form)",
                     kProfiles[i].name, kProfiles[i].count, kProfiles[i].count);
                return;
            }
        }
        LogA("no profile matched. Game.dll may be an unsupported build/version.");
        LogA("supported: 1.24b / 1.24e / 1.25b / 1.26 / 1.27a");
        LogA("nothing was modified.");
        return;
    }

    LogA("detected profile: %s", hit->name);
    LogA("slot table RVA = 0x%08X (VA = 0x%p)", hit->tableRva, base + hit->tableRva);

    // 槽表健全性检查：读一次，确认地址在映像范围内（不作为失败条件）
    if (hit->tableRva + 4 <= imgSize)
        LogA("slot table first dword = 0x%08X", *(DWORD *)(base + hit->tableRva));
    else
        LogA("slot table RVA out of image range (skipped probe)");

    int applied = 0;
    for (int i = 0; i < hit->count; ++i)
    {
        const BytePatch *b = &hit->patches[i];
        BYTE *p = base + b->rva;
        LogA("  [%d/%d] rva=0x%08X  %02X %02X%s -> %02X %02X%s   %s",
             i + 1, hit->count, b->rva,
             p[0], p[1], (b->len > 2 ? " .." : ""),
             b->repl[0], b->repl[1], (b->len > 2 ? " .." : ""),
             b->desc);
        if (bDryRun)
            continue;
        if (!WriteBytes(p, b->repl, b->len))
        {
            LogA("  ERROR: write failed, aborting remaining patches");
            break;
        }
        ++applied;
    }

    if (bDryRun)
    {
        LogA("DRY RUN complete: %d site(s) validated, 0 written.", hit->count);
    }
    else if (applied == hit->count)
    {
        LogA("slot patch applied (%s): %d/%d sites, index = (path >> 12) & 0xF",
             hit->name, applied, hit->count);
    }
    else
    {
        LogA("slot patch INCOMPLETE: %d/%d sites applied", applied, hit->count);
    }
}

// ---------------------------------------------------------------- 线程与入口

static DWORD WINAPI WorkerProc(LPVOID)
{
    // 注：MinGW 不支持 MSVC 的 __try/__except，因此改用「PE 边界校验 + 签名校验」
    // 来保证不越界、不盲写；所有读取都在 SizeOfImage 范围内。

    HMODULE game = WaitForGame();
    if (!game)
    {
        LogA("Game.dll not found after 60s (still at main menu?), nothing done");
        return 0;
    }

    ApplySlotPatch(game);
    return 0;
}

BOOL WINAPI DllMain(HINSTANCE hinst, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(hinst);
        LogInit(hinst);
        LogA("--- SlotPatch.dll loaded (pid %lu) ---", GetCurrentProcessId());

        HANDLE th = CreateThread(NULL, 0, WorkerProc, NULL, 0, NULL);
        if (th) CloseHandle(th);
        else    LogA("CreateThread failed, err=%lu", GetLastError());
    }
    return TRUE;
}
