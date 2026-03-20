/**
 * dllmain.cpp - DLL entry point for the Door Skip Mod
 *
 * Delegates to the Mod-SDK init/close callbacks on process attach/detach.
 */

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
