#!/bin/bash

# delete previous result
# rm main

# install sdl2
# sudo apt install libsdl2-dev libsdl2-gfx-dev libsdl2-image-dev libsdl2-ttf-dev libsdl2-mixer-dev

# with debugmalloc
# gcc -g *.c -o main `sdl2-config --cflags --libs` -DDEBUGMALLOC  -lSDL2_gfx -lSDL2_ttf -lSDL2_image -lSDL2_mixer 

# without debugmalloc
gcc -g *.c -o main `sdl2-config --cflags --libs` -lSDL2_gfx -lSDL2_ttf -lSDL2_image -lSDL2_mixer 