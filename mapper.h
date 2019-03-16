#pragma once

#include "global.h"

typedef struct NROM {
    Mapper base;
} NROM;

void NROM_init(NROM *mapper);

typedef struct MMC1 {
    Mapper base;
    u8 shift;
    u8 prg_rom_bank_mode;
    bool chr_rom_bank_mode;
    u8 chr_bank0;
    u8 chr_bank1;
    u8 prg_bank;
} MMC1;

void MMC1_init(MMC1 *mapper);

typedef struct UxROM {
    Mapper base;
    u8 prg_bank;
} UxROM;

void UxROM_init(UxROM *mapper);

typedef struct CNROM {
    Mapper base;
    u8 chr_bank;
} CNROM;

void CNROM_init(CNROM *mapper);
