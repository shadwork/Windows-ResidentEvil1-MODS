#pragma once
#include <stdafx.h>

//////////////////////////////////
// LIBGS STRUCTURES

struct MATRIX
{
	s16 m[3][3];
	s16 dm;
	int t[3];
};

struct VECTOR
{
	int vx;
	int vy;
	int vz;
	int pad;
};

struct SVECTOR
{
	s16 vx;
	s16 vy;
	s16 vz;
	s16 pad;
};

typedef struct LINE
{
	u32 tag;
	s16 x0, y0,
		x1, y1;
	u8 r, g, b,
		pad;
} LINE;

typedef struct TILE
{
	u32 tag;
	s16 x, y;
	u16 w, h;
	u8 r, g, b,
		pad;
} TILE;

//////////////////////////////////
// DIRECT MEMORY ACCESS

#define PL_WORK_BASE   0xC351B4

// Player active flag
#define PL_BE_FLG      (*(char *)PL_WORK_BASE)

// Player health (s16)
#define PL_LIFE        (*(s16 *)0x00C3870E)

// Character ID (0=Chris, 1=Jill)
#define CHAR_ID        (*(u8 *)0x00C351B5)

// Max HP byte
#define MAX_LIFE       (*(u8 *)0x00C35329)

// Player world position
#define PL_POS_X       (*(int *)(PL_WORK_BASE + 0x34))
#define PL_POS_Y       (*(int *)(PL_WORK_BASE + 0x38))
#define PL_POS_Z       (*(int *)(PL_WORK_BASE + 0x3C))

// GsWSMATRIX (world-to-camera view matrix)
#define GS_WS_MATRIX   ((MATRIX *)0x4AA320)
#define GS_OFFX        (*(s16 *)0x4AA360)
#define GS_OFFY        (*(s16 *)0x4AA362)

// Projection height
#define GS_SCREEN_H    (*(int *)0x4D7030)

// Render queue counters
#define RENDER_COUNT_0  ((u32 *)0x4B27F8)
#define RENDER_COUNT_1  ((u32 *)0x4B27FC)
#define RENDER_COUNT_2  ((u32 *)0x4B2800)

// Enemy work array
#define EM_WORK_BASE    (PL_WORK_BASE + 0x180)
#define EM_WORK_SIZE    0x18C
#define EM_MAX_COUNT    32
#define EM_HP_OFFSET    0x88
#define EM_STATE_OFFSET 0x84

void Install_healthbar(u8 *pExe);
