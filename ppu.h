#pragma once

#include "SDL_render.h"
#include "global.h"

void ppu_step(System *sys, uint32_t *pixels, int pitch);
