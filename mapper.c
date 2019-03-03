#include "global.h"
#include "mapper.h"

void dummy_write(Mapper *mapper, u16 addr, u8 data) {
}

u8 NROM128_cpu_read(NROM128 *mapper, u16 addr, u8 data_bus) {
    if (addr < 0x8000)
        return data_bus;
    else if (addr < 0xC000)
        return mapper->prg_rom[addr - 0x8000];
    else
        return mapper->prg_rom[addr - 0xC000];
}

u8 NROM128_ppu_read(NROM128 *mapper, u16 addr) {
    return mapper->chr_rom[addr];
}

Mirroring NROM128_mirroring(NROM128 *mapper) {
    switch (mapper->mirroring) {
    case 0:
        return MIRR_HORIZONTAL;
    case 1:
        return MIRR_VERTICAL;
    }
}

NROM128 *new_NROM128(void) {
    NROM128 *mapper = s_malloc(sizeof(NROM128));
    mapper->base.cpu_read = &NROM128_cpu_read;
    mapper->base.cpu_write = &dummy_write;
    mapper->base.ppu_read = &NROM128_ppu_read;
    mapper->base.ppu_write = &dummy_write;
    mapper->base.mirroring = &NROM128_mirroring;
    return mapper;
}

u8 NROM256_cpu_read(NROM256 *mapper, u16 addr, u8 data_bus) {
    if (addr < 0x8000)
        return data_bus;
    else
        return mapper->prg_rom[addr - 0x8000];
}

u8 NROM256_ppu_read(NROM256 *mapper, u16 addr) {
    return mapper->chr_rom[addr];
}

Mirroring NROM256_mirroring(NROM128 *mapper) {
    switch (mapper->mirroring) {
    case 0:
        return MIRR_HORIZONTAL;
    case 1:
        return MIRR_VERTICAL;
    }
}

NROM256 *new_NROM256(void) {
    NROM256 *mapper = s_malloc(sizeof(NROM256));
    mapper->base.cpu_read = &NROM256_cpu_read;
    mapper->base.cpu_write = &dummy_write;
    mapper->base.ppu_read = &NROM256_ppu_read;
    mapper->base.ppu_write = &dummy_write;
    mapper->base.mirroring = &NROM256_mirroring;
    return mapper;
}
