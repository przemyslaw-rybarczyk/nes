#!/bin/bash

gcc -O2 -Wall *.c -o nes -lsoxr-lsr `sdl2-config --cflags --libs` "${@:1}"
