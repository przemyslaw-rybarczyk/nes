#pragma once

#include "global.h"

typedef struct NROM128 {
    Mapper base;
    Mirroring mirroring;
    u8 prg_rom[0x4000];
    u8 chr_rom[0x2000];
} NROM128;

NROM128 *new_NROM128(void);

typedef struct NROM256 {
    Mapper base;
    Mirroring mirroring;
    u8 prg_rom[0x8000];
    u8 chr_rom[0x2000];
} NROM256;

NROM256 *new_NROM256(void);
