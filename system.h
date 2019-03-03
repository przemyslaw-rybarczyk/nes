#pragma once

#include "global.h"

System *new_System(Mapper *mapper);
u8 cpu_read(System *sys, u16 addr);
void cpu_write(System *sys, u16 addr, u8 data);
u8 ppu_read(System *sys, u16 addr);
void ppu_write(System *sys, u16 addr, u8 data);
