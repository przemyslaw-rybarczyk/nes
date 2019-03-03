#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "SDL.h"
#include "SDL_render.h"
#include "SDL_keyboard.h"

#include "global.h"
#include "mapper.h"
#include "system.h"
#include "cpu.h"
#include "ppu.h"

void s_fread(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    if (fread(ptr, size, nmemb, stream) < nmemb) {
        if (feof(stream))
            eprintln("Error: premature end of file");
        else
            eprintln("Error reading file");
        exit(1);
    }
}

int key_mask(SDL_Scancode sc) {
    switch (sc) {
    case SDL_SCANCODE_K:
        return 0x80;
    case SDL_SCANCODE_J:
        return 0x40;
    case SDL_SCANCODE_U:
        return 0x20;
    case SDL_SCANCODE_I:
        return 0x10;
    case SDL_SCANCODE_W:
        return 0x08;
    case SDL_SCANCODE_S:
        return 0x04;
    case SDL_SCANCODE_A:
        return 0x02;
    case SDL_SCANCODE_D:
        return 0x01;
    default:
        return 0x00;
    }
}

char header[16];

System *saved_state;

int main(int argc, char **argv) {
    srand(time(NULL));
    saved_state = malloc(sizeof(System));
    if (argc != 2) {
        eprintln("Error: invalid number of arguments");
        exit(1);
    }
    char *path = argv[1];
    FILE *f = fopen(path, "rb");
    if (f == NULL) {
        eprintln("Error opening file (errno %d)", errno);
        exit(1);
    }
    s_fread(header, 1, 16, f);
    if (strncmp(header, "NES\x1A", 4) != 0) {
        eprintln("Error: not a valid NES ROM file");
        exit(1);
    }
    u8 mapper_num = (header[7] & 0xF0) | (header[6] >> 4);
    u8 prg_rom_size = header[4];
    Mapper *mapper;
    switch (mapper_num) {
    case 0: {
        bool mirroring = header[6] & 0x01 ? MIRR_VERTICAL : MIRR_HORIZONTAL;
        switch (prg_rom_size) {
        case 1: {
            NROM128 *mapper_ = new_NROM128();
            s_fread(&mapper_->prg_rom, 1, 0x4000, f);
            s_fread(&mapper_->chr_rom, 1, 0x2000, f);
            mapper_->mirroring = mirroring;
            mapper = mapper_;
            break;
        }
        case 2: {
            NROM256 *mapper_ = new_NROM256();
            s_fread(&mapper_->prg_rom, 1, 0x8000, f);
            s_fread(&mapper_->chr_rom, 1, 0x2000, f);
            mapper_->mirroring = mirroring;
            mapper = mapper_;
            break;
        }
        default:
            eprintln("Error: invalid PRG ROM size for mapper 0");
            exit(1);
        }
        break;
    }
    default:
        eprintln("Error: unsupported mapper (%d)", mapper_num);
        exit(1);
    }
    if (fclose(f) != 0)
        eprintln("Error closing file");
    System *sys = new_System(mapper);
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window *win = SDL_CreateWindow("NES emulator",
            SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 256, 240, SDL_WINDOW_SHOWN);
    SDL_Renderer *renderer = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    SDL_RenderClear(renderer);
    SDL_Event e;
    while (true) {
        cpu_step(sys);
        ppu_step(sys, renderer);
        ppu_step(sys, renderer);
        ppu_step(sys, renderer);
        if (sys->strobe)
            sys->controller_shift = controller_state;
        if (sys->scanline == 261 && sys->pixel >= 338) {
            while (SDL_PollEvent(&e)) {
                switch (e.type) {
                case SDL_QUIT:
                    exit(0);
                case SDL_KEYUP:
                    controller_state &= ~key_mask(e.key.keysym.scancode);
                    break;
                case SDL_KEYDOWN:
                    controller_state |= key_mask(e.key.keysym.scancode);
                    switch (e.key.keysym.scancode) {
                    case SDL_SCANCODE_Q:
                        *saved_state = *sys;
                        break;
                    case SDL_SCANCODE_E:
                        *sys = *saved_state;
                        break;
                    default:
                        break;
                    }
                    break;
                }
            }
            SDL_RenderPresent(renderer);
            SDL_RenderClear(renderer);
        }
    }
}
