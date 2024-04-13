#ifndef MENU_H
#define MENU_H

#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>

int mainMenu(SDL_Renderer *renderer, SDL_Texture *tiles, TTF_Font *font, char *filename);

#endif