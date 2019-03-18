#pragma GCC diagnostic ignored "-Wincompatible-pointer-types"

#include <string.h>

#include "global.h"
#include "mapper.h"

#define def_mapper_copy(name) void name##_copy(name *src, name *dest) { \
    *dest = *src; \
    memcpy(dest->base.prg_rom, src->base.prg_rom, src->base.prg_rom_size * 0x4000); \
    memcpy(dest->base.chr_rom, src->base.chr_rom, src->base.chr_rom_size * 0x2000); \
}

def_mapper_copy(NROM)
def_mapper_copy(MMC1)
def_mapper_copy(UxROM)
def_mapper_copy(CNROM)
def_mapper_copy(MMC3)

void dummy_write(Mapper *mapper, u16 addr, u8 data) {
}

u8 basic_cpu_read(Mapper *mapper, u16 addr) {
    return mapper->prg_rom[(addr - 0x8000) % (mapper->prg_rom_size * 0x4000)];
}

u8 basic_ppu_read(Mapper *mapper, u16 addr) {
    return mapper->chr_rom[addr];
}

void NROM_init(NROM *mapper) {
    mapper->base.cpu_read = &basic_cpu_read;
    mapper->base.cpu_write = &dummy_write;
    mapper->base.ppu_read = &basic_ppu_read;
    mapper->base.ppu_write = &dummy_write;
    mapper->base.copy = &NROM_copy;
}

u8 MMC1_cpu_read(MMC1 *mapper, u16 addr) {
    switch (mapper->prg_rom_bank_mode) {
    case 0:
    case 1:
        return mapper->base.prg_rom[addr - 0x8000 + (mapper->prg_bank & 0x0E) % mapper->base.prg_rom_size * 0x4000];
    case 2:
        if (addr < 0xC000)
            return mapper->base.prg_rom[addr - 0x8000];
        else
            return mapper->base.prg_rom[addr - 0xC000 + mapper->prg_bank % mapper->base.prg_rom_size * 0x4000];
    case 3:
        if (addr < 0xC000)
            return mapper->base.prg_rom[addr - 0x8000 + mapper->prg_bank % mapper->base.prg_rom_size * 0x4000];
        else
            return mapper->base.prg_rom[addr - 0xC000 + (mapper->base.prg_rom_size - 1) * 0x4000];
    }
}

Mirroring MMC1_mirroring_table[4] = {MIRR_ONE_SCREEN_LOW, MIRR_ONE_SCREEN_HIGH, MIRR_VERTICAL, MIRR_HORIZONTAL};

void MMC1_cpu_write(MMC1 *mapper, u16 addr, u8 data) {
    if (data & 0x80) {
        mapper->shift = 0x10;
        mapper->prg_rom_bank_mode = 3;
    } else {
        bool write = mapper->shift & 0x01;
        mapper->shift = mapper->shift >> 1 | (data & 0x01) << 4;
        if (write) {
            mapper->shift &= 0x1F;
            if (addr < 0xA000) {
                mapper->base.mirroring = MMC1_mirroring_table[mapper->shift & 0x03];
                mapper->prg_rom_bank_mode = (mapper->shift & 0x0C) >> 2;
                mapper->chr_rom_bank_mode = (mapper->shift & 0x10) >> 4;
            } else if (addr < 0xC000)
                mapper->chr_bank0 = mapper->shift;
            else if (addr < 0xE000)
                mapper->chr_bank1 = mapper->shift;
            else
                mapper->prg_bank = mapper->shift & 0x0F;
            mapper->shift = 0x10;
        }
    }
}

u8 MMC1_ppu_read(MMC1 *mapper, u16 addr) {
    switch (mapper->chr_rom_bank_mode) {
    case 0:
        return mapper->base.chr_rom[addr + (mapper->chr_bank0 & 0x1E) % (2 * mapper->base.chr_rom_size) * 0x1000];
    case 1:
        if (addr < 0x1000)
            return mapper->base.chr_rom[addr + mapper->chr_bank0 % (2 * mapper->base.chr_rom_size) * 0x1000];
        else
            return mapper->base.chr_rom[addr - 0x1000 + mapper->chr_bank1 % (2 * mapper->base.chr_rom_size) * 0x1000];
    }
}

