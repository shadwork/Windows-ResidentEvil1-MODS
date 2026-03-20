#include "stdafx.h"
#include "re1.h"
#include "import.h"

// ---------------------------------------------------------------------------
// Bighead Mod for Resident Evil (Classic REbirth)
//
// Scales the head bone of player and enemy character models to create an
// exaggerated big-head effect. Head bones can be specified explicitly per
// entity type or auto-detected by finding the highest bone in an upright
// skeleton.
//
// Configuration:
//   DEBUG_ENUMERATE  - Set to 1 to cycle through bone indices one at a time,
//                      displaying the index on-screen. Useful for identifying
//                      which bone index corresponds to which body part.
//   HEAD_BONE_PLAYER - Fixed bone index for the player (-1 = auto-detect).
//   HEAD_BONE_ENEMY  - Fixed bone index for enemies (-1 = auto-detect).
//   HEAD_SCALE       - Scale factor in 4.12 fixed-point (0x1000 = 1.0x).
// ---------------------------------------------------------------------------

// 1 = enumerate mode (cycle bones), 0 = normal bighead mode
#define DEBUG_ENUMERATE   0

// Frames to hold each bone during enumeration
#define ENUM_FRAMES       60

// Fixed bone indices (-1 = auto-detect via relative Y position)
#define HEAD_BONE_PLAYER  -1
#define HEAD_BONE_ENEMY    2

// Scale multiplier in 4.12 fixed-point (0x1000 = 1.0x)
#define HEAD_SCALE     0x2000

// Visibility flags
#define BH_PLAYER      0x01
#define BH_ENEMIES     0x02
#define BH_DEAD        0x04
#define BH_VISIBLE     (BH_PLAYER | BH_ENEMIES | BH_DEAD)

#define BONE_COUNT     15
#define UPRIGHT_Y_SPREAD 500

#define CURRENT_ENTITY  (*(u8 **)0x00C3ABA4)

typedef void (__cdecl *fn_MulMatrixKernel)(void *a, void *b, void *out);
static const fn_MulMatrixKernel MulMatrixKernel = (fn_MulMatrixKernel)0x4336F0;

typedef int (__cdecl *fn_camera_setup)(int *param_1);
static const fn_camera_setup orig_camera_setup = (fn_camera_setup)0x433BE0;

// Per entity-type head bone cache
static s8 g_head_by_type[256];
static int g_type_init = 0;

// Enumerate state
static int g_frame = 0;
static int g_enum_bone = 0;

// Camera matrix for world-to-screen projection
static MATRIX g_camera;
static int g_camera_valid = 0;

// ---------------------------------------------------------------------------
// Bitmap font digits (0-9) for on-screen bone index display
// ---------------------------------------------------------------------------
static const char font_digits[10][8] = {
    { 0x3E, 0x63, 0x73, 0x7B, 0x6F, 0x67, 0x3E, 0x00},   // 0
    { 0x0C, 0x0E, 0x0C, 0x0C, 0x0C, 0x0C, 0x3F, 0x00},   // 1
    { 0x1E, 0x33, 0x30, 0x1C, 0x06, 0x33, 0x3F, 0x00},   // 2
    { 0x1E, 0x33, 0x30, 0x1C, 0x30, 0x33, 0x1E, 0x00},   // 3
    { 0x38, 0x3C, 0x36, 0x33, 0x7F, 0x30, 0x78, 0x00},   // 4
    { 0x3F, 0x03, 0x1F, 0x30, 0x30, 0x33, 0x1E, 0x00},   // 5
    { 0x1C, 0x06, 0x03, 0x1F, 0x33, 0x33, 0x1E, 0x00},   // 6
    { 0x3F, 0x33, 0x30, 0x18, 0x0C, 0x0C, 0x0C, 0x00},   // 7
    { 0x1E, 0x33, 0x33, 0x1E, 0x33, 0x33, 0x1E, 0x00},   // 8
    { 0x1E, 0x33, 0x33, 0x3E, 0x30, 0x18, 0x0E, 0x00},   // 9
};

// ---------------------------------------------------------------------------
static void draw_hline(s16 x0, s16 x1, s16 y, u8 r, u8 g, u8 b, int z)
{
    LINE ln = {};
    ln.x0 = x0; ln.y0 = y;
    ln.x1 = x1; ln.y1 = y;
    ln.r = r; ln.g = g; ln.b = b;
    Add_line(&ln, z);
}

