// ---------------------------------------------------------------------------
// DllMain entry point for Mod Launcher
// ---------------------------------------------------------------------------
// Forwards DLL_PROCESS_ATTACH / DETACH to Modsdk_init / Modsdk_close
// in launcher.cpp, which handles all mod loading and callback forwarding.
// ---------------------------------------------------------------------------

#include "stdafx.h"

void Modsdk_init(HMODULE hModule);
void Modsdk_close();

BOOL APIENTRY DllMain(HMODULE hModule,
                      DWORD  ul_reason_for_call,
                      LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        Modsdk_init(hModule);
        break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        break;
    case DLL_PROCESS_DETACH:
        Modsdk_close();
        break;
    }
    return TRUE;
}
