#include "stdafx.h"
#include "re1.h"
#include "import.h"
#include "font8x8_basic.h"
// Runtime icon data from game memory.
#define ICON_PIX_BASE   ((u8 *)0x00C197A0)
#define ICON_W  40
#define ICON_H  30
#define ICON_SIZE (ICON_W * ICON_H)  // 1200 bytes per icon
#define ICON_COUNT 75

static u32  g_palette[256];
static bool g_palette_ready = false;
static u32  g_icon_buf[ICON_SIZE];

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static void dbg(const char * /*fmt*/, ...)
{
}

// Convert PS1 16-bit color (ABBBBBGGGGGRRRRR) to 0xAARRGGBB
// Matches ToRGB888_full from the unpacker for accurate color expansion
static u32 ps1_to_argb(u16 c)
{
    if (c == 0) return 0x00000000; // transparent black
    int r, g, b;
    if (c == 0x8000) {
        r = 1; g = 0; b = 0; // special STP-only black
    } else {
        r = c & 0x1F;         r = (r << 3) | (r >> 2);
        g = (c >> 5) & 0x1F;  g = (g << 3) | (g >> 2);
        b = (c >> 10) & 0x1F; b = (b << 3) | (b >> 2);
    }
    return 0xFF000000u | ((u32)r << 16) | ((u32)g << 8) | (u32)b;
}

static const u16 k_item_palette_ps1[256] = {
    0x0000, 0x739C, 0x6F7B, 0x6318, 0x5EF7, 0x5AD6, 0x56B5, 0x4E73,
    0x4A52, 0x4210, 0x3DEF, 0x39CE, 0x35AD, 0x318C, 0x2529, 0x1CE7,
    0x18C6, 0x14A5, 0x1084, 0x0C63, 0x4E74, 0x4632, 0x39CF, 0x294C,
    0x39D1, 0x35B7, 0x2D74, 0x2955, 0x0C67, 0x18CF, 0x108E, 0x1091,
    0x0011, 0x000E, 0x000A, 0x0008, 0x0002, 0x044E, 0x2D96, 0x10D5,
    0x044B, 0x4238, 0x2970, 0x31D8, 0x2DB7, 0x1937, 0x0493, 0x0490,
    0x1116, 0x4679, 0x08D3, 0x00B5, 0x08AD, 0x04F7, 0x1D2E, 0x29B2,
    0x4AFE, 0x110E, 0x08EE, 0x1D70, 0x25F7, 0x29D3, 0x1DB4, 0x0931,
    0x296C, 0x0133, 0x675B, 0x56D7, 0x35CF, 0x4A96, 0x3A56, 0x29B0,
    0x3258, 0x150B, 0x0886, 0x4E95, 0x21F5, 0x2DD0, 0x4AFA, 0x150A,
    0x2657, 0x0464, 0x31CF, 0x214B, 0x1DB0, 0x156E, 0x4EB6, 0x2DAE,
    0x2A34, 0x1DD1, 0x0885, 0x116E, 0x3A75, 0x2E76, 0x2613, 0x36D9,
    0x114C, 0x11B0, 0x3A32, 0x36FA, 0x3F7F, 0x1DAF, 0x0CC7, 0x2255,
    0x3B3C, 0x158E, 0x3F9F, 0x0A34, 0x3EB6, 0x25F0, 0x2E74, 0x0509,
    0x01D0, 0x0633, 0x0675, 0x09AE, 0x058D, 0x254A, 0x3610, 0x1908,
    0x21EF, 0x1E10, 0x2273, 0x1E52, 0x0E52, 0x11EE, 0x0549, 0x2250,
    0x0969, 0x1949, 0x08C5, 0x09AA, 0x1D69, 0x0A07, 0x0E88, 0x21EA,
    0x0D85, 0x21C9, 0x16A7, 0x1606, 0x6338, 0x296A, 0x35ED, 0x10A4,
    0x10E4, 0x0441, 0x1144, 0x1263, 0x05E0, 0x0D01, 0x1600, 0x2700,
    0x15A0, 0x1140, 0x2680, 0x2201, 0x1D25, 0x3720, 0x2183, 0x31E8,
    0x3680, 0x2180, 0x39EB, 0x3A42, 0x3200, 0x2DA1, 0x2120, 0x4222,
    0x6B59, 0x6317, 0x2949, 0x39C2, 0x5200, 0x5A20, 0x4180, 0x55C0,
    0x3101, 0x61E0, 0x5180, 0x3D00, 0x55EA, 0x4501, 0x5120, 0x55CA,
    0x5E71, 0x4989, 0x5DEB, 0x356A, 0x3081, 0x4527, 0x38E5, 0x5D04,
    0x5A51, 0x2483, 0x5926, 0x4DCD, 0x6568, 0x3C82, 0x4CA2, 0x7650,
    0x3D07, 0x71ED, 0x30A4, 0x38C5, 0x3062, 0x3462, 0x58A4, 0x5273,
    0x4A31, 0x3DCE, 0x39AD, 0x358C, 0x66F7, 0x398C, 0x6AD6, 0x2908,
    0x5210, 0x418C, 0x3129, 0x49AD, 0x414A, 0x38E7, 0x30C6, 0x3863,
    0x2C42, 0x6CE8, 0x5422, 0x2C01, 0x2801, 0x2422, 0x2401, 0x2443,
    0x2485, 0x28C7, 0x1C02, 0x0C01, 0x356C, 0x350C, 0x2889, 0x7BBE,
    0x7FFF, 0x1C07, 0x282C, 0x310F, 0x28CD, 0x2850, 0x2832, 0x28CE,
    0x1C0E, 0x0C06, 0x0C0D, 0x080C, 0x148C, 0x040A, 0x18B3, 0x0001,
};

// Build 256-entry ARGB palette from the embedded STATUS.TIM item CLUT.
static void build_palette()
{
    if (g_palette_ready) return;

    for (int i = 0; i < 256; i++)
        g_palette[i] = ps1_to_argb(k_item_palette_ps1[i]);

    g_palette_ready = true;
}

// Get ARGB icon pixels for given icon index (1-based, from ITEM_DATA icon field).
// Returns static buffer (valid until next call).
static const u32* get_icon_argb(int icon_index)
{
    if (!g_palette_ready) build_palette();
    if (!g_palette_ready) return NULL;
    // icon field is 1-based: offset = (icon - 1) * 1200
    if (icon_index < 1 || icon_index > ICON_COUNT) return NULL;

    u8 *src = ICON_PIX_BASE + (icon_index - 1) * ICON_SIZE;
    for (int i = 0; i < ICON_SIZE; i++)
        g_icon_buf[i] = g_palette[src[i]];
    return g_icon_buf;
}

