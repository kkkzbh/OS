#include "stdio.h"

enum
{
    BOOT_MARKER_ROW = 24,
    BOOT_MARKER_COLS = 80,
    BOOT_MARKER_SLOT_WIDTH = 8,
    BOOT_MARKER_ATTR = 0x2F,
};

void write_boot_marker_slot(u32 slot, char const* marker)
{
    volatile u8* const vga = (volatile u8*)0xC00B8000;
    u32 const cell = (u32)(BOOT_MARKER_ROW * BOOT_MARKER_COLS) + slot * BOOT_MARKER_SLOT_WIDTH;

    for(u32 i = 0; i < BOOT_MARKER_SLOT_WIDTH; ++i) {
        u8 const ch = marker[i] == '\0' ? ' ' : (u8)marker[i];
        u32 const offset = (cell + i) * 2;
        vga[offset] = ch;
        vga[offset + 1] = BOOT_MARKER_ATTR;
    }
}
