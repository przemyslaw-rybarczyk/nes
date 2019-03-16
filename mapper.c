#include "global.h"
#include "mapper.h"

void dummy_write(Mapper *mapper, u16 addr, u8 data) {
}

u8 NROM_cpu_read(NROM *mapper, u16 addr, u8 data_bus) {
    return mapper->base.prg_rom[(addr - 0x8000) % (mapper->base.prg_rom_size * 0x4000)];
}

u8 basic_ppu_read(Mapper *mapper, u16 addr) {
    return mapper->chr_rom[addr];
}

void basic_ppu_write(MMC1 *mapper, u16 addr, u8 data) {
    if (mapper->base.chr_rom_size == 0)
        mapper->base.chr_rom[addr] = data;
}

void NROM_init(NROM *mapper) {
    mapper->base.cpu_read = &NROM_cpu_read;
    mapper->base.cpu_write = &dummy_write;
    mapper->base.ppu_read = &basic_ppu_read;
    mapper->base.ppu_write = &basic_ppu_write;
}

u8 MMC1_cpu_read(MMC1 *mapper, u16 addr, u8 data_bus) {
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
    mapper->base.ppu_write = &basic_ppu_write;
    mapper->prg_rom_bank_mode = 3;
}

u8 UxROM_cpu_read(UxROM *mapper, u16 addr, u8 data_bus) {
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
    mapper->base.ppu_write = &basic_ppu_write;
}
