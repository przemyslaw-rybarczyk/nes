#include <stdio.h>

#include "SDL_render.h"

#include "global.h"
#include "ppu.h"
#include "system.h"

typedef struct RGB_Color {
    u8 r;
    u8 g;
    u8 b;
} RGB_Color;

RGB_Color rgb_colors[64] = {
    { 84,  84,  84},
    {  0,  30, 116},
    {  8,  16, 144},
    { 48,   0, 136},
    { 68,   0, 100},
    { 92,   0,  48},
    { 84,   4,   0},
    { 60,  24,   0},
    { 32,  42,   0},
    {  8,  58,   0},
    {  0,  64,   0},
    {  0,  60,   0},
    {  0,  50,  60},
    {  0,   0,   0},
    {  0,   0,   0},
    {  0,   0,   0},
    {152, 150, 152},
    {  8,  76, 196},
    { 48,  50, 236},
    { 92,  30, 228},
    {136,  20, 176},
    {160,  20, 100},
    {152,  34,  32},
    {120,  60,   0},
    { 84,  90,   0},
    { 40, 114,   0},
    {  8, 124,   0},
    {  0, 118,  40},
    {  0, 102, 120},
    {  0,   0,   0},
    {  0,   0,   0},
    {  0,   0,   0},
    {236, 238, 236},
    { 76, 154, 236},
    {120, 124, 236},
    {176,  98, 236},
    {228,  84, 236},
    {236,  88, 180},
    {236, 106, 100},
    {212, 136,  32},
    {160, 170,   0},
    {116, 196,   0},
    { 76, 208,  32},
    { 56, 204, 108},
    { 56, 180, 204},
    { 60,  60,  60},
    {  0,   0,   0},
    {  0,   0,   0},
    {236, 238, 236},
    {168, 204, 236},
    {188, 188, 236},
    {212, 178, 236},
    {236, 174, 236},
    {236, 174, 212},
    {236, 180, 176},
    {228, 196, 144},
    {204, 210, 120},
    {180, 222, 120},
    {168, 226, 144},
    {152, 226, 180},
    {160, 214, 228},
    {160, 162, 160},
    {  0,   0,   0},
    {  0,   0,   0},
};

u8 flip_bits(u8 i) {
    i = (i & 0xF0) >> 4 | (i & 0x0F) << 4;
    i = (i & 0xCC) >> 2 | (i & 0x33) << 2;
    i = (i & 0xAA) >> 1 | (i & 0x55) << 1;
    return i;
}