// Render queue counters zeroed by FUN_00440a10 each frame
#define RENDER_COUNT_0  ((u32*)0x4B27F8)
#define RENDER_COUNT_1  ((u32*)0x4B27FC)
#define RENDER_COUNT_2  ((u32*)0x4B2800)

// Pointer at 0xAA8E48 holds the active character's inventory base address.
// [item_id, quantity, item_id, quantity, ...]  2 bytes per slot.
#define INV_BASE  (*(u8 **)0x00AA8E48)

// character_id & 3 == 1 => Jill (8 slots), else Chris (6 slots)
#define CHAR_ID   (*(u8  *)0x00C351B5)

// Player health (s16)
#define PL_LIFE   (*(s16 *)0x00C3870E)

// ITEM_DATA table: 4 bytes per item {max, icon, mix_num, alt_name} at 0x4CCCBC
// icon = byte at ITEM_DATA_BASE + item_id * 4 + 1
#define ITEM_DATA_BASE  ((u8 *)0x004CCCBC)

static int item_icon(u8 item_id)
{
    if (item_id > 0x4D) return -1;
    return ITEM_DATA_BASE[item_id * 4 + 1];
}

// --- HUD layout -------------------------------------------------------
#define SLOT_COUNT  8

// Alignment enum for config
enum HudAlign { ALIGN_TOP_LEFT, ALIGN_TOP_RIGHT, ALIGN_BOTTOM_LEFT, ALIGN_BOTTOM_RIGHT };

struct HudConfig {
    bool     inv_enabled;
    HudAlign inv_align;
    int      inv_dx, inv_dy;
    int      inv_spacing_x, inv_spacing_y;
    int      inv_columns;
    int      inv_slot_w, inv_slot_h;
    int      inv_font_size;
    HudAlign inv_text_align;  // quantity text alignment relative to slot
    int      inv_text_dx, inv_text_dy;
    int      inv_quantity;   // 0=all, 1=weapon only (guns with ammo, not knife)
    bool     inv_current_weapon_enable;
    u32      inv_current_weapon_color;
    int      inv_current_weapon_width;
    bool     weapon_enabled;
    HudAlign weapon_align;
    int      weapon_dx, weapon_dy;
    int      weapon_font_size;
    HudAlign weapon_text_align;
    int      weapon_text_dx, weapon_text_dy;
    bool     health_enabled;
    HudAlign health_align;
    int      health_dx, health_dy;
    int      health_font_size;
};

static HudConfig g_cfg = {
    true, ALIGN_TOP_LEFT, 2, 2, 20, 20, 2, 16, 16, 2,
    ALIGN_BOTTOM_LEFT, 0, 0, 0,
    true, 0xFFFF0000, 1,
    false, ALIGN_BOTTOM_LEFT, 2, 2, 2,
    ALIGN_BOTTOM_LEFT, 0, 0,
    false, ALIGN_BOTTOM_LEFT, 4, 32, 2
};

static u32 parse_color(const char *val)
{
    return (u32)strtoul(val, NULL, 16);
}

static HudAlign parse_align(const char *val)
{
    if (strstr(val, "top") && strstr(val, "right"))    return ALIGN_TOP_RIGHT;
    if (strstr(val, "bottom") && strstr(val, "left"))  return ALIGN_BOTTOM_LEFT;
    if (strstr(val, "bottom") && strstr(val, "right")) return ALIGN_BOTTOM_RIGHT;
    return ALIGN_TOP_LEFT;
}

static const char *default_config_text()
{
    return
        "# HUD Overlay Configuration\n"
        "# Alignment: top-left, top-right, bottom-left, bottom-right\n"
        "# dx, dy: offset from the aligned corner (in game pixels, 320x240 space)\n"
        "# spacing: gap between inventory cells (in game pixels)\n"
        "# slot_w, slot_h: size of each inventory cell (in game pixels)\n"
        "# font_size: multiplier for 8x8 font (1 = normal, 2 = double, 4 = quadruple)\n"
        "# enabled: 1 = show, 0 = hide\n"
        "\n"
        "# Inventory panel\n"
        "inv_enabled = 1\n"
        "inv_align = top-left\n"
        "inv_dx = 2\n"
        "inv_dy = 2\n"
        "inv_spacing_x = 20\n"
        "inv_spacing_y = 20\n"
        "inv_columns = 2\n"
        "inv_slot_w = 16\n"
        "inv_slot_h = 16\n"
        "inv_font_size = 2\n"
        "# Quantity text position relative to slot (top-left, top-right, bottom-left, bottom-right)\n"
        "inv_text_align = bottom-left\n"
        "inv_text_dx = 2\n"
        "inv_text_dy = 2\n"
        "# Border highlight on currently equipped weapon slot\n"
        "# Quantity display: all = show for all items, weapon = only guns with ammo (not knife)\n"
        "inv_quantity = weapon\n"
        "inv_current_weapon_enable = 1\n"
        "inv_current_weapon_color = ffff0000\n"
        "inv_current_weapon_width = 1\n"
        "\n"
        "# Health display (ECG chart)\n"
        "health_enabled = 0\n"
        "health_align = bottom-left\n"
        "health_dx = 4\n"
        "health_dy = 32\n"
        "health_font_size = 2\n"
        "\n"
        "# Equipped weapon display (below health)\n"
        "weapon_enabled = 0\n"
        "weapon_align = bottom-left\n"
        "weapon_dx = 2\n"
        "weapon_dy = 2\n"
        "weapon_font_size = 2\n"
        "weapon_text_align = bottom-left\n"
        "weapon_text_dx = 2\n"
        "weapon_text_dy = 2\n";
}

static void create_default_config(const char *path)
{
    FILE *f = fopen(path, "w");
    if (!f) return;
    fputs(default_config_text(), f);
    fclose(f);
}

static void load_config()
{
    // Look for config next to the DLL, and create it there on first run.
    char path[MAX_PATH];
    HMODULE hm = NULL;
    GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                       GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                       (LPCSTR)&load_config, &hm);
    GetModuleFileNameA(hm, path, MAX_PATH);

    char *slash = strrchr(path, '\\');
    if (!slash) slash = strrchr(path, '/');
    if (slash) strcpy(slash + 1, "mod_hud.ini");
    else       strcpy(path, "mod_hud.ini");

    FILE *f = fopen(path, "r");
    if (!f) {
        dbg("Config not found: %s", path);
        create_default_config(path);
        f = fopen(path, "r");
        if (!f) return;
    }
    dbg("Loading config: %s", path);

    char line[256];
    while (fgets(line, sizeof(line), f)) {
        // Skip comments and empty lines
        char *p = line;
        while (*p == ' ' || *p == '\t') p++;
        if (*p == '#' || *p == '\n' || *p == '\r' || *p == 0) continue;

        char key[64] = {}, val[64] = {};
        if (sscanf(p, "%63[^=]= %63[^\r\n]", key, val) < 2) continue;

        // Trim trailing spaces from key
        char *end = key + strlen(key) - 1;
        while (end > key && (*end == ' ' || *end == '\t')) *end-- = 0;

        if      (!strcmp(key, "inv_enabled"))      g_cfg.inv_enabled      = atoi(val) != 0;
        else if (!strcmp(key, "inv_align"))    g_cfg.inv_align   = parse_align(val);
        else if (!strcmp(key, "inv_dx"))       g_cfg.inv_dx      = atoi(val);
        else if (!strcmp(key, "inv_dy"))       g_cfg.inv_dy      = atoi(val);
        else if (!strcmp(key, "inv_spacing_x")) g_cfg.inv_spacing_x = atoi(val);
        else if (!strcmp(key, "inv_spacing_y")) g_cfg.inv_spacing_y = atoi(val);
        else if (!strcmp(key, "inv_columns"))   g_cfg.inv_columns   = atoi(val);
        else if (!strcmp(key, "inv_slot_w"))      g_cfg.inv_slot_w      = atoi(val);
        else if (!strcmp(key, "inv_slot_h"))      g_cfg.inv_slot_h      = atoi(val);
        else if (!strcmp(key, "inv_font_size"))   g_cfg.inv_font_size   = atoi(val);
        else if (!strcmp(key, "inv_text_align"))  g_cfg.inv_text_align  = parse_align(val);
        else if (!strcmp(key, "inv_text_dx"))     g_cfg.inv_text_dx     = atoi(val);
        else if (!strcmp(key, "inv_text_dy"))     g_cfg.inv_text_dy     = atoi(val);
        else if (!strcmp(key, "inv_quantity"))    g_cfg.inv_quantity    = strstr(val, "weapon") ? 1 : 0;
        else if (!strcmp(key, "inv_current_weapon_enable")) g_cfg.inv_current_weapon_enable = atoi(val) != 0;
        else if (!strcmp(key, "inv_current_weapon_color"))  g_cfg.inv_current_weapon_color  = parse_color(val);
        else if (!strcmp(key, "inv_current_weapon_width"))  g_cfg.inv_current_weapon_width  = atoi(val);
        else if (!strcmp(key, "weapon_enabled"))  g_cfg.weapon_enabled  = atoi(val) != 0;
        else if (!strcmp(key, "weapon_align"))    g_cfg.weapon_align    = parse_align(val);
        else if (!strcmp(key, "weapon_dx"))       g_cfg.weapon_dx       = atoi(val);
        else if (!strcmp(key, "weapon_dy"))       g_cfg.weapon_dy       = atoi(val);
        else if (!strcmp(key, "weapon_font_size"))g_cfg.weapon_font_size= atoi(val);
        else if (!strcmp(key, "weapon_text_align"))g_cfg.weapon_text_align= parse_align(val);
        else if (!strcmp(key, "weapon_text_dx"))  g_cfg.weapon_text_dx  = atoi(val);
        else if (!strcmp(key, "weapon_text_dy"))  g_cfg.weapon_text_dy  = atoi(val);
        else if (!strcmp(key, "health_enabled"))  g_cfg.health_enabled  = atoi(val) != 0;
        else if (!strcmp(key, "health_align"))    g_cfg.health_align    = parse_align(val);
        else if (!strcmp(key, "health_dx"))       g_cfg.health_dx       = atoi(val);
        else if (!strcmp(key, "health_dy"))       g_cfg.health_dy       = atoi(val);
        else if (!strcmp(key, "health_font_size"))g_cfg.health_font_size= atoi(val);
    }
    fclose(f);
    dbg("Config: inv=%d,%d,%d sp=%d %dx%d weapon=%d,%d,%d health=%d,%d,%d",
        g_cfg.inv_align, g_cfg.inv_dx, g_cfg.inv_dy, g_cfg.inv_spacing_x,
        g_cfg.inv_slot_w, g_cfg.inv_slot_h,
        g_cfg.weapon_align, g_cfg.weapon_dx, g_cfg.weapon_dy,
        g_cfg.health_align, g_cfg.health_dx, g_cfg.health_dy);
}

// Tile-based HUD constants (used by Add_tile rendering)
#define SLOT_W        16
#define SLOT_H        16
#define SLOT_SPACING  18

// Tile coords are relative to screen centre (renderer adds 160, 120).
#define HUD_X  (-160)
#define HUD_Y  (-120)

// Tile budget: 300 tiles total per frame.
// We cap our HUD usage at 150, leaving 150 for game elements (foreground masks, etc.).
#define HUD_TILE_BUDGET  200
static int g_hud_tiles;

static void hud_tile(TILE *t)
{
    if (g_hud_tiles < HUD_TILE_BUDGET) {
        Add_tile(t, 0, 0);
        g_hud_tiles++;
    }
}

// --- primitives -------------------------------------------------------
static TILE g_box;   // reused for slot boxes
static TILE g_px;    // reused for font pixel runs (green)

// Draw an 8x8 glyph at tile position (x, y).
// Bit order: bit 0 = leftmost pixel (standard for font8x8_basic).
static void draw_glyph(s16 x, s16 y, u8 ch)
{
    if (ch >= 128) return;
    const unsigned char *glyph = font8x8_basic[ch];

    for (int oy = 0; oy < 8; oy++)
    {
        u8 row = (u8)glyph[oy];
        int col = 0;
        while (col < 8) {
            while (col < 8 && !(row & (1 << col))) col++;
            if (col >= 8) break;
            int start = col;
            while (col < 8 && (row & (1 << col))) col++;
            g_px.x = (s16)(x + start);
            g_px.y = (s16)(y + oy);
            g_px.w = (u16)(col - start);
            hud_tile(&g_px);
        }
    }
}

