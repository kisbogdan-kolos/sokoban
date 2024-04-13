#ifndef EDIT_H
#define EDIT_H

#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>

int editLevel(SDL_Renderer *renderer, SDL_Texture *tiles, TTF_Font *font, char *filename);

#endif