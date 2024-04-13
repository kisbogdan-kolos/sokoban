#ifndef INPUT_H
#define INPUT_H

#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>

int textInput(SDL_Renderer *renderer, SDL_Texture *tiles, TTF_Font *font, char *promptText, char *enteredText, int maxLength);
int alertBox(SDL_Renderer *renderer, SDL_Texture *tiles, TTF_Font *font, char *promptText);
int dialogBox(SDL_Renderer *renderer, SDL_Texture *tiles, TTF_Font *font, char *promptText);

#endif