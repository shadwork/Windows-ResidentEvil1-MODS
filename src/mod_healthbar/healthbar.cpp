#include "stdafx.h"
#include "re1.h"
#include "import.h"
#include "font8x8_basic.h"
#include <stdlib.h>

// ---------------------------------------------------------------------------
// Healthbar Mod for Resident Evil (1996 PC)
//
// Renders floating HP bars above the player and enemies using the game's
// native Add_line primitive. Each entity's world-space bounding box is
// computed from its bone skeleton, projected to screen coordinates via
// GsRotTransPers, and an HP bar is drawn at the upper-left corner.
//
// Hook points:
//   0x440A10  - render queue clear (chain-safe JMP hook)
//   0x433BE0  - camera setup (CALL hooks at 6 call sites)
// ---------------------------------------------------------------------------

static MATRIX g_camera;
static int g_camera_valid = 0;

typedef int (__cdecl *fn_camera_setup)(int *param_1);
static const fn_camera_setup orig_camera_setup = (fn_camera_setup)0x433BE0;

// Health display flags (bitmask)
#define HP_DIGITS    0x01   // Show numeric HP value
#define HP_BAR       0x02   // Show health bar

// Visibility flags (bitmask)
#define HP_PLAYER    0x01   // Show player HP
#define HP_ENEMIES   0x02   // Show enemy HP

// Active configuration
#define HP_SHOW      (HP_BAR)
#define HP_VISIBLE   (HP_PLAYER | HP_ENEMIES)

// Screen-space pixel offset from projected bbox corner
#define HP_PLAYER_DX   0
#define HP_PLAYER_DY   0
#define HP_ENEMY_DX    0
#define HP_ENEMY_DY    0

// Bar dimensions
#define BAR_HP_PER_PX  5    // HP units per pixel of bar width
#define BAR_MAX_PX     60   // Maximum bar length in pixels
#define FRONT_Z        2
#define BBOX_PADDING   150
#define BONE_COUNT     15

// ---------------------------------------------------------------------------
// Camera hook: capture the current view matrix after the game sets it up
// ---------------------------------------------------------------------------
void __cdecl Hook_camera_wrapper(int *param_1)
{
    orig_camera_setup(param_1);
    memcpy(&g_camera, GS_WS_MATRIX, sizeof(MATRIX));
    g_camera_valid = 1;
}

// ---------------------------------------------------------------------------
// Project a world-space point to PSX screen coordinates via the game's GTE
// ---------------------------------------------------------------------------
static int game_project(int wx, int wy, int wz, s16 *sx, s16 *sy)
{
    s16 v[3] = { (s16)wx, (s16)wy, (s16)wz };
    int sxy = 0;
    int z4 = GsRotTransPers(v, &sxy);
    *sx = (s16)(sxy & 0xFFFF);
    *sy = (s16)(sxy >> 16);
    return z4;
}

// ---------------------------------------------------------------------------
// 2D drawing primitives using Add_line (PSX screen coordinates)
// Font rendering merges consecutive set bits into single line segments.
// ---------------------------------------------------------------------------
static void draw_hline(s16 x0, s16 x1, s16 y, u8 r, u8 g, u8 b, int z)
{
    LINE ln = {};
    ln.x0 = x0; ln.y0 = y;
    ln.x1 = x1; ln.y1 = y;
    ln.r = r; ln.g = g; ln.b = b;
    Add_line(&ln, z);
}

static void draw_rect(s16 x, s16 y, u16 w, u16 h, u8 r, u8 g, u8 b, int z)
{
    for (int row = 0; row < h; row++)
        draw_hline(x, x + w, y + row, r, g, b, z);
}

static void draw_glyph(s16 px, s16 py, char ch, u8 r, u8 g, u8 b, int z)
{
    if (ch < 0 || ch > 127) return;
    char *bitmap = font8x8_basic[(int)ch];
    for (int row = 0; row < 8; row++)
    {
        char rowbits = bitmap[row];
        if (rowbits == 0) continue;
        int run_start = -1;
        for (int col = 0; col <= 8; col++)
        {
            int set = (col < 8) && (rowbits & (1 << col));
            if (set && run_start < 0)
                run_start = col;
            else if (!set && run_start >= 0)
            {
                draw_hline(px + run_start, px + col, py + row, r, g, b, z);
                run_start = -1;
            }
        }
    }
}

static void draw_string(s16 x, s16 y, const char *str, u8 r, u8 g, u8 b, int z)
{
    while (*str)
    {
        draw_glyph(x, y, *str, r, g, b, z);
        x += 8;
        str++;
    }
}

