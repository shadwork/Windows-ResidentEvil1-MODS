/**
 * inject.h - x86 CALL instruction injection helper
 *
 * Provides a macro to overwrite a code region with a relative CALL (0xE8)
 * to a replacement function, NOP-padding the remaining bytes.
 */

#pragma once

#pragma pack(push, 1)
typedef struct {
    BYTE opCode;    // 0xE8 (CALL) or 0xE9 (JMP)
    DWORD offset;   // Relative offset to target
} JMP;
#pragma pack(pop)

/**
 * Overwrites `size` bytes at `from` with NOPs, then writes a relative CALL
 * instruction at the start that redirects execution to `to`.
 */
#define INJECT_CALL(from, to, size) { \
    memset((from), 0x90, size); \
    ((JMP*)(from))->opCode = 0xE8; \
    ((JMP*)(from))->offset = (DWORD)(to) - ((DWORD)(from) + sizeof(JMP)); \
}