void MMC1_init(MMC1 *mapper) {
    mapper->base.cpu_read = &MMC1_cpu_read;
    mapper->base.cpu_write = &MMC1_cpu_write;
    mapper->base.ppu_read = &MMC1_ppu_read;
    mapper->base.ppu_write = &dummy_write;
    mapper->base.copy = &MMC1_copy;
    mapper->prg_rom_bank_mode = 3;
}

u8 UxROM_cpu_read(UxROM *mapper, u16 addr) {
    if (addr < 0xC000)
        return mapper->base.prg_rom[addr - 0x8000 + mapper->prg_bank % mapper->base.prg_rom_size * 0x4000];
    else
        return mapper->base.prg_rom[addr - 0xC000 + (mapper->base.prg_rom_size - 1) * 0x4000];
}

void UxROM_cpu_write(UxROM *mapper, u16 addr, u8 data) {
    mapper->prg_bank = data;
}

void UxROM_init(UxROM *mapper) {
    mapper->base.cpu_read = &UxROM_cpu_read;
    mapper->base.cpu_write = &UxROM_cpu_write;
    mapper->base.ppu_read = &basic_ppu_read;
    mapper->base.ppu_write = &dummy_write;
    mapper->base.copy = &UxROM_copy;
}

void CNROM_cpu_write(CNROM *mapper, u16 addr, u8 data) {
    mapper->chr_bank = data;
}

u8 CNROM_ppu_read(CNROM *mapper, u16 addr) {
    return mapper->base.chr_rom[addr + mapper->chr_bank % mapper->base.chr_rom_size * 0x2000];
}

void CNROM_init(CNROM *mapper) {
    mapper->base.cpu_read = &basic_cpu_read;
    mapper->base.cpu_write = &CNROM_cpu_write;
    mapper->base.ppu_read = &CNROM_ppu_read;
    mapper->base.ppu_write = &dummy_write;
    mapper->base.copy = &CNROM_copy;
}

u8 MMC3_cpu_read(MMC3 *mapper, u16 addr) {
    u8 prg_bank;
    if (addr < 0xA000)
        prg_bank = mapper->prg_mode ? 2 * mapper->base.prg_rom_size - 2 : mapper->reg[6];
    else if (addr < 0xC000)
        prg_bank = mapper->reg[7];
    else if (addr < 0xE000)
        prg_bank = mapper->prg_mode ? mapper->reg[6] : 2 * mapper->base.prg_rom_size - 2;
    else
        prg_bank = 2 * mapper->base.prg_rom_size - 1;
    return mapper->base.prg_rom[(addr & 0x1FFF) + prg_bank % (2 * mapper->base.prg_rom_size) * 0x2000];
}

void MMC3_cpu_write(MMC3 *mapper, u16 addr, u8 data) {
    if (addr < 0xA000) {
        if (!(addr & 1)) {
            mapper->bank_select = data & 0x07;
            mapper->prg_mode = data & 0x40;
            mapper->chr_mode = data & 0x80;
        } else {
            mapper->reg[mapper->bank_select] = data;
        }
    } else if (addr < 0xC000) {
        if (!(addr & 1)) {
            mapper->base.mirroring = data & 0x01 ? MIRR_HORIZONTAL : MIRR_VERTICAL;
        } else {
            // TODO RAM protection
        }
    } else if (addr < 0xE000) {
        // TODO IRQ
    } else {
        // TODO IRQ
    }
}

u8 MMC3_ppu_read(MMC3 *mapper, u16 addr) {
    if (mapper->chr_mode)
        addr ^= 0x1000;
    if (!(addr & 0x1000))
        return mapper->base.chr_rom
            [(addr & 0x07FF) + (mapper->reg[(addr & 0x0800) >> 11] & 0xFE) % (8 * mapper->base.chr_rom_size) * 0x0400];
    else
        return mapper->base.chr_rom
            [(addr & 0x03FF) + mapper->reg[((addr & 0x0C00) >> 10) + 2] % (8 * mapper->base.chr_rom_size) * 0x0400];
}

void MMC3_init(MMC3 *mapper) {
    mapper->base.cpu_read = &MMC3_cpu_read;
    mapper->base.cpu_write = &MMC3_cpu_write;
    mapper->base.ppu_read = &MMC3_ppu_read;
    mapper->base.ppu_write = &dummy_write;
    mapper->base.copy = &MMC3_copy;
}
