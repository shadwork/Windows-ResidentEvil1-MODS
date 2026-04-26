#pragma once
#include <stdafx.h>

typedef struct TILE
{
	u32 tag;
	s16 x, y;
	u16 w, h;
	u8 r, g, b,
		pad;
} TILE;


void Install_re1(u8 *pExe);
