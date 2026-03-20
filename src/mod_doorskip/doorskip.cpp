/**
 * doorskip.cpp - Door Skip Mod for Resident Evil 1 (PC)
 *
 * Eliminates door-opening transition animations and the associated fade-out
 * delay in the Classic REbirth mod-SDK environment. Two runtime patches
 * redirect game CALL instructions to stub functions:
 *
 *   1. The animation loop call is replaced with an immediate return,
 *      preventing the door-opening cinematic from playing.
 *   2. The Task_sleep call that waits for the fade-out to complete is
 *      replaced with a function that forces the screen to full black
 *      instantly, allowing room loading to begin without delay.
 *
 * Together these reduce door transition times by approximately 2.5-2.7x.
 */

#include "stdafx.h"
#include "inject.h"

// Call to the animation loop function inside the animation task
#define ADDR_ANIM_CALL      0x00435AD5

// Call to Task_sleep(1) that waits for fade-out completion (~670 ms)
#define ADDR_FIRST_SLEEP    0x0048D71E

// Fade system globals
#define FADE_COUNTER        ((volatile s16 *)0x00C38704)
#define FADE_SPEED          ((volatile u16 *)0x00C3AB9A)

// ---------------------------------------------------------------------------
// Replacement stubs
// ---------------------------------------------------------------------------

/** Replaces the door animation loop with an immediate return. */
static void __cdecl skip_door_anim(void)
{
}

/**
 * Replaces Task_sleep during fade-out. Forces the screen to full black
 * immediately and holds it there so the loading screen stays opaque.
 */
static void __cdecl skip_first_sleep(int)
{
    *FADE_COUNTER = 0x7FFF;
    *FADE_SPEED = 0;
}

// ---------------------------------------------------------------------------
// Patch installation
// ---------------------------------------------------------------------------

/** Overwrites a CALL instruction at `addr` to redirect to `target`. */
static void patch_call(void *addr, void *target)
{
    DWORD old_protect;
    VirtualProtect(addr, 5, PAGE_EXECUTE_READWRITE, &old_protect);
    INJECT_CALL(addr, target, 5);
    VirtualProtect(addr, 5, old_protect, &old_protect);
}

/** Installs both door-skip patches into the running process. */
static void install_doorskip()
{
    patch_call((void *)ADDR_ANIM_CALL,  (void *)skip_door_anim);
    patch_call((void *)ADDR_FIRST_SLEEP, (void *)skip_first_sleep);
}

// ---------------------------------------------------------------------------
// Classic REbirth Mod-SDK interface
// ---------------------------------------------------------------------------

void Modsdk_init(HMODULE hModule)
{
    install_doorskip();
}

void Modsdk_close()
{
}

void Modsdk_post_init()
{
}

void Modsdk_load(u8 *src, size_t pos, size_t size)
{
}

void Modsdk_save(u8* &dst, size_t &size)
{
}
