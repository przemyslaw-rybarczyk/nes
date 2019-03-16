#pragma once

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#define println(fmt, ...) printf(fmt "\n", ##__VA_ARGS__)
#define eprintf(...) fprintf(stderr, __VA_ARGS__)
#define eprintln(fmt, ...) fprintf(stderr, fmt "\n", ##__VA_ARGS__)

typedef uint8_t u8;
typedef uint16_t u16;
typedef int8_t i8;
typedef int16_t i16;

typedef enum Mirroring {
    MIRR_ONE_SCREEN_LOW,
    MIRR_ONE_SCREEN_HIGH,
    MIRR_HORIZONTAL,
    MIRR_VERTICAL,
} Mirroring;

typedef struct Mapper {
    u8 *prg_rom;
    u8 *chr_rom;
    u8 prg_rom_size;
    u8 chr_rom_size;
    u8 (*cpu_read)(struct Mapper *mapper, u16 addr, u8 data_bus);
    void (*cpu_write)(struct Mapper *mapper, u16 addr, u8 data_bus);
    u8 (*ppu_read)(struct Mapper *mapper, u16 addr);
    void (*ppu_write)(struct Mapper *mapper, u16 addr, u8 data_bus);
    Mirroring mirroring;
} Mapper;

typedef enum CPU_State {
    XXX_0,
    IMP_1,
    IMMr1,
    ZPGr1, ZPGr2,
    ZPXr1, ZPXr2, ZPXr3,
    ZPYr1, ZPYr2, ZPYr3,
    ABSr1, ABSr2, ABSr3,
    ABXr1, ABXr2, ABXr3, ABXr4,
    ABYr1, ABYr2, ABYr3, ABYr4,
    IZXr1, IZXr2, IZXr3, IZXr4, IZXr5,
    IZYr1, IZYr2, IZYr3, IZYr4, IZYr5,
    ZPGw1, ZPGw2,
    ZPXw1, ZPXw2, ZPXw3,
    ZPYw1, ZPYw2, ZPYw3,
    ABSw1, ABSw2, ABSw3,
    ABXw1, ABXw2, ABXw3, ABXw4,
    ABYw1, ABYw2, ABYw3, ABYw4,
    IZXw1, IZXw2, IZXw3, IZXw4, IZXw5,
    IZYw1, IZYw2, IZYw3, IZYw4, IZYw5,
    ZPGb1, ZPGb2, ZPGb3, ZPGb4,
    ZPXb1, ZPXb2, ZPXb3, ZPXb4, ZPXb5,
    ABSb1, ABSb2, ABSb3, ABSb4, ABSb5,
    ABXb1, ABXb2, ABXb3, ABXb4, ABXb5, ABXb6,
    ABSj1, ABSj2,
    INDj1, INDj2, INDj3, INDj4,
    REL_1, REL_2, REL_3,
    PHA_1, PHA_2,
    PHP_1, PHP_2,
    PLA_1, PLA_2, PLA_3,
    PLP_1, PLP_2, PLP_3,
    JSR_1, JSR_2, JSR_3, JSR_4, JSR_5,
    RTS_1, RTS_2, RTS_3, RTS_4, RTS_5,
    BRK_1, BRK_2, BRK_3, BRK_4, BRK_5, BRK_6,
    RTI_1, RTI_2, RTI_3, RTI_4, RTI_5,
    INV_x,
} CPU_State;

typedef struct APU_Sweep {
    bool enable;
    u8 period;
    bool negate;
    u8 shift;
    i16 target;
    u16 divider;
    bool reload;
} APU_Sweep;

typedef struct APU_Envelope {
    bool start;
    u8 divider;
    u8 decay;
} APU_Envelope;

typedef struct System {
    Mapper *mapper;
    // CPU
    u16 PC;
    u8 A;
    u8 X;
    u8 Y;
    u8 S;
    bool C;
    bool Z;
    bool I;
    bool D;
    bool V;
    bool N;
    u8 data;
    CPU_State state;
    u8 op_;
    u16 addr_;
    u16 ptr_;
    bool NMI_detected;
    i16 OAMDMA_state;
    u16 OAMDMA_addr;
    // PPU
    u16 scanline;
    u16 pixel;
    bool odd_frame;
    bool PPUDATA_increment;
    bool sp_table;
    bool bg_table;
    bool tall_sprites;
    bool NMI_output;
    bool grayscale;
    bool show_bg_left;
    bool show_sp_left;
    bool show_bg;
    bool show_sp;
    bool NMI_occured;
    u8 PPUSTATUS;
    u16 OAMADDR;
    u8 PPUDATA_read_buffer;
    u16 PPU_v;
    u16 PPU_t;
    u8 PPU_x;
    bool PPU_w;
    u8 tile_fetch;
    u16 patt_low;
    u16 patt_high;
    u8 patt_fetch_low;
    u8 attr_low;
    u8 attr_high;
    bool attr_latch_low;
    bool attr_latch_high;
    bool attr_fetch_low;
    bool attr_fetch_high;
    u8 OAM2_addr;
    u8 PPU_m1;
    u8 PPU_bus;
    u8 sprite0;
    u8 sprite0_next;
    // APU
    u16 APU_cycle_counter;
    bool APU_interrupt;
    bool APU_sequencer_mode;
    bool APU_interrupt_inhibit;
    bool pulse1_enable;
    u8 pulse1_duty;
    bool pulse1_halt;
    bool pulse1_const_volume;
    u8 pulse1_volume;
    APU_Sweep pulse1_sweep;
    u8 pulse1_sequencer;
    u16 pulse1_timer;
    u16 pulse1_period;
    u8 pulse1_length_counter;
    APU_Envelope pulse1_envelope;
    bool pulse2_enable;
    u8 pulse2_duty;
    bool pulse2_halt;
    bool pulse2_const_volume;
    u8 pulse2_volume;
    APU_Sweep pulse2_sweep;
    u8 pulse2_sequencer;
    u16 pulse2_timer;
    u16 pulse2_period;
    u8 pulse2_length_counter;
    APU_Envelope pulse2_envelope;
    bool triangle_enable;
    bool triangle_halt;
    u8 triangle_counter_reload;
    u8 triangle_counter;
    u8 triangle_sequencer;
    u16 triangle_timer;
    u16 triangle_period;
    u8 triangle_length_counter;
    bool triangle_reload;
    bool noise_enable;
    bool noise_halt;
    bool noise_const_volume;
    u8 noise_volume;
    bool noise_mode;
    u16 noise_timer;
    u16 noise_period;
    u16 noise_lfsr;
    u8 noise_length_counter;
    APU_Envelope noise_envelope;
    bool dmc_enable;
    bool dmc_loop;
    u16 dmc_period;
    u16 dmc_timer;
    u16 dmc_sample_address;
    u16 dmc_address;
    u16 dmc_sample_length;
    u16 dmc_length_counter;
    bool dmc_silence;
    u8 dmc_buffer;
    bool dmc_buffer_empty;
    u8 dmc_shift;
    u8 dmc_shift_bits;
    i8 dmc_output;
    // input
    bool strobe;
    u8 controller_shift;
    // memory
    u8 sprite_patt_low[8];
    u8 sprite_patt_high[8];
    u8 sprite_attr[8];
    u8 sprite_x[8];
    u8 OAM2[0x20];
    u8 OAM[0x100];
    u8 pal_RAM[0x20];
    u8 RAM[0x800];
    u8 VRAM[0x800];
    u8 WRAM[0x2000];
} System;

extern u8 controller_state;

void *s_malloc(size_t size);