void ppu_step(System *sys, SDL_Renderer *renderer) {
    if (sys->scanline < 240 && sys->pixel > 0 && sys->pixel <= 256 && !(sys->show_bg || sys->show_sp)) {
        u16 color = sys->PPU_v >= 0x3F00 ? sys->PPU_v : 0x3F00;
        RGB_Color rgb_color = rgb_colors[ppu_read(sys, color) & 0x3F];
        SDL_SetRenderDrawColor(renderer, rgb_color.r, rgb_color.g, rgb_color.b, 255);
        SDL_RenderDrawPoint(renderer, sys->pixel - 1, sys->scanline);
    }
    if (sys->scanline < 240 && sys->pixel > 0 && sys->pixel <= 256 && (sys->show_bg || sys->show_sp)) {
        u8 bg_pattern = (sys->patt_high >> (15 - sys->PPU_x) & 1) << 1 | (sys->patt_low >> (15 - sys->PPU_x) & 1);
        u8 attr = (sys->attr_high >> (7 - sys->PPU_x) & 1) << 1 | (sys->attr_low >> (7 - sys->PPU_x) & 1);
        u16 color = bg_pattern == 0 ? 0x3F00 : 0x3F00 | attr << 2 | bg_pattern;
        bool drew_sprite = false;
        sys->patt_low <<= 1;
        sys->patt_high <<= 1;
        sys->attr_low = sys->attr_low << 1 | sys->attr_latch_low;
        sys->attr_high = sys->attr_high << 1 | sys->attr_latch_high;
        for (int i = 0; i < 8; i++) {
            if (sys->sprite_x[i] == 0) {
                u8 pattern = (sys->sprite_patt_high[i] & 0x80) >> 6 | (sys->sprite_patt_low[i] & 0x80) >> 7;
                if (pattern != 0 && !drew_sprite) {
                    drew_sprite = true;
                    if (sys->sprite0 && i == 0)
                        sys->PPUSTATUS |= 0x40;
                    if ((sys->sprite_attr[i] & 0x20) == 0 || bg_pattern == 0)
                        color = 0x3F10 | (sys->sprite_attr[i] & 0x03) << 2 | pattern;
                }
                sys->sprite_patt_low[i] <<= 1;
                sys->sprite_patt_high[i] <<= 1;
            } else {
                sys->sprite_x[i]--;
            }
        }
        RGB_Color rgb_color = rgb_colors[ppu_read(sys, color) & 0x3F];
        SDL_SetRenderDrawColor(renderer, rgb_color.r, rgb_color.g, rgb_color.b, 255);
        SDL_RenderDrawPoint(renderer, sys->pixel - 1, sys->scanline);
    }
    if ((sys->scanline < 240 || sys->scanline == 261) && (sys->pixel == 328 || sys->pixel == 336) && (sys->show_bg || sys->show_sp)) {
        sys->patt_low <<= 8;
        sys->patt_high <<= 8;
        sys->attr_low = sys->attr_latch_low * 0xFF;
        sys->attr_high = sys->attr_latch_high * 0xFF;
    }
    if ((sys->scanline < 240 || sys->scanline == 261)
            && ((sys->pixel > 0 && sys->pixel <= 256) || (sys->pixel >= 321 && sys->pixel <= 336))
            && (sys->show_bg || sys->show_sp)) {
        switch (sys->pixel % 8) {
        case 2:
            sys->tile_fetch = ppu_read(sys, (sys->PPU_v & 0x0FFF) | 0x2000);
            break;
        case 4: {
            u16 attr_addr = 0x23C0 | (sys->PPU_v & 0x0C00) | ((sys->PPU_v & 0x0380) >> 4) | ((sys->PPU_v & 0x001C) >> 2);
            int attr_bit = ((sys->PPU_v & 0x0040) >> 4) | (sys->PPU_v & 0x0002);
            u8 attr = ppu_read(sys, attr_addr);
            sys->attr_fetch_low = (attr >> attr_bit) & 0x01;
            sys->attr_fetch_high = (attr >> attr_bit) & 0x02;
            break;
        }
        case 6: {
            u16 patt_addr = (sys->bg_table << 12) | (sys->tile_fetch << 4) | (sys->PPU_v >> 12);
            sys->patt_fetch_low = ppu_read(sys, patt_addr);
            break;
        }
        case 0: {
            u16 patt_addr = (sys->bg_table << 12) | 0x0008 | (sys->tile_fetch << 4) | (sys->PPU_v >> 12);
            sys->attr_latch_low = sys->attr_fetch_low;
            sys->attr_latch_high = sys->attr_fetch_high;
            sys->patt_low |= sys->patt_fetch_low;
            sys->patt_high |= ppu_read(sys, patt_addr);
            if (sys->show_bg || sys->show_sp) {
                if (sys->pixel != 256) {
                    if ((sys->PPU_v & 0x001F) == 0x001F) {
                        sys->PPU_v &= 0xFFE0;
                        sys->PPU_v ^= 0x0400;
                    } else {
                        sys->PPU_v++;
                    }
                } else {
                    if ((sys->PPU_v & 0x7000) == 0x7000) {
                        if ((sys->PPU_v & 0x03E0) == 0x03A0) {
                            sys->PPU_v &= 0x0C1F;
                            sys->PPU_v ^= 0x0800;
                        } else if ((sys->PPU_v & 0x03E0) == 0x03E0) {
                            sys->PPU_v &= 0x0C1F;
                        } else {
                            sys->PPU_v &= 0x0FFF;
                            sys->PPU_v += 0x20;
                        }
                    } else {
                        sys->PPU_v += 0x1000;
                    }
                }
            }
            break;
        }
        }
    }
    if ((sys->scanline < 240 || sys->scanline == 261) && sys->pixel == 257 && (sys->show_bg || sys->show_sp))
            sys->PPU_v = (sys->PPU_v & 0x7BE0) | (sys->PPU_t & 0x041F);
    if (sys->scanline == 241 && sys->pixel == 1) {
        sys->PPUSTATUS |= 0x80;
        if (sys->NMI_output)
            sys->NMI_occured = true;
    }
    if (sys->scanline == 261 && sys->pixel >= 280 && sys->pixel <= 304 && (sys->show_bg || sys->show_sp))
            sys->PPU_v = (sys->PPU_v & 0x041F) | (sys->PPU_t & 0x7BE0);
    if (sys->scanline < 240 && sys->pixel > 0 && sys->pixel <= 64 && sys->pixel % 2 == 0)
        sys->OAM2[sys->pixel / 2 - 1] = 0xFF;
    if ((sys->scanline < 240 || sys->scanline == 261) && sys->pixel > 256 && sys->pixel <= 320) {
        sys->OAMADDR = 0x00;
        sys->OAM2_addr = 0x00;
    }
    if (sys->scanline < 240 && sys->pixel > 64 && sys->pixel <= 256 && sys->pixel % 2 == 1 && (sys->show_bg || sys->show_sp)) {
        if (sys->pixel == 65)
            sys->sprite0_next = false;
        if (sys->OAMADDR < 0x100) {
            u8 OAM_fetch = sys->OAM[sys->OAMADDR];
            if (sys->OAM2_addr < 0x20) {
                sys->OAM2[sys->OAM2_addr] = OAM_fetch;
                if ((sys->OAM2_addr & 0x03) == 0) {
                    if (OAM_fetch <= sys->scanline && sys->scanline < OAM_fetch + (sys->tall_sprites ? 16 : 8)) {
                        if (sys->pixel == 65)
                            sys->sprite0_next = true;
                        sys->OAMADDR++;
                        sys->OAM2_addr++;
                    } else {
                        sys->OAMADDR += 4;
                    }
                } else {
                    sys->OAMADDR++;
                    sys->OAM2_addr++;
                }
            } else {
                if (sys->PPU_m1 == 0) {
                    if (OAM_fetch <= sys->scanline && sys->scanline < OAM_fetch + (sys->tall_sprites ? 16 : 8)) {
                        sys->PPUSTATUS |= 0x20;
                        sys->OAMADDR++;
                        sys->PPU_m1 = (sys->PPU_m1 + 1) % 4;
                    } else {
                        sys->OAMADDR += (sys->OAMADDR & 0x03) == 0x03 ? 1 : 5;
                    }
                } else {
                    sys->OAMADDR++;
                    sys->PPU_m1 = (sys->PPU_m1 + 1) % 4;
                }
            }
        } else {
            sys->OAMADDR += 4;
        }
    }
    if (sys->scanline < 240 && sys->pixel == 257)
        sys->sprite0 = sys->sprite0_next;
    if (sys->scanline < 240 && sys->pixel > 256 && sys->pixel <= 320) {
        int i = (sys->pixel - 257) / 8;
        u16 x_pos = sys->OAM2[4 * i + 2] & 0x80
            ? sys->OAM2[4 * i] + (sys->tall_sprites ? 15 : 7) - sys->scanline
            : sys->scanline - sys->OAM2[4 * i];
        u16 patt_addr = sys->tall_sprites
            ? ((sys->OAM2[4 * i + 1] & 0x01) << 12) | ((sys->OAM2[4 * i + 1] & 0xFE) << 4) | ((x_pos & 8) << 1) | (x_pos & 7)
            : (sys->sp_table << 12) | (sys->OAM2[4 * i + 1] << 4) | x_pos;
        bool in_range = x_pos < (sys->tall_sprites ? 16 : 8);
        switch (sys->pixel % 8) {
        case 6: {
            sys->sprite_attr[i] = sys->OAM2[4 * i + 2];
            sys->sprite_x[i] = sys->OAM2[4 * i + 3];
            u8 patt = in_range ? ppu_read(sys, patt_addr) : 0x00;
            sys->sprite_patt_low[i] = sys->sprite_attr[i] & 0x40 ? flip_bits(patt) : patt;
            break;
        }
        case 0: {
            patt_addr |= 0x0008;
            u8 patt = in_range ? ppu_read(sys, patt_addr) : 0x00;
            sys->sprite_patt_high[i] = sys->sprite_attr[i] & 0x40 ? flip_bits(patt) : patt;
            break;
        }
        }
    }
    if (sys->scanline == 261 && sys->pixel == 1) {
        sys->PPUSTATUS = 0x00;
        sys->sprite0 = false;
    }
    sys->pixel++;
    if (sys->pixel > 340) {
        sys->pixel = 0;
        sys->scanline++;
        if (sys->scanline > 261)
            sys->scanline = 0;
    }
}