// Draw decimal number centred inside the 16x16 slot box at (bx, by).
// Each digit is 8x8 px.
//   1 digit  (8px wide)  → bx+4
//   2 digits (16px wide) → bx+0, bx+8
static void draw_number(s16 bx, s16 by, u8 value)
{
    s16 ty = (s16)(by + 4);   // vertical centre: (16 - 8) / 2 = 4

    if (value >= 10) {
        draw_glyph((s16)(bx + 0), ty, (u8)('0' + (value / 10) % 10));
        draw_glyph((s16)(bx + 8), ty, (u8)('0' + value % 10));
    } else {
        draw_glyph((s16)(bx + 4), ty, (u8)('0' + value));
    }
}

// --- direct pixel drawing via MarniSurface -----------------------------------
// Surface object at 0x6972E0; bpp=16, RGB565.
// Lock: int __thiscall (void* surf, int* pitch_out, void** pixels_out) @ 0x41E750
// Unlock: void __thiscall (void* surf) @ 0x41E7B0

typedef int  (__thiscall *fn_Lock)  (void* self, int* pitch_out, void** pixels_out);
typedef void (__thiscall *fn_Unlock)(void* self);

#define MARNI_SURF   ((void*)0x006972E0)

// Draw item sprites for every filled inventory slot directly into the framebuffer.
// Sprites come from game memory (40x30 indexed pixels); we centre-crop 16x16 from each.
// Slot pixel origin: (160 + bx, 120 + by) where bx/by are the tile coords.
static void log_marni(const char *label, u8 *s)
{
    u8   valid = s[0x22];
    u8   bpp   = s[0x20];
    u16  w     = *(u16*)(s + 0x1A);
    u16  h     = *(u16*)(s + 0x1C);
    void *px   = *(void**)(s + 0x08);
    u32  pitch = *(u32*)(s + 0x04);
    dbg("  %s addr=%p valid=%d bpp=%d w=%d h=%d pitch=%d px=%p",
        label, (void*)s, valid, bpp, w, h, pitch, px);
}

static void log_marni_idx(int i, u8 *s)
{
    char lbl[16];
    wsprintfA(lbl, "[%d]", i);
    log_marni(lbl, s);
}

static void log_marni_label(const char *label, u8 *s)
{
    log_marni(label, s);
}

// Scan surfaces once to identify the framebuffer.
static void scan_surfaces_once()
{
    static bool done = false;
    if (done) return;

    // Wait until render object is populated
    u32 render_obj = *(u32*)0x004CFD38;
    if (!render_obj) return;

    done = true;
    dbg("=== Surface scan (render_obj=%08x) ===", render_obj);

    // 9 MarniSurface objects at 0x697020
    char lbl[16];
    u8 *base = (u8*)0x00697020;
    for (int i = 0; i < 9; i++) {
        wsprintfA(lbl, "[%d]", i);
        log_marni(lbl, base + i * 0x58);
    }

    // Inline MarniSurface at render_obj+0x18 (what FUN_00433e50 tile renderer receives)
    log_marni("render+0x18", (u8*)render_obj + 0x18);

    dbg("  render_obj+0x1340 = %08x (flip surface ptr)", *(u32*)(render_obj + 0x1340));

    // Log vtable of the flip surface to identify Lock/Unlock offsets
    void *flip_surf = *(void**)(render_obj + 0x1340);
    if (flip_surf) {
        void **vt = *(void***)flip_surf;
        dbg("  flip_surf=%p vtable=%p", flip_surf, vt);
        for (int i = 0; i <= 0x24; i++)
            dbg("    vtable[0x%02x] = %p", i*4, vt[i]);
    }
    dbg("========================");
}

// Minimal DDSURFACEDESC (108 bytes) — only fields we need
struct DDSurfDesc {
    u32  dwSize;           // 0x00
    u32  dwFlags;          // 0x04
    u32  dwHeight;         // 0x08
    u32  dwWidth;          // 0x0C
    u32  lPitch;           // 0x10
    u32  dwBackBufferCount;// 0x14
    u32  dwMipMapCount;    // 0x18
    u32  dwAlphaBitDepth;  // 0x1C
    u32  dwReserved;       // 0x20
    void *lpSurface;       // 0x24  ← pixel pointer
    u8   _pad[108 - 0x28]; // rest of struct
};

// Per-frame game state captured by Hook_clear_render_queue (mid-frame),
// consumed by update_overlay (after Flip). Both run on the game's main thread.
// Selected inventory slot (1-based, 0 = none)
#define SELECTED_SLOT  (*(u8 *)0x00C38719)

// Game's computed health status index at 0xC2E94F (only updates on menu open):
// 0=Danger, 1=Caution(low), 2=Caution(high), 3=Fine, 4=Poison
#define GAME_STATUS_INDEX  (*(u8 *)0x00C2E94F)
// Poison raw flag at 0xC3523E: transiently 0x01/0x02 during hits (~20 frames)
// Real poison persists indefinitely
#define POISON_FLAG        (*(u8 *)0x00C3523E)
#define POISON_DEBOUNCE    25
static int g_poison_counter = 0;
// Max HP byte used by game's status formula
#define MAX_LIFE           (*(u8 *)0x00C35329)

struct FrameData {
    u8   inv[SLOT_COUNT * 2];   // [item_id, quantity] per slot
    int  max_slots;
    u8   equip_item;            // equipped weapon item_id (0 = none)
    u8   equip_ammo;            // loaded ammo count
    u8   equip_slot;            // equipped weapon slot index (0-based, 0xFF=none)
    s16  health;                // player health
    bool valid;
};

static FrameData g_cached;

// --- transparent overlay window -------------------------------------------
// A separate WS_EX_LAYERED popup sits on top of the game window.
// We draw into a 32-bit BGRA DIB and call UpdateLayeredWindow each frame.
// This is immune to D3D Present because it's a completely separate window.

static HWND    g_game_hwnd;
static HWND    g_overlay;
static HDC     g_memdc;
static HBITMAP g_membmp;
static u32    *g_ovl_px;      // BGRA pixel buffer (top-down)
static int     g_ovl_w, g_ovl_h;

