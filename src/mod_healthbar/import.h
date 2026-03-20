#pragma once
#include <stdafx.h>
#include "re1.h"

EXTERN_C_START

// Rendering
int __cdecl Add_line(LINE *t, int z);
int __cdecl Add_tile(TILE *t, int mode, int z);

// Projection / GTE emulation
void __cdecl GsSetRotMatrix(void *m);
void __cdecl GsSetTransMatrix(void *m);
int  __cdecl GsRotTransPers(s16 *v, int *sxy);

EXTERN_C_END
