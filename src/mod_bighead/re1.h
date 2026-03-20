#pragma once
#include <stdafx.h>

struct MATRIX
{
	s16 m[3][3];
	s16 dm;
	int t[3];
};

typedef struct LINE
{
	u32 tag;
	s16 x0, y0,
		x1, y1;
	u8 r, g, b,
		pad;
} LINE;

// Player work base
#define PL_WORK_BASE   0xC351B4
#define PL_BE_FLG      (*(char *)PL_WORK_BASE)

// Enemy work array
#define EM_WORK_BASE    (PL_WORK_BASE + 0x180)
#define EM_WORK_SIZE    0x18C
#define EM_MAX_COUNT    32
#define EM_STATE_OFFSET 0x84

// GS world-screen matrix and projection
#define GS_WS_MATRIX   ((MATRIX *)0x4AA320)
#define GS_SCREEN_H    (*(int *)0x4D7030)

// Render queue counters
#define RENDER_COUNT_0  ((u32 *)0x4B27F8)
#define RENDER_COUNT_1  ((u32 *)0x4B27FC)
#define RENDER_COUNT_2  ((u32 *)0x4B2800)

void Install_bighead(u8 *pExe);
