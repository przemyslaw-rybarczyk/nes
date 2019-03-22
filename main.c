#pragma GCC diagnostic ignored "-Wincompatible-pointer-types"

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "SDL.h"
#include "SDL_render.h"
#include "SDL_keyboard.h"
#include "soxr-lsr.h"

#include "global.h"
#include "mapper.h"
#include "system.h"
#include "cpu.h"
#include "ppu.h"
#include "apu.h"

#define CPU_CYCLE_RATE 1786830
#define SRC_DATA_IN_SIZE 4096
#define SRC_DATA_OUT_SIZE 256

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
float src_data_in[SRC_DATA_IN_SIZE];
float src_data_out[SRC_DATA_OUT_SIZE];

int main(int argc, char **argv) {
    srand(time(NULL));
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
    Mapper *mapper;
    Mapper *saved_mapper;
    switch (mapper_num) {
    case 0:
        mapper = s_malloc(sizeof(NROM));
        saved_mapper = s_malloc(sizeof(NROM));
        NROM_init(mapper);
        mapper->mirroring = header[6] & 0x01 ? MIRR_VERTICAL : MIRR_HORIZONTAL;
        break;
    case 1:
        mapper = s_malloc(sizeof(MMC1));
        saved_mapper = s_malloc(sizeof(MMC1));
        MMC1_init(mapper);
        break;
    case 2:
        mapper = s_malloc(sizeof(UxROM));
        saved_mapper = s_malloc(sizeof(UxROM));
        UxROM_init(mapper);
        mapper->mirroring = header[6] & 0x01 ? MIRR_VERTICAL : MIRR_HORIZONTAL;
        break;
    case 3:
        mapper = s_malloc(sizeof(CNROM));
        saved_mapper = s_malloc(sizeof(CNROM));
        CNROM_init(mapper);
        mapper->mirroring = header[6] & 0x01 ? MIRR_VERTICAL : MIRR_HORIZONTAL;
        break;
    case 4:
        mapper = s_malloc(sizeof(MMC3));
        saved_mapper = s_malloc(sizeof(MMC3));
        MMC3_init(mapper);
        break;
    default:
        eprintln("Error: unsupported mapper (%03d)", mapper_num);
        exit(1);
    }
    mapper->prg_rom_size = header[4];
    mapper->chr_rom_size = header[5];
    mapper->prg_rom = s_malloc(mapper->prg_rom_size * 0x4000);
    s_fread(mapper->prg_rom, 1, mapper->prg_rom_size * 0x4000, f);
    if (mapper->chr_rom_size > 0) {
        mapper->chr_rom = s_malloc(mapper->chr_rom_size * 0x2000);
        s_fread(mapper->chr_rom, 1, mapper->chr_rom_size * 0x2000, f);
    } else
        mapper->chr_rom = s_malloc(0x2000);
    if (fclose(f) != 0)
        eprintln("Error closing file");
    mapper->copy(mapper, saved_mapper);
    System *sys = new_System(mapper);
    System *saved_sys = s_malloc(sizeof(System));
    *saved_sys = *sys;
    saved_sys->mapper = saved_mapper;
    // TODO SDL error checking
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    SDL_Window *window = SDL_CreateWindow("NES emulator",
            SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 960, SDL_WINDOW_SHOWN);
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, 256, 240);
    uint32_t *pixels;
    int pitch;
    if (SDL_LockTexture(texture, NULL, &pixels, &pitch)) {
        eprintln("SDL error: %s", SDL_GetError());
        exit(1);
    }
    SDL_AudioSpec want, have;
    SDL_memset(&want, 0, sizeof(want));
    want.freq = 44100;
    want.format = AUDIO_F32;
    want.channels = 1;
    want.samples = 4096;
    want.callback = NULL;
    SDL_AudioDeviceID audio_dev;
    audio_dev = SDL_OpenAudioDevice(NULL, 0, &want, &have, SDL_AUDIO_ALLOW_FREQUENCY_CHANGE);
    if (audio_dev == 0) {
        eprintln("SDL error: %s", SDL_GetError());
        exit(1);
    }
    int src_err;
    SRC_STATE *src_state = src_new(SRC_SINC_BEST_QUALITY, 1, &src_err);
    if (src_err) {
        eprintln("libsamplerate error: %s", src_strerror(src_err));
        exit(1);
    }
    SRC_DATA src_data;
    src_data.data_in = src_data_in;
    src_data.data_out = src_data_out;
    src_data.input_frames = SRC_DATA_IN_SIZE;
    src_data.output_frames = SRC_DATA_OUT_SIZE;
    src_data.end_of_input = 0;
    src_data.src_ratio = (double)have.freq / CPU_CYCLE_RATE;
    SDL_PauseAudioDevice(audio_dev, 0);
    int src_data_in_fill = 0;
    SDL_Event e;
#ifdef MEASURE_TIME
    clock_t begin = clock();
#endif
    unsigned long long cycles = 0;
    while (true) {
        cpu_step(sys);
        ppu_step(sys, pixels, pitch);
        ppu_step(sys, pixels, pitch);
        ppu_step(sys, pixels, pitch);
        src_data_in[src_data_in_fill++] = apu_step(sys);
        if (src_data_in_fill >= SRC_DATA_IN_SIZE) {
            int src_err = src_process(src_state, &src_data);
            if (src_err) {
                eprintln("libsamplerate error: %s", src_strerror(src_err));
                exit(1);
            }
            SDL_QueueAudio(audio_dev, src_data_out, sizeof(float) * src_data.output_frames_gen);
            src_data_in_fill = 0;
        }
        if (sys->strobe)
            sys->controller_shift = controller_state;
        if (sys->scanline == 261 && sys->pixel >= 338) {
            while (SDL_PollEvent(&e)) {
                switch (e.type) {
                case SDL_QUIT: {
#ifdef MEASURE_TIME
                    clock_t end = clock();
                    double t = (double)(end - begin) / CLOCKS_PER_SEC;
                    println("%fs over %lld cycles - %fns avg.", t, cycles, t / cycles * 1000000000);
#endif
                    exit(0);
                }
                case SDL_KEYUP:
                    controller_state &= ~key_mask(e.key.keysym.scancode);
                    break;
                case SDL_KEYDOWN:
                    controller_state |= key_mask(e.key.keysym.scancode);
                    switch (e.key.keysym.scancode) {
                    case SDL_SCANCODE_Q:
                        *saved_sys = *sys;
                        saved_sys->mapper = saved_mapper;
                        mapper->copy(mapper, saved_mapper);
                        break;
                    case SDL_SCANCODE_E:
                        *sys = *saved_sys;
                        sys->mapper = mapper;
                        mapper->copy(saved_mapper, mapper);
                        break;
                    default:
                        break;
                    }
                    break;
                }
            }
            SDL_UnlockTexture(texture);
            SDL_RenderClear(renderer);
            SDL_RenderCopy(renderer, texture, NULL, NULL);
            SDL_RenderPresent(renderer);
            if (SDL_LockTexture(texture, NULL, &pixels, &pitch)) {
                eprintln("SDL error: %s", SDL_GetError());
                exit(1);
            }
        }
        cycles++;
    }
}
