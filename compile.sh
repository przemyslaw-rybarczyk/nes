#!/bin/bash

gcc -O2 -Wall *.c -o nes `sdl2-config --cflags --libs` "${@:1}"
