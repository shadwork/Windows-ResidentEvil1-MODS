// ---------------------------------------------------------------------------
// Mod Launcher for Classic REbirth (mod-sdk proxy)
// ---------------------------------------------------------------------------
// Classic REbirth loads a single mod DLL per directory. This DLL acts as a
// multiplexer: it reads mods_list.ini from the game's EXE directory and
// LoadLibrary's each listed DLL, then forwards the mod-sdk callbacks
// (Modsdk_post_init, Modsdk_load, Modsdk_save) to every loaded mod in order.
//
// mods_list.ini format — one DLL path per line:
//   # Lines starting with # or ; are comments
//   mod_example\example.dll
//   C:\Mods\another.dll
// ---------------------------------------------------------------------------

#include "stdafx.h"

#define MAX_MODS 32

// Mod-sdk callback signatures
typedef void (__cdecl *fn_post_init)();
typedef void (__cdecl *fn_load)(u8 *src, size_t pos, size_t size);
typedef void (__cdecl *fn_save)(u8* &dst, size_t &size);

struct ModEntry {
    char     name[MAX_PATH];
    HMODULE  hLib;
    fn_post_init  pfn_post_init;
    fn_load       pfn_load;
    fn_save       pfn_save;
};

static ModEntry g_mods[MAX_MODS];
static int      g_mod_count = 0;
static FILE    *g_log = NULL;
static char     g_exe_dir[MAX_PATH];   // game root directory (contains the EXE)

// ---------------------------------------------------------------------------
// Logging
// ---------------------------------------------------------------------------

static void log_msg(const char *fmt, ...)
{
    if (!g_log) return;
    va_list ap;
    va_start(ap, fmt);
    vfprintf(g_log, fmt, ap);
    va_end(ap);
    fprintf(g_log, "\n");
    fflush(g_log);
}

// ---------------------------------------------------------------------------
// Path helpers
// ---------------------------------------------------------------------------

// Extract the directory portion of a full path (result includes trailing backslash).
static void path_to_dir(const char *full, char *dir, size_t dirsize)
{
    strncpy(dir, full, dirsize);
    dir[dirsize - 1] = '\0';
    char *last = strrchr(dir, '\\');
    if (!last) last = strrchr(dir, '/');
    if (last) *(last + 1) = '\0';
    else dir[0] = '\0';
}

// Determine the EXE directory (game root).
static void init_dirs()
{
    char exe_path[MAX_PATH];
    GetModuleFileNameA(NULL, exe_path, sizeof(exe_path));
    path_to_dir(exe_path, g_exe_dir, sizeof(g_exe_dir));
}

// Resolve a DLL path from mods_list.ini to an absolute path.
// Absolute paths are used as-is. Relative paths are searched in:
//   1. The EXE directory
//   2. Any mod_* subdirectory of the EXE directory
static bool resolve_dll_path(const char *name, char *out, size_t outsize)
{
    // Absolute path — use as-is
    if (name[0] != '\0' && name[1] == ':') {
        strncpy(out, name, outsize);
        out[outsize - 1] = '\0';
        return true;
    }

    // Try relative to EXE directory
    snprintf(out, outsize, "%s%s", g_exe_dir, name);
    if (GetFileAttributesA(out) != INVALID_FILE_ATTRIBUTES)
        return true;

    // Try mod_* subdirectories of the EXE directory
    char search[MAX_PATH];
    snprintf(search, sizeof(search), "%smod_*", g_exe_dir);
    WIN32_FIND_DATAA fd;
    HANDLE hFind = FindFirstFileA(search, &fd);
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                snprintf(out, outsize, "%s%s\\%s", g_exe_dir, fd.cFileName, name);
                if (GetFileAttributesA(out) != INVALID_FILE_ATTRIBUTES) {
                    FindClose(hFind);
                    return true;
                }
            }
        } while (FindNextFileA(hFind, &fd));
        FindClose(hFind);
    }

    // Not found — fall back to EXE-relative path for the error message
    snprintf(out, outsize, "%s%s", g_exe_dir, name);
    return false;
}