static void draw_digit(s16 px, s16 py, int d, u8 r, u8 g, u8 b, int z)
{
    if (d < 0 || d > 9) return;
    const char *bitmap = font_digits[d];
    for (int row = 0; row < 8; row++)
    {
        char bits = bitmap[row];
        if (bits == 0) continue;
        int run_start = -1;
        for (int col = 0; col <= 8; col++)
        {
            int set = (col < 8) && (bits & (1 << col));
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

static void draw_number(s16 x, s16 y, int num, u8 r, u8 g, u8 b, int z)
{
    char buf[8];
    sprintf(buf, "%d", num);
    for (int i = 0; buf[i]; i++)
    {
        draw_digit(x, y, buf[i] - '0', r, g, b, z);
        x += 8;
    }
}

// ---------------------------------------------------------------------------
// Project a world-space point to screen coordinates
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
// Draw bone index label above a bone's world position (enumerate mode)
// ---------------------------------------------------------------------------
static void draw_bone_label(u8 *entity, int bone_idx)
{
    if (!g_camera_valid || GS_SCREEN_H == 0) return;

    u8 *pSin = *(u8 **)(entity + 0x98);
    if (!pSin) return;

    MATRIX *wm = (MATRIX *)(pSin + bone_idx * 0x7C + 0x44);

    GsSetRotMatrix(&g_camera);
    GsSetTransMatrix(&g_camera);

    s16 sx, sy;
    int z = game_project(wm->t[0], wm->t[1] - 300, wm->t[2], &sx, &sy);
    if (z <= 0) return;

    draw_number(sx + 1, sy + 1, bone_idx, 0, 0, 0, 2);
    draw_number(sx, sy, bone_idx, 255, 255, 0, 1);
}

// ---------------------------------------------------------------------------
// Auto-detect the head bone by finding the highest bone (lowest Y) in an
// upright skeleton. Returns -1 if the skeleton is not upright or invalid.
// ---------------------------------------------------------------------------
static int detect_head_bone(u8 *entity)
{
    u8 *pSin = *(u8 **)(entity + 0x98);
    if (!pSin) return -1;

    int parts_no = *(u8 *)(entity + 0x8D);
    int bc = (parts_no > 0 && parts_no <= 32) ? parts_no : BONE_COUNT;

    int entity_y = *(int *)(entity + 0x38);
    int topIdx = 0;
    int min_rel = ((MATRIX *)(pSin + 0x44))->t[1] - entity_y;
    int max_rel = min_rel;

    for (int i = 1; i < bc; i++)
    {
        MATRIX *wm = (MATRIX *)(pSin + i * 0x7C + 0x44);
        int rel = wm->t[1] - entity_y;
        if (rel < min_rel) { min_rel = rel; topIdx = i; }
        if (rel > max_rel) max_rel = rel;
    }

    if ((max_rel - min_rel) < UPRIGHT_Y_SPREAD) return -1;
    return topIdx;
}

// ---------------------------------------------------------------------------
// Get the head bone index for an entity. Uses fixed overrides if configured,
// otherwise auto-detects and caches the result by entity type.
// ---------------------------------------------------------------------------
static int get_head_for_entity(u8 *entity)
{
    u32 addr = (u32)entity;

    if (addr == PL_WORK_BASE && HEAD_BONE_PLAYER >= 0)
        return HEAD_BONE_PLAYER;
    if (addr != PL_WORK_BASE && HEAD_BONE_ENEMY >= 0)
        return HEAD_BONE_ENEMY;

    if (!g_type_init)
    {
        memset(g_head_by_type, -1, sizeof(g_head_by_type));
        g_type_init = 1;
    }

    u8 type_id = *(u8 *)(entity + 0x01);
    if (g_head_by_type[type_id] >= 0)
        return g_head_by_type[type_id];

    int head = detect_head_bone(entity);
    if (head >= 0)
        g_head_by_type[type_id] = (s8)head;
    return head;
}

// ---------------------------------------------------------------------------
// Determine which bone to scale this frame. In enumerate mode, cycles through
// all bones; in normal mode, returns the head bone.
// ---------------------------------------------------------------------------
static int get_scale_bone(u8 *entity)
{
#if DEBUG_ENUMERATE
    u8 *pSin = *(u8 **)(entity + 0x98);
    if (!pSin) return -1;
    int parts_no = *(u8 *)(entity + 0x8D);
    int bc = (parts_no > 0 && parts_no <= 32) ? parts_no : BONE_COUNT;
    return (g_enum_bone < bc) ? g_enum_bone : -1;
#else
    return get_head_for_entity(entity);
#endif
}

// ---------------------------------------------------------------------------
// Check whether an entity should have its head scaled
// ---------------------------------------------------------------------------
static int should_scale(u8 *entity)
{
    if (!entity) return 0;
    u32 addr = (u32)entity;

    if (addr == PL_WORK_BASE)
        return (BH_VISIBLE & BH_PLAYER) ? 1 : 0;

    u32 base = EM_WORK_BASE;
    u32 end = base + EM_MAX_COUNT * EM_WORK_SIZE;
    if (addr >= base && addr < end)
        return (BH_VISIBLE & BH_ENEMIES) ? 1 : 0;

    return 0;
}

// ---------------------------------------------------------------------------
// Hook for GsMulMatrix. Temporarily scales the bone's rotation matrix before
// the multiply, then restores the original values afterward.
// ---------------------------------------------------------------------------
static void __cdecl Hook_GsMulMatrix(void *a, void *b, void *out)
{
    s16 saved[9];
    int scaled = 0;

    u8 *entity = CURRENT_ENTITY;
    if (entity && should_scale(entity))
    {
        int bone = get_scale_bone(entity);
        if (bone >= 0)
        {
            u8 *pSin = *(u8 **)(entity + 0x98);
            if (pSin && (u8 *)b == pSin + bone * 0x7C + 0x44)
            {
                s16 *m = (s16 *)b;
                for (int i = 0; i < 9; i++)
                {
                    saved[i] = m[i];
                    m[i] = (s16)(((int)m[i] * HEAD_SCALE) >> 12);
                }
                scaled = 1;

#if DEBUG_ENUMERATE
                draw_bone_label(entity, bone);
#endif
            }
        }
    }

    MulMatrixKernel(a, b, out);

    if (scaled)
    {
        s16 *m = (s16 *)b;
        for (int i = 0; i < 9; i++)
            m[i] = saved[i];
    }
}

// ---------------------------------------------------------------------------
// Hook for the camera setup function. Captures the world-screen matrix for
// use in world-to-screen projection.
// ---------------------------------------------------------------------------
void __cdecl Hook_camera_wrapper(int *param_1)
{
    orig_camera_setup(param_1);
    memcpy(&g_camera, GS_WS_MATRIX, sizeof(MATRIX));
    g_camera_valid = 1;
}

// ---------------------------------------------------------------------------
// Hook for the render queue clear. Advances the enumerate bone counter when
// DEBUG_ENUMERATE is enabled.
// ---------------------------------------------------------------------------
void __cdecl Hook_clear_render_queue()
{
    *RENDER_COUNT_0 = 0;
    *RENDER_COUNT_1 = 0;
    *RENDER_COUNT_2 = 0;

#if DEBUG_ENUMERATE
    g_frame++;
    if (g_frame >= ENUM_FRAMES)
    {
        g_frame = 0;
        g_enum_bone++;
        if (g_enum_bone >= 20) g_enum_bone = 0;
    }
#endif
}

// ---------------------------------------------------------------------------
// Install hooks into the game executable
// ---------------------------------------------------------------------------
void Install_bighead(u8 *pExe)
{
    DWORD old;

    // Hook GsMulMatrix to intercept bone matrix multiplies
    VirtualProtect(&pExe[0x433750 - EXE_DIFF], 5, PAGE_EXECUTE_READWRITE, &old);
    INJECT(&pExe[0x433750 - EXE_DIFF], Hook_GsMulMatrix);
    VirtualProtect(&pExe[0x433750 - EXE_DIFF], 5, old, &old);

    // Hook render queue clear for enumerate frame counter
    VirtualProtect(&pExe[0x440A10 - EXE_DIFF], 5, PAGE_EXECUTE_READWRITE, &old);
    INJECT(&pExe[0x440A10 - EXE_DIFF], Hook_clear_render_queue);
    VirtualProtect(&pExe[0x440A10 - EXE_DIFF], 5, old, &old);

    // Hook camera setup for world-to-screen projection
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
void Modsdk_post_init() { Install_bighead((u8 *)0x400000); }
void Modsdk_load(u8 *src, size_t pos, size_t size) {}
void Modsdk_save(u8* &dst, size_t &size) {}
