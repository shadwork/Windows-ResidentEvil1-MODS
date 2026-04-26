#include "stdafx.h"
#include "re1.h"

u8 *pExe;

// This is called as soon as the DLL loads.
void Modsdk_init(HMODULE hExe)
{
	pExe = (u8*)hExe;
}

// This is called when the DLL is closed.
void Modsdk_close()
{
}

// This is called right before the game is booted.
// Install hooks here so they don't interfere with Classic REbirth's hooks.
void Modsdk_post_init()
{
	Install_re1((u8*)0x400000);
}

// This is called whenever a save file is being loaded.
void Modsdk_load(u8 *src, size_t pos, size_t size)
{
}

// This is called whenever a save file is being written.
void Modsdk_save(u8* &dst, size_t &size)
{
}