static bool hwnd_belongs_to_process(HWND hwnd)
{
    if (!hwnd || !IsWindow(hwnd)) return false;

    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    return pid == GetCurrentProcessId();
}

static void find_game_hwnd()
{
    if (hwnd_belongs_to_process(g_game_hwnd)) return;

    g_game_hwnd = FindWindowA("BIOHAZARD", NULL);
    if (!hwnd_belongs_to_process(g_game_hwnd))
        g_game_hwnd = FindWindowA(NULL, "BIOHAZARD");
    if (!hwnd_belongs_to_process(g_game_hwnd))
        g_game_hwnd = NULL;
}

static void ensure_overlay()
{
    find_game_hwnd();
    if (!g_game_hwnd) return;

    if (!g_overlay) {
        WNDCLASSA wc = {};
        wc.lpfnWndProc   = DefWindowProcA;
        wc.hInstance      = GetModuleHandleA(NULL);
        wc.lpszClassName  = "RE1HudOvl";
        RegisterClassA(&wc);

        g_overlay = CreateWindowExA(
            WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_TOOLWINDOW,
            "RE1HudOvl", "",
            WS_POPUP,
            0, 0, 1, 1,
            NULL, NULL, wc.hInstance, NULL);

        ShowWindow(g_overlay, SW_SHOWNOACTIVATE);
    }

    // Match overlay to game window client area
    RECT cr;
    GetClientRect(g_game_hwnd, &cr);
    int w = cr.right, h = cr.bottom;
    if (w < 1) w = 1;
    if (h < 1) h = 1;

    POINT pt = {0, 0};
    ClientToScreen(g_game_hwnd, &pt);
    SetWindowPos(g_overlay, HWND_TOPMOST, pt.x, pt.y, w, h, SWP_NOACTIVATE);

    // Recreate DIB if size changed
    if (w != g_ovl_w || h != g_ovl_h) {
        if (g_membmp) { DeleteObject(g_membmp); g_membmp = NULL; }
        if (g_memdc)  { DeleteDC(g_memdc);      g_memdc  = NULL; }

        BITMAPINFO bmi = {};
        bmi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth       = w;
        bmi.bmiHeader.biHeight      = -h;   // top-down
        bmi.bmiHeader.biPlanes      = 1;
        bmi.bmiHeader.biBitCount    = 32;
        bmi.bmiHeader.biCompression = BI_RGB;

        HDC screenDC = GetDC(NULL);
        g_memdc  = CreateCompatibleDC(screenDC);
        g_membmp = CreateDIBSection(screenDC, &bmi, DIB_RGB_COLORS,
                                    (void**)&g_ovl_px, NULL, 0);
        SelectObject(g_memdc, g_membmp);
        ReleaseDC(NULL, screenDC);

        g_ovl_w = w;
        g_ovl_h = h;
    }
}

// Clear overlay pixel buffer (all transparent)
static void ovl_clear()
{
    if (g_ovl_px)
        memset(g_ovl_px, 0, g_ovl_w * g_ovl_h * 4);
}

// Set a pixel in overlay coords. Color: 0xAARRGGBB -> stored as BGRA in memory.
static void ovl_pixel(int x, int y, u32 argb)
{
    if (x < 0 || x >= g_ovl_w || y < 0 || y >= g_ovl_h) return;
    u8 a = (u8)(argb >> 24);
    u8 r = (u8)(argb >> 16);
    u8 g = (u8)(argb >> 8);
    u8 b = (u8)(argb);
    // Pre-multiplied alpha (required by UpdateLayeredWindow AC_SRC_ALPHA)
    g_ovl_px[y * g_ovl_w + x] = ((u32)a << 24) |
                                  ((u32)((r * a) / 255) << 16) |
                                  ((u32)((g * a) / 255) << 8) |
                                  ((u32)((b * a) / 255));
}

// Fill a rectangle in overlay coords
static void ovl_fill_rect(int x, int y, int w, int h, u32 argb)
{
    for (int oy = 0; oy < h; oy++)
        for (int ox = 0; ox < w; ox++)
            ovl_pixel(x + ox, y + oy, argb);
}

// Draw a border rectangle (outline only) at (x,y) with size (w,h) and line thickness.
static void ovl_draw_border(int x, int y, int w, int h, int thickness, u32 argb)
{
    ovl_fill_rect(x, y, w, thickness, argb);               // top
    ovl_fill_rect(x, y + h - thickness, w, thickness, argb); // bottom
    ovl_fill_rect(x, y, thickness, h, argb);               // left
    ovl_fill_rect(x + w - thickness, y, thickness, h, argb); // right
}

// Draw an 8x8 glyph scaled up by (sx, sy) into the overlay at (px, py).
static void ovl_draw_glyph(int px, int py, u8 ch, u32 argb, int scaleX, int scaleY)
{
    if (ch >= 128) return;
    const unsigned char *g = font8x8_basic[ch];
    for (int oy = 0; oy < 8; oy++) {
        u8 row = (u8)g[oy];
        for (int ox = 0; ox < 8; ox++) {
            if (row & (1 << ox))
                ovl_fill_rect(px + ox * scaleX, py + oy * scaleY, scaleX, scaleY, argb);
        }
    }
}

// Draw a decimal number string into the overlay at (px, py), scaled.
// font_size: multiplier in game pixels (1 = 8px glyph, 2 = 16px, 4 = 32px)
static void ovl_draw_number(int px, int py, int value, u32 argb, int font_size = 1)
{
    int sc = font_size * g_ovl_w / (320 * 4);
    if (sc < 1) sc = 1;
    char buf[12];
    wsprintfA(buf, "%d", value);
    for (int i = 0; buf[i]; i++)
        ovl_draw_glyph(px + i * 8 * sc, py, (u8)buf[i], argb, sc, sc);
}

// Draw a sprite scaled with nearest-neighbor.
// src: pointer to cell_width * cell_height u32 pixels (0xAARRGGBB).
// dst rect in overlay coords: (dx, dy, dw, dh).
static void ovl_draw_sprite(int dx, int dy, int dw, int dh,
                            const u32 *src, int sw, int sh)
{
    for (int oy = 0; oy < dh; oy++) {
        int sy = oy * sh / dh;
        for (int ox = 0; ox < dw; ox++) {
            int sx = ox * sw / dw;
            u32 argb = src[sy * sw + sx];
            if ((argb >> 24) == 0) continue;   // skip fully transparent
            ovl_pixel(dx + ox, dy + oy, argb);
        }
    }
}

