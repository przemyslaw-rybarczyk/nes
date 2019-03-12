#!/bin/bash

gcc -O2 -Wall *.c -o nes -lsamplerate `sdl2-config --cflags --libs` "${@:1}"