// ---------------------------------------------------------------------------
// Mod loading
// ---------------------------------------------------------------------------

static void load_mods_list()
{
    init_dirs();

    // Open log file in the EXE directory
    char log_path[MAX_PATH];
    snprintf(log_path, sizeof(log_path), "%smod_launcher.log", g_exe_dir);
    g_log = fopen(log_path, "w");
    log_msg("=== Mod Launcher started ===");
    log_msg("EXE dir: %s", g_exe_dir);

    // Open mods_list.ini from the game root directory
    char ini_path[MAX_PATH];
    snprintf(ini_path, sizeof(ini_path), "%smods_list.ini", g_exe_dir);
    FILE *f = fopen(ini_path, "r");
    if (!f) {
        log_msg("mods_list.ini not found — create it with one DLL path per line.");
        return;
    }
    log_msg("Reading %s", ini_path);

    char line[MAX_PATH];
    while (fgets(line, sizeof(line), f))
    {
        // Strip trailing newline / carriage return
        char *p = line;
        while (*p && *p != '\r' && *p != '\n') p++;
        *p = '\0';

        // Skip blank lines and comments
        p = line;
        while (*p == ' ' || *p == '\t') p++;
        if (*p == '\0' || *p == '#' || *p == ';')
            continue;

        if (g_mod_count >= MAX_MODS) {
            log_msg("WARNING: max %d mods reached, skipping: %s", MAX_MODS, p);
            continue;
        }

        char full_path[MAX_PATH];
        bool found = resolve_dll_path(p, full_path, sizeof(full_path));

        log_msg("Loading [%d]: %s -> %s%s", g_mod_count, p, full_path,
                found ? "" : " (NOT FOUND)");

        HMODULE hLib = LoadLibraryA(full_path);
        if (!hLib) {
            log_msg("  FAILED (error %u)", GetLastError());
            continue;
        }

        ModEntry *m = &g_mods[g_mod_count];
        strncpy(m->name, p, sizeof(m->name));
        m->name[sizeof(m->name) - 1] = '\0';
        m->hLib = hLib;

        m->pfn_post_init = (fn_post_init)GetProcAddress(hLib, "Modsdk_post_init");
        m->pfn_load      = (fn_load)     GetProcAddress(hLib, "Modsdk_load");
        m->pfn_save      = (fn_save)     GetProcAddress(hLib, "Modsdk_save");

        log_msg("  OK — post_init=%p load=%p save=%p",
                m->pfn_post_init, m->pfn_load, m->pfn_save);

        g_mod_count++;
    }
    fclose(f);
    log_msg("Loaded %d mod(s)", g_mod_count);
}

static void unload_mods()
{
    for (int i = g_mod_count - 1; i >= 0; i--) {
        if (g_mods[i].hLib) {
            FreeLibrary(g_mods[i].hLib);
            g_mods[i].hLib = NULL;
        }
    }
    g_mod_count = 0;
}

// ---------------------------------------------------------------------------
// Mod-SDK interface — called by Classic REbirth
// ---------------------------------------------------------------------------

void Modsdk_init(HMODULE /* hModule */)
{
    load_mods_list();
}

void Modsdk_close()
{
    log_msg("Shutting down — unloading %d mod(s)", g_mod_count);
    unload_mods();
    if (g_log) { fclose(g_log); g_log = NULL; }
}

void Modsdk_post_init()
{
    for (int i = 0; i < g_mod_count; i++) {
        if (g_mods[i].pfn_post_init)
            g_mods[i].pfn_post_init();
    }
}

void Modsdk_load(u8 *src, size_t pos, size_t size)
{
    for (int i = 0; i < g_mod_count; i++) {
        if (g_mods[i].pfn_load)
            g_mods[i].pfn_load(src, pos, size);
    }
}

void Modsdk_save(u8* &dst, size_t &size)
{
    for (int i = 0; i < g_mod_count; i++) {
        if (g_mods[i].pfn_save)
            g_mods[i].pfn_save(dst, size);
    }
}