// Push the pixel buffer to screen via UpdateLayeredWindow
static void ovl_present()
{
    if (!g_overlay || !g_memdc || !g_game_hwnd) return;

    POINT ptSrc = {0, 0};
    SIZE  sz    = {g_ovl_w, g_ovl_h};

    BLENDFUNCTION blend = {};
    blend.BlendOp             = AC_SRC_OVER;
    blend.SourceConstantAlpha = 255;
    blend.AlphaFormat         = AC_SRC_ALPHA;

    POINT ptDst = {0, 0};
    ClientToScreen(g_game_hwnd, &ptDst);

    UpdateLayeredWindow(g_overlay, NULL, &ptDst, &sz,
                        g_memdc, &ptSrc, 0, &blend, ULW_ALPHA);
}

// Map game coords (320x240) to overlay screen coords
static int game_to_screen_x(int gx) { return gx * g_ovl_w / 320; }
static int game_to_screen_y(int gy) { return gy * g_ovl_h / 240; }
static int game_to_screen_w(int gw) { return gw * g_ovl_w / 320; }
static int game_to_screen_h(int gh) { return gh * g_ovl_h / 240; }

// Compute game-space origin (gx, gy) for slot index in a grid layout.
// columns: number of columns (items wrap into rows).
// spacing_x/spacing_y: gap between cells horizontally/vertically.
static void aligned_slot_pos(HudAlign align, int dx, int dy,
                             int spacing_x, int spacing_y, int columns,
                             int slot_w, int slot_h, int slot_idx, int total_slots,
                             int *out_gx, int *out_gy)
{
    if (columns < 1) columns = 1;
    int col = slot_idx % columns;
    int row = slot_idx / columns;
    int total_rows = (total_slots + columns - 1) / columns;

    int ox = col * spacing_x;
    int oy = row * spacing_y;

    switch (align) {
    case ALIGN_TOP_LEFT:
        *out_gx = dx + ox;
        *out_gy = dy + oy;
        break;
    case ALIGN_TOP_RIGHT:
        *out_gx = 320 - slot_w - dx - (columns - 1) * spacing_x + ox;
        *out_gy = dy + oy;
        break;
    case ALIGN_BOTTOM_LEFT:
        *out_gx = dx + ox;
        *out_gy = 240 - slot_h - dy - (total_rows - 1) * spacing_y + oy;
        break;
    case ALIGN_BOTTOM_RIGHT:
        *out_gx = 320 - slot_w - dx - (columns - 1) * spacing_x + ox;
        *out_gy = 240 - slot_h - dy - (total_rows - 1) * spacing_y + oy;
        break;
    }
}

// Compute game-space origin for a single element (weapon display).
static void aligned_pos(HudAlign align, int dx, int dy, int slot_w, int slot_h,
                        int *out_gx, int *out_gy)
{
    aligned_slot_pos(align, dx, dy, 0, 0, 1, slot_w, slot_h, 0, 1, out_gx, out_gy);
}

// --- ECG heartbeat health display -------------------------------------------
// Mimics RE1's health status display with animated ECG waveform.
// RE1 health: Chris max=140 (0x8C), Jill max=96 (0x60).
// Statuses: Fine (green), Caution (yellow), Caution! (orange), Danger (red+flash)
// Thresholds are percentage-based: Fine >75%, Caution 50-75%, Caution! 25-50%, Danger <25%

static int g_ecg_phase = 0;  // animation phase counter (incremented per frame)

enum HealthStatus { HS_FINE, HS_CAUTION_Y, HS_CAUTION_O, HS_DANGER, HS_POISON };


static HealthStatus get_health_status(s16 hp)
{
    // Poison: game's menu status (definitive) OR debounced raw flag (real-time)
    if (GAME_STATUS_INDEX == 4 || g_poison_counter > POISON_DEBOUNCE)
        return HS_POISON;

    if (hp <= 0) return HS_DANGER;

    // Game formula: (HP-1) / (MaxLife >> 2) → index 0-3
    int max_hp = MAX_LIFE;
    if (max_hp <= 0) max_hp = ((CHAR_ID & 1) == 0) ? 0x8C : 0x60;
    int divisor = max_hp >> 2;
    if (divisor <= 0) divisor = 1;
    int idx = (hp - 1) / divisor;

    if (idx >= 3) return HS_FINE;
    if (idx == 2) return HS_CAUTION_Y;
    if (idx == 1) return HS_CAUTION_O;
    return HS_DANGER;
}

static u32 health_color(HealthStatus st, int phase)
{
    switch (st) {
    case HS_FINE:      return 0xFF00CC00; // green
    case HS_CAUTION_Y: return 0xFFCCCC00; // yellow
    case HS_CAUTION_O: return 0xFFCC6600; // orange
    case HS_DANGER:
        // Flash between red and dark red
        return (phase & 8) ? 0xFFCC0000 : 0xFF660000;
    case HS_POISON:    return 0xFFCC00CC; // magenta/purple
    }
    return 0xFF00CC00;
}

// ECG waveform shape: 64 samples, values -8..+8 (centred on 0).
// Flat baseline with a QRS-like spike.
static const s8 g_ecg_wave[64] = {
    0, 0, 0, 0, 0, 0, 0, 0,   // flat
    0, 0, 0, 0, 0, 0, 0, 0,   // flat
    0, 0, 0, 0, 0, 0,-1,-2,   // small P wave dip
   -1, 0, 1, 2,-3,-8, 8, 4,   // QRS spike
    0,-1, 0, 0, 0, 1, 2, 2,   // T wave
    1, 0, 0, 0, 0, 0, 0, 0,   // recovery
    0, 0, 0, 0, 0, 0, 0, 0,   // flat
    0, 0, 0, 0, 0, 0, 0, 0,   // flat
};

