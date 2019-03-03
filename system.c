#include <string.h>
#include <stdlib.h>

#include "global.h"
#include "system.h"

System *new_System(Mapper *mapper) {
    System *sys = s_malloc(sizeof(System));
    sys->mapper = mapper;
    sys->PC = cpu_read(sys, 0xFFFD) << 8 | cpu_read(sys, 0xFFFC);
    sys->A = 0x00;
    sys->X = 0x00;
    sys->Y = 0x00;
    sys->S = 0xFD;
    sys->C = false;
    sys->Z = false;
    sys->I = true;
    sys->D = false;
    sys->V = false;
    sys->N = false;
    sys->data = 0x00;
    sys->state = XXX_0;
    sys->op_ = 0xFF;
    sys->addr_ = 0x00;
    sys->ptr_ = 0x00;
    sys->OAMDMA_state = 512;
    sys->NMI_detected = false;
    sys->scanline = 0;
    sys->pixel = 0;
    sys->NMI_output = false;
    sys->PPUSTATUS = 0x00;
    sys->show_bg = false;
    sys->show_sp = false;
    sys->PPUDATA_increment = false;
    sys->NMI_occured = false;
    sys->PPU_v = 0x0000;
    sys->PPU_t = 0x0000;
    sys->PPU_x = 0x00;
    sys->PPU_w = false;
    for (int i = 0; i < 0x800; i++)
        sys->VRAM[i] = rand();
    for (int i = 0; i < 0x20; i++)
        sys->pal_RAM[i] = rand();
    for (int i = 0; i < 0x100; i++)
        sys->OAM[i] = rand();
    return sys;
}

u8 cpu_read(System *sys, u16 addr) {
    if (addr < 0x2000)
        return sys->RAM[addr % 0x800];
    else if (addr < 0x4000) {
        switch (addr % 0x8) {
        case 2:
            sys->PPU_bus = (sys->PPU_bus & 0x1F) | (sys->PPUSTATUS & 0xE0);
            sys->PPUSTATUS &= 0x60;
            sys->NMI_occured = false;
            sys->PPU_w = false;
            break;
        case 7:
            sys->PPU_bus = sys->PPUDATA_read_buffer;
            sys->PPUDATA_read_buffer = ppu_read(sys, sys->PPU_v);
            if ((sys->PPU_v & 0x3FFF) >= 0x3F00)
                sys->PPU_bus = sys->PPUDATA_read_buffer;
            sys->PPU_v += sys->PPUDATA_increment ? 0x20 : 0x1;
            break;
        }
        return sys->PPU_bus;
    } else if (addr < 0x4018) {
        switch (addr) {
        case 0x4016:
            sys->data = (sys->data & 0xE0) | ((sys->controller_shift & 0x80) >> 7);
            sys->controller_shift = sys->controller_shift << 1 | 1;
        }
        return sys->data;
    } else
        return sys->mapper->cpu_read(sys->mapper, addr, sys->data);
}

void cpu_write(System *sys, u16 addr, u8 data) {
    if (addr < 0x2000)
        sys->RAM[addr % 0x800] = data;
    else if (addr < 0x4000) {
        sys->PPU_bus = data;
        switch (addr % 0x8) {
        case 0:
            sys->PPU_t = (sys->PPU_t & 0x73FF) | ((data & 0x03) << 10);
            sys->PPUDATA_increment = data & 0x04;
            sys->sp_table = data & 0x08;
            sys->bg_table = data & 0x10;
            sys->tall_sprites = data & 0x20;
            if (data & 0x40)
                eprintln("Warning: PPUCTRL bit 6 set");
            sys->NMI_output = data & 0x80;
            break;
        case 1:
            sys->show_bg = data & 0x08;
            sys->show_sp = data & 0x10;
            break;
        case 3:
            sys->OAMADDR = data;
            break;
        case 4:
            sys->OAM[(u8)sys->OAMADDR] = data;
            break;
        case 5:
            switch (sys->PPU_w) {
            case false:
                sys->PPU_x = data & 0x07;
                sys->PPU_t = (sys->PPU_t & 0x7FE0) | (data >> 3);
                break;
            case true:
                sys->PPU_t = (sys->PPU_t & 0x0C1F) | ((data << 12) & 0x7000) | ((data << 2) & 0x03E0);
                break;
            }
            sys->PPU_w = !sys->PPU_w;
            break;
        case 6:
            switch (sys->PPU_w) {
            case false:
                sys->PPU_t = (sys->PPU_t & 0x00FF) | ((data << 8) & 0x3F00);
                break;
            case true:
                sys->PPU_t = (sys->PPU_t & 0xFF00) | data;
                sys->PPU_v = sys->PPU_t;
                break;
            }
            sys->PPU_w = !sys->PPU_w;
            break;
        case 7:
            ppu_write(sys, sys->PPU_v, data);
            sys->PPU_v += sys->PPUDATA_increment ? 0x20 : 0x1;
            break;
        }
    } else if (addr < 0x4018)
        switch (addr) {
        case 0x4014:
            sys->OAMDMA_state = -1;
            sys->OAMDMA_addr = data << 8;
            break;
        case 0x4016:
            sys->strobe = data & 0x01;
            break;
        }
    else
        sys->mapper->cpu_write(sys->mapper, addr, data);
}

u8 ppu_read(System *sys, u16 addr) {
    addr &= 0x3FFF;
    if (addr < 0x2000)
        return sys->mapper->ppu_read(sys->mapper, addr);
    else if (addr < 0x3F00) {
        switch (sys->mapper->mirroring(sys->mapper)) {
        case MIRR_HORIZONTAL:
            addr = (addr & 0x03FF) | ((addr >> 1) & 0x0400);
            break;
        case MIRR_VERTICAL:
            addr &= 0x07FF;
            break;
        }
        return sys->VRAM[addr];
    } else {
        if ((addr & 0x0003) == 0)
            addr &= 0xFFEF;
        return sys->pal_RAM[addr % 0x20];
    }
}

void ppu_write(System *sys, u16 addr, u8 data) {
    addr &= 0x3FFF;
    if (addr < 0x2000)
        sys->mapper->ppu_write(sys->mapper, addr, data);
    else if (addr < 0x3F00) {
        switch (sys->mapper->mirroring(sys->mapper)) {
        case MIRR_HORIZONTAL:
            addr = (addr & 0x03FF) | ((addr >> 1) & 0x0400);
            break;
        case MIRR_VERTICAL:
            addr &= 0x07FF;
            break;
        }
        sys->VRAM[addr] = data;
    } else {
        if ((addr & 0x0003) == 0)
            addr &= 0xFFEF;
        sys->pal_RAM[addr % 0x20] = data;
    }
}
