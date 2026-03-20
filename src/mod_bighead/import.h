#pragma once
#include <stdafx.h>
#include "re1.h"

EXTERN_C_START

int __cdecl Add_line(LINE *t, int z);

void __cdecl GsSetRotMatrix(void *m);
void __cdecl GsSetTransMatrix(void *m);
int  __cdecl GsRotTransPers(s16 *v, int *sxy);

EXTERN_C_END