// Draw the animated ECG health display at screen coords (sx, sy).
// font_size controls the overall scale.
static void draw_health_ecg(int sx, int sy, s16 health, int font_size)
{
    int sc = font_size * g_ovl_w / (320 * 4);
    if (sc < 1) sc = 1;

    HealthStatus st = get_health_status(health);
    u32 color = health_color(st, g_ecg_phase);

    // Pulse speed: Fine = slow, Caution = medium, Danger/Poison = fast
    int speed;
    switch (st) {
    case HS_FINE:      speed = 1; break;
    case HS_CAUTION_Y: speed = 2; break;
    case HS_CAUTION_O: speed = 2; break;
    case HS_DANGER:    speed = 3; break;
    case HS_POISON:    speed = 3; break;
    default:           speed = 1; break;
    }

    // ECG trace: 64 columns, scrolling left
    int wave_w = 64;
    int ecg_h = 16 * sc;          // total ECG height in pixels
    int mid_y = sy + ecg_h / 2;   // baseline Y
    int col_w = sc;                // width of each column in pixels

    int offset = (g_ecg_phase * speed) % wave_w;

    for (int i = 0; i < wave_w; i++) {
        int wave_idx = (i + offset) % wave_w;
        int val = g_ecg_wave[wave_idx];

        // Scale the waveform amplitude
        int py = mid_y - val * sc;

        // Draw a thick dot (sc x sc) for this column
        ovl_fill_rect(sx + i * col_w, py, col_w, sc, color);

        // Draw vertical line connecting to previous sample for smooth trace
        if (i > 0) {
            int prev_idx = (i - 1 + offset) % wave_w;
            int prev_val = g_ecg_wave[prev_idx];
            int prev_py = mid_y - prev_val * sc;
            int y0 = (prev_py < py) ? prev_py : py;
            int y1 = (prev_py > py) ? prev_py : py;
            for (int yy = y0; yy <= y1; yy++)
                ovl_fill_rect(sx + i * col_w, yy, col_w, 1, color);
        }
    }

    // Draw status text to the right of the ECG
    const char *label;
    switch (st) {
    case HS_FINE:      label = "Fine";    break;
    case HS_CAUTION_Y: label = "Caution"; break;
    case HS_CAUTION_O: label = "Caution"; break;
    case HS_DANGER:    label = "Danger";  break;
    case HS_POISON:    label = "Poison";  break;
    default:           label = "";        break;
    }
    int txt_x = sx;
    int txt_y = sy + ecg_h + 2 * sc;
    for (int i = 0; label[i]; i++)
        ovl_draw_glyph(txt_x + i * 8 * sc, txt_y, (u8)label[i], color, sc, sc);
}

static void update_overlay()
{
    ensure_overlay();
    if (!g_ovl_px) return;

    ovl_clear();

    // Hide overlay when no inventory loaded
    if (!g_cached.valid) { ovl_present(); return; }

    int max_slots = g_cached.max_slots;
    int sw = game_to_screen_w(g_cfg.inv_slot_w);
    int sh = game_to_screen_h(g_cfg.inv_slot_h);

    if (g_cfg.inv_enabled) {
        for (int i = 0; i < max_slots; i++)
        {
            u8 item_id  = g_cached.inv[i * 2];
            u8 quantity = g_cached.inv[i * 2 + 1];
            if (item_id == 0) continue;

            int gx, gy;
            aligned_slot_pos(g_cfg.inv_align, g_cfg.inv_dx, g_cfg.inv_dy,
                             g_cfg.inv_spacing_x, g_cfg.inv_spacing_y, g_cfg.inv_columns,
                             g_cfg.inv_slot_w, g_cfg.inv_slot_h,
                             i, max_slots, &gx, &gy);

            int sx = game_to_screen_x(gx);
            int sy = game_to_screen_y(gy);

            int idx = item_icon(item_id);
            const u32 *icon = (idx >= 0) ? get_icon_argb(idx) : NULL;
            if (icon) {
                ovl_draw_sprite(sx, sy, sw, sh, icon, ICON_W, ICON_H);
            }

            bool show_qty = (quantity > 0);
            if (show_qty && g_cfg.inv_quantity == 1) {
                // weapon mode: guns (0x02-0x0A, not knife 0x01) + ammo (0x0B-0x12) + bonus weapons (0x6F-0x70)
                show_qty = (item_id >= 0x02 && item_id <= 0x12) || item_id == 0x6F || item_id == 0x70;
            }
            if (show_qty) {
                // Compute text position relative to slot based on inv_text_align
                int fsc = g_cfg.inv_font_size * g_ovl_w / (320 * 4);
                if (fsc < 1) fsc = 1;
                // Estimate text width: count digits
                int digits = 1;
                if (quantity >= 10) digits = 2;
                if (quantity >= 100) digits = 3;
                int tw = digits * 8 * fsc;
                int th = 8 * fsc;
                int tdx = game_to_screen_w(g_cfg.inv_text_dx);
                int tdy = game_to_screen_h(g_cfg.inv_text_dy);
                int tx, ty;
                switch (g_cfg.inv_text_align) {
                case ALIGN_TOP_LEFT:
                    tx = sx + tdx;
                    ty = sy + tdy;
                    break;
                case ALIGN_TOP_RIGHT:
                    tx = sx + sw - tw - tdx;
                    ty = sy + tdy;
                    break;
                case ALIGN_BOTTOM_RIGHT:
                    tx = sx + sw - tw - tdx;
                    ty = sy + sh - th - tdy;
                    break;
                default: // ALIGN_BOTTOM_LEFT
                    tx = sx + tdx;
                    ty = sy + sh - th - tdy;
                    break;
                }
                ovl_draw_number(tx, ty, quantity, 0xFF00FF00, g_cfg.inv_font_size);
            }

            // Draw border around currently equipped weapon slot
            if (g_cfg.inv_current_weapon_enable && i == g_cached.equip_slot) {
                int bw = g_cfg.inv_current_weapon_width * g_ovl_w / 320;
                if (bw < 1) bw = 1;
                ovl_draw_border(sx, sy, sw, sh, bw, g_cfg.inv_current_weapon_color);
            }
        }
    }

    if (g_cfg.weapon_enabled && g_cached.equip_item != 0) {
        int gx, gy;
        aligned_pos(g_cfg.weapon_align, g_cfg.weapon_dx, g_cfg.weapon_dy,
                    g_cfg.inv_slot_w, g_cfg.inv_slot_h, &gx, &gy);

        int sx = game_to_screen_x(gx);
        int sy = game_to_screen_y(gy);

        int idx = item_icon(g_cached.equip_item);
        const u32 *icon = (idx >= 0) ? get_icon_argb(idx) : NULL;
        if (icon) {
            ovl_draw_sprite(sx, sy, sw, sh, icon, ICON_W, ICON_H);
        }

        {
            int fsc = g_cfg.weapon_font_size * g_ovl_w / (320 * 4);
            if (fsc < 1) fsc = 1;
            int digits = 1;
            if (g_cached.equip_ammo >= 10) digits = 2;
            if (g_cached.equip_ammo >= 100) digits = 3;
            int tw = digits * 8 * fsc;
            int th = 8 * fsc;
            int tdx = game_to_screen_w(g_cfg.weapon_text_dx);
            int tdy = game_to_screen_h(g_cfg.weapon_text_dy);
            int tx, ty;
            switch (g_cfg.weapon_text_align) {
            case ALIGN_TOP_LEFT:
                tx = sx + tdx;
                ty = sy + tdy;
                break;
            case ALIGN_TOP_RIGHT:
                tx = sx + sw - tw - tdx;
                ty = sy + tdy;
                break;
            case ALIGN_BOTTOM_RIGHT:
                tx = sx + sw - tw - tdx;
                ty = sy + sh - th - tdy;
                break;
            default: // ALIGN_BOTTOM_LEFT
                tx = sx + tdx;
                ty = sy + sh - th - tdy;
                break;
            }
            ovl_draw_number(tx, ty, g_cached.equip_ammo, 0xFF00FF00, g_cfg.weapon_font_size);
        }
    }

    if (g_cfg.health_enabled) {
        int gx, gy;
        aligned_pos(g_cfg.health_align, g_cfg.health_dx, g_cfg.health_dy,
                    g_cfg.inv_slot_w, g_cfg.inv_slot_h, &gx, &gy);

        int sx = game_to_screen_x(gx);
        int sy = game_to_screen_y(gy);

        draw_health_ecg(sx, sy, g_cached.health, g_cfg.health_font_size);
    }

    ovl_present();
}

