/**
 * stdafx.h - Precompiled header for the Door Skip Mod
 *
 * Defines fixed-width integer type aliases and includes platform headers.
 * Also declares the Mod-SDK export functions for Classic REbirth.
 */

#pragma once
#define _CRT_SECURE_NO_WARNINGS

typedef unsigned char    u8;
typedef unsigned short   u16;
typedef unsigned int     u32;
typedef unsigned __int64 u64;
typedef signed char      s8;
typedef signed short     s16;
typedef signed int       s32;
typedef signed __int64   s64;

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>

// Mod-SDK functions exported for Classic REbirth
EXTERN_C __declspec(dllexport) void Modsdk_post_init();
EXTERN_C __declspec(dllexport) void Modsdk_load(u8 *src, size_t pos, size_t size);
EXTERN_C __declspec(dllexport) void Modsdk_save(u8* &dst, size_t &size);