// ---------------------------------------------------------------------------
// Compute an axis-aligned bounding box from an entity's bone skeleton.
// Iterates over bone world matrices and extends the bbox by BBOX_PADDING.
// ---------------------------------------------------------------------------
static int compute_bbox(u8 *work_base, int bone_count,
                        int *ox0, int *oy0, int *oz0,
                        int *ox1, int *oy1, int *oz1)
{
    u8 *pSin = *(u8 **)(work_base + 0x98);
    if (pSin == NULL) return 0;

    int posX = *(int *)(work_base + 0x34);
    int posY = *(int *)(work_base + 0x38);
    int posZ = *(int *)(work_base + 0x3C);

    int bx0 = posX, bx1 = posX;
    int by0 = posY, by1 = posY;
    int bz0 = posZ, bz1 = posZ;

    int topBoneIdx = 0;
    for (int i = 0; i < bone_count; i++)
    {
        MATRIX *wm = (MATRIX *)(pSin + i * 0x7C + 0x44);
        int wx = wm->t[0], wy = wm->t[1], wz = wm->t[2];

        if (wx < bx0) bx0 = wx; if (wx > bx1) bx1 = wx;
        if (wy < by0) by0 = wy; if (wy > by1) by1 = wy;
        if (wz < bz0) bz0 = wz; if (wz > bz1) bz1 = wz;

        if (wy < ((MATRIX *)(pSin + topBoneIdx * 0x7C + 0x44))->t[1])
            topBoneIdx = i;
    }

    MATRIX *topLocal = (MATRIX *)(pSin + topBoneIdx * 0x7C + 0x24);
    by0 -= abs(topLocal->t[1]) / 2;

    *ox0 = bx0 - BBOX_PADDING; *oy0 = by0 - BBOX_PADDING; *oz0 = bz0 - BBOX_PADDING;
    *ox1 = bx1 + BBOX_PADDING; *oy1 = by1 + BBOX_PADDING; *oz1 = bz1 + BBOX_PADDING;
    return 1;
}

// ---------------------------------------------------------------------------
// Project all 8 bbox corners and return the screen-space upper-left point
// ---------------------------------------------------------------------------
static int bbox_screen_upper_left(int bx0, int by0, int bz0,
                                  int bx1, int by1, int bz1,
                                  s16 *out_sx, s16 *out_sy, int *out_z)
{
    int corners[8][3] = {
        {bx0, by0, bz0}, {bx1, by0, bz0}, {bx0, by1, bz0}, {bx1, by1, bz0},
        {bx0, by0, bz1}, {bx1, by0, bz1}, {bx0, by1, bz1}, {bx1, by1, bz1}
    };

    int min_sx = 32767, min_sy = 32767;
    int any_visible = 0;
    int avg_z = 0;

    for (int i = 0; i < 8; i++)
    {
        s16 sx, sy;
        int z = game_project(corners[i][0], corners[i][1], corners[i][2], &sx, &sy);
        if (z <= 0) continue;
        any_visible = 1;
        avg_z += z;
        if (sx < min_sx) min_sx = sx;
        if (sy < min_sy) min_sy = sy;
    }

    if (!any_visible) return 0;
    *out_sx = (s16)min_sx;
    *out_sy = (s16)min_sy;
    *out_z = avg_z / 8;
    return 1;
}

// ---------------------------------------------------------------------------
// Draw HP indicator (text and/or bar) at a screen position
// ---------------------------------------------------------------------------
static void draw_hp_at(s16 sx, s16 sy, int z, s16 hp, s16 max_hp,
                       u8 r, u8 g, u8 b)
{
    if (max_hp <= 0) max_hp = 1;
    if (hp < 0) hp = 0;

    int oz = z / 4;
    if (oz < 1) oz = 1;

    s16 cur_y = sy;

#if HP_SHOW & HP_DIGITS
    {
        char buf[16];
        sprintf(buf, "%d", (int)hp);
        draw_string(sx + 1, cur_y + 1, buf, 0, 0, 0, oz + 1);
        draw_string(sx, cur_y, buf, r, g, b, oz);
        cur_y += 9;
    }
#endif

#if HP_SHOW & HP_BAR
    {
        int bar_w = hp / BAR_HP_PER_PX;
        if (bar_w < 1) bar_w = 1;
        if (bar_w > BAR_MAX_PX) bar_w = BAR_MAX_PX;
        LINE ln = {};
        ln.x0 = sx; ln.y0 = cur_y;
        ln.x1 = sx + bar_w; ln.y1 = cur_y;
        ln.r = r; ln.g = g; ln.b = b;
        Add_line(&ln, oz);
    }
#endif
}

