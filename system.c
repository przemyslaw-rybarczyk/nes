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
    sys->APU_cycle_counter = 0;
    for (int i = 0; i < 0x800; i++)
        sys->VRAM[i] = rand();
    for (int i = 0; i < 0x20; i++)
        sys->pal_RAM[i] = rand();
    for (int i = 0; i < 0x100; i++)
        sys->OAM[i] = rand();
    return sys;
}

u8 length_counter_load[32] = {
     10, 254,  20,   2,  40,   4,  80,   6, 160,   8,  60,  10,  14,  12,  26,  14,
     12,  16,  24,  18,  48,  20,  96,  22, 192,  24,  72,  26,  16,  28,  32,  30,
};

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
    } else if (addr < 0x4020) {
        switch (addr) {
        case 0x4015:
            sys->data = 
                  (sys->triangle_length_counter > 0) << 2
                | (sys->pulse2_length_counter > 0) << 1
                | (sys->pulse1_length_counter > 0) << 0;
            break;
        case 0x4016:
            sys->data = (sys->data & 0xE0) | ((sys->controller_shift & 0x80) >> 7);
            sys->controller_shift = sys->controller_shift << 1 | 1;
            break;
        }
        return sys->data;
    } else if (addr >= 0x6000 && addr < 0x8000)
        return sys->WRAM[addr - 0x6000];
    else
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
    } else if (addr < 0x4020) {
        switch (addr) {
        case 0x4000:
            sys->pulse1_duty = data >> 6;
            sys->pulse1_halt = data & 0x20;
            sys->pulse1_const_volume = data & 0x10;
            sys->pulse1_volume = data & 0x0F;
            break;
        case 0x4001:
            sys->pulse1_sweep.enable = data & 0x80;
            sys->pulse1_sweep.period = (data & 0x70) >> 4;
            sys->pulse1_sweep.negate = data & 0x08;
            sys->pulse1_sweep.shift = data & 0x07;
            sys->pulse1_sweep.reload = true;
            break;
        case 0x4002:
            sys->pulse1_period = (sys->pulse1_period & 0x700) | data;
            break;
        case 0x4003:
            sys->pulse1_length_counter = length_counter_load[data >> 3];
            sys->pulse1_period = (sys->pulse1_period & 0x0FF) | (data & 0x03) << 8;
            sys->pulse1_sequencer = 0;
            sys->pulse1_envelope.start = true;
            break;
        case 0x4004:
            sys->pulse2_duty = data >> 6;
            sys->pulse2_halt = data & 0x20;
            sys->pulse2_const_volume = data & 0x10;
            sys->pulse2_volume = data & 0x0F;
            break;
        case 0x4005:
            sys->pulse2_sweep.enable = data & 0x80;
            sys->pulse2_sweep.period = (data & 0x70) >> 4;
            sys->pulse2_sweep.negate = data & 0x08;
            sys->pulse2_sweep.shift = data & 0x07;
            sys->pulse2_sweep.reload = true;
            break;
        case 0x4006:
            sys->pulse2_period = (sys->pulse2_period & 0x700) | data;
            break;
        case 0x4007:
            sys->pulse2_length_counter = length_counter_load[data >> 3];
            sys->pulse2_period = (sys->pulse2_period & 0x0FF) | (data & 0x03) << 8;
            sys->pulse2_sequencer = 0;
            sys->pulse2_envelope.start = true;
            break;
        case 0x4008:
            sys->triangle_halt = data & 0x80;
            sys->triangle_counter_reload = data & 0x7F;
            break;
        case 0x400A:
            sys->triangle_period = (sys->triangle_period & 0x700) | data;
            break;
        case 0x400B:
            sys->triangle_length_counter = length_counter_load[data >> 3];
            sys->triangle_period = (sys->triangle_period & 0x0FF) | (data & 0x03) << 8;
            sys->triangle_reload = true;
            break;
        case 0x4014:
            sys->OAMDMA_state = -1;
            sys->OAMDMA_addr = data << 8;
            break;
        case 0x4015:
            sys->triangle_enable = data & 0x04;
            if (!sys->triangle_enable)
                sys->triangle_length_counter = 0;
            sys->pulse2_enable = data & 0x02;
            if (!sys->pulse2_enable)
                sys->pulse2_length_counter = 0;
            sys->pulse1_enable = data & 0x01;
            if (!sys->pulse1_enable)
                sys->pulse1_length_counter = 0;
            break;
        case 0x4016:
            sys->strobe = data & 0x01;
            break;
        case 0x4017:
            sys->APU_sequencer_mode = data & 0x80;
            sys->APU_cycle_counter = sys->APU_sequencer_mode ? 37281 : 0;
            sys->APU_interrupt_inhibit = data & 0x40;
            if (sys->APU_interrupt_inhibit)
                sys->APU_interrupt = false;
            break;
        }
    } else if (addr >= 0x6000 && addr < 0x8000)
        sys->WRAM[addr - 0x6000] = data;
    else
        sys->mapper->cpu_write(sys->mapper, addr, data);
}

u16 decode_VRAM_addr(u16 addr, Mirroring mirroring) {
    switch (mirroring) {
    case MIRR_ONE_SCREEN_LOW:
        return addr & 0x03FF;
    case MIRR_ONE_SCREEN_HIGH:
        return (addr & 0x03FF) | 0x0400;
    case MIRR_HORIZONTAL:
        return (addr & 0x03FF) | ((addr >> 1) & 0x0400);
    case MIRR_VERTICAL:
        return addr & 0x07FF;
    }
}

u8 ppu_read(System *sys, u16 addr) {
    addr &= 0x3FFF;
    if (addr < 0x2000)
        return sys->mapper->ppu_read(sys->mapper, addr);
    else if (addr < 0x3F00)
        return sys->VRAM[decode_VRAM_addr(addr, sys->mapper->mirroring)];
    else {
        if ((addr & 0x0003) == 0)
            addr &= 0xFFEF;
        return sys->pal_RAM[addr % 0x20];
    }
}

void ppu_write(System *sys, u16 addr, u8 data) {
    addr &= 0x3FFF;
    if (addr < 0x2000)
        sys->mapper->ppu_write(sys->mapper, addr, data);
    else if (addr < 0x3F00)
        sys->VRAM[decode_VRAM_addr(addr, sys->mapper->mirroring)] = data;
    else {
        if ((addr & 0x0003) == 0)
            addr &= 0xFFEF;
        sys->pal_RAM[addr % 0x20] = data;
    }
}