// Naked hook at 0x44cb04 (right after successful Flip).
// Original instruction: MOV EAX, 1  (B8 01 00 00 00)
static const u32 g_post_flip_ret = 0x0044cb09;

void __declspec(naked) Hook_post_flip()
{
    __asm {
        mov eax, 1
        jmp [g_post_flip_ret]
    }
}

static void draw_sprites_direct(u8 * /*inv*/, int /*max_slots*/)
{
}

// Chain hook support — call previous hook if another mod hooked 0x440A10 first
typedef void (__cdecl *fn_render_clear)();
static fn_render_clear g_prev_hook = NULL;

// Replaces FUN_00440a10 (render queue clear, called at end of every frame).
void __cdecl Hook_clear_render_queue()
{
    g_ecg_phase++;

    // Call previous hook in chain (or clear render queue ourselves)
    if (g_prev_hook)
        g_prev_hook();
    else
    {
        *RENDER_COUNT_0 = 0;
        *RENDER_COUNT_1 = 0;
        *RENDER_COUNT_2 = 0;
    }
    g_hud_tiles = 0;

    // --- inventory HUD (upper-left corner, 8 slots in a row) ---
    u8 *inv       = INV_BASE;
    int max_slots = ((CHAR_ID & 3) == 1) ? 8 : 6;

    if (inv != 0)
    {
        // Cache inventory for the overlay (runs later, after Flip)
        g_cached.max_slots = max_slots;
        for (int s = 0; s < SLOT_COUNT; s++) {
            g_cached.inv[s * 2]     = (s < max_slots) ? inv[s * 2]     : 0;
            g_cached.inv[s * 2 + 1] = (s < max_slots) ? inv[s * 2 + 1] : 0;
        }
        // Capture equipped weapon from selected slot
        u8 sel = SELECTED_SLOT;
        if (sel >= 1 && sel <= max_slots) {
            u8 eid = inv[(sel - 1) * 2];
            // Weapons: 0x01-0x0A (regular) + 0x6F-0x70 (bonus: Ingram, Minimi)
            if ((eid >= 0x01 && eid <= 0x0A) || eid == 0x6F || eid == 0x70) {
                g_cached.equip_item = eid;
                g_cached.equip_ammo = inv[(sel - 1) * 2 + 1];
                g_cached.equip_slot = sel - 1;
            } else {
                g_cached.equip_item = 0;
                g_cached.equip_ammo = 0;
                g_cached.equip_slot = 0xFF;
            }
        } else {
            g_cached.equip_item = 0;
            g_cached.equip_ammo = 0;
            g_cached.equip_slot = 0xFF;
        }

        g_cached.health = PL_LIFE;

        // Debounced poison flag for real-time detection
        u8 raw_poison = POISON_FLAG;
        if (raw_poison != 0) {
            g_poison_counter++;
        } else {
            g_poison_counter = 0;
        }

        g_cached.valid = true;

        // In-game tile rendering removed for compatibility with
        // mods that draw using game's Add_line/Add_tile primitives.
        // HUD is rendered via the Win32 overlay window instead.
    }
    else
    {
        memset(&g_cached, 0, sizeof(g_cached));
        g_cached.equip_slot = 0xFF;
        g_poison_counter = 0;
    }

    // Update overlay every frame (uses g_cached which was just written above)
    update_overlay();
}

void Install_re1(u8 *pExe)
{
    static bool installed = false;

    if (installed) return;
    dbg("Install_re1 called, pExe=%p", pExe);
    load_config();

    // slot box tile (size never changes)
    g_box.tag = 0;
    g_box.w   = SLOT_W;
    g_box.h   = SLOT_H;
    g_box.pad = 0;

    // font pixel tile (h=1, green; x/y/w set per run)
    g_px.tag = 0;
    g_px.h   = 1;
    g_px.r   = 0;
    g_px.g   = 255;
    g_px.b   = 0;
    g_px.pad = 0;

    DWORD old;
    u8 *target = &pExe[0x440A10 - EXE_DIFF];

    // Chain hook: detect if another mod already hooked this address
    VirtualProtect(target, 5, PAGE_EXECUTE_READWRITE, &old);
    if (target[0] == 0xE9)
    {
        s32 offset = *(s32 *)(target + 1);
        fn_render_clear existing_hook = (fn_render_clear)((u32)target + 5 + offset);
        if (existing_hook == Hook_clear_render_queue) {
            installed = true;
            VirtualProtect(target, 5, old, &old);
            return;
        }
        g_prev_hook = existing_hook;
        dbg("Chaining to previous hook at %p", g_prev_hook);
    }
    INJECT(target, Hook_clear_render_queue);
    VirtualProtect(target, 5, old, &old);
    installed = true;
}