// ---------------------------------------------------------------------------
static void draw_player_hp()
{
    if (PL_BE_FLG == 0) return;

    s16 hp = PL_LIFE;
    s16 max_hp = (s16)(MAX_LIFE > 0 ? MAX_LIFE : 200);
    if (hp <= 0) return;

    int bx0, by0, bz0, bx1, by1, bz1;
    if (!compute_bbox((u8 *)PL_WORK_BASE, BONE_COUNT,
                      &bx0, &by0, &bz0, &bx1, &by1, &bz1))
        return;

    s16 sx, sy;
    int z;
    if (!bbox_screen_upper_left(bx0, by0, bz0, bx1, by1, bz1, &sx, &sy, &z))
        return;

    draw_hp_at(sx + HP_PLAYER_DX, sy + HP_PLAYER_DY, z, hp, max_hp, 0, 200, 0);
}

// ---------------------------------------------------------------------------
static void draw_enemy_hp()
{
    for (int e = 0; e < EM_MAX_COUNT; e++)
    {
        u8 *em = (u8 *)(EM_WORK_BASE + e * EM_WORK_SIZE);
        if (*(u8 *)em == 0) continue;

        s16 hp = *(s16 *)(em + EM_HP_OFFSET);
        if (hp <= 0) continue;

        int parts_no = *(u8 *)(em + 0x8D);
        int bone_count = (parts_no > 0 && parts_no <= 32) ? parts_no : BONE_COUNT;

        int bx0, by0, bz0, bx1, by1, bz1;
        if (!compute_bbox(em, bone_count, &bx0, &by0, &bz0, &bx1, &by1, &bz1))
            continue;

        s16 sx, sy;
        int z;
        if (!bbox_screen_upper_left(bx0, by0, bz0, bx1, by1, bz1, &sx, &sy, &z))
            continue;

        draw_hp_at(sx + HP_ENEMY_DX, sy + HP_ENEMY_DY, z, hp, hp, 255, 0, 0);
    }
}

// ---------------------------------------------------------------------------
// Main draw entry point, called each frame after the render queue is cleared
// ---------------------------------------------------------------------------
static void draw_healthbars()
{
    if (GS_SCREEN_H == 0) return;
    if (!g_camera_valid) return;

    GsSetRotMatrix(&g_camera);
    GsSetTransMatrix(&g_camera);

#if HP_VISIBLE & HP_PLAYER
    draw_player_hp();
#endif
#if HP_VISIBLE & HP_ENEMIES
    draw_enemy_hp();
#endif
}

// ---------------------------------------------------------------------------
// Chain hook at 0x440A10 (render queue clear).
// Preserves any existing JMP hook so multiple mods can coexist.
// ---------------------------------------------------------------------------
typedef void (__cdecl *fn_render_clear)();
static fn_render_clear g_prev_render_clear = NULL;

void __cdecl Hook_clear_render_queue()
{
    if (g_prev_render_clear)
        g_prev_render_clear();
    else
    {
        *RENDER_COUNT_0 = 0;
        *RENDER_COUNT_1 = 0;
        *RENDER_COUNT_2 = 0;
    }

    draw_healthbars();
}

// ---------------------------------------------------------------------------
// Installation: hook render queue clear and all camera setup call sites
// ---------------------------------------------------------------------------
void Install_healthbar(u8 *pExe)
{
    DWORD old;
    u8 *target = &pExe[0x440A10 - EXE_DIFF];

    VirtualProtect(target, 5, PAGE_EXECUTE_READWRITE, &old);

    // Chain with any existing hook at this address
    if (target[0] == 0xE9)
    {
        s32 offset = *(s32 *)(target + 1);
        g_prev_render_clear = (fn_render_clear)((u32)target + 5 + offset);
    }

    INJECT(target, Hook_clear_render_queue);
    VirtualProtect(target, 5, old, &old);

    // Hook camera setup at all call sites to capture the view matrix
    u32 cam_sites[] = { 0x4017ED, 0x40184B, 0x404E93, 0x40B53F, 0x414AC6, 0x4359AB };
    for (int i = 0; i < 6; i++)
    {
        VirtualProtect(&pExe[cam_sites[i] - EXE_DIFF], 5, PAGE_EXECUTE_READWRITE, &old);
        INJECT_CALL(&pExe[cam_sites[i] - EXE_DIFF], Hook_camera_wrapper, 5);
        VirtualProtect(&pExe[cam_sites[i] - EXE_DIFF], 5, old, &old);
    }
}

// ---------------------------------------------------------------------------
// Mod SDK entry points
// ---------------------------------------------------------------------------
void Modsdk_init(HMODULE hModule) {}
void Modsdk_close() {}
void Modsdk_post_init() { Install_healthbar((u8 *)0x400000); }
void Modsdk_load(u8 *src, size_t pos, size_t size) {}
void Modsdk_save(u8* &dst, size_t &size) {}
