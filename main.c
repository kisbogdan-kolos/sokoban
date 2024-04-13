#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <math.h>
#include <stdlib.h>

#include "menu.h"
#include "file.h"
#include "play.h"
#include "edit.h"
#include "input.h"

#ifdef DEBUGMALLOC
#include "debugmalloc.h"
#endif

// Initialize SDL. Returns renderer pointer on success, returns NULL on faliure
SDL_Renderer *initSDL()
{
    if (SDL_Init(SDL_INIT_EVERYTHING) < 0)
    {
        return NULL;
    }
    // 20 x 12 blocks
    SDL_Window *window = SDL_CreateWindow("Sokoban", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 768, 0);
    if (window == NULL)
    {
        return NULL;
    }
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    return renderer;
}

// Main program function
// Return values: 0 - success, 1 - SDL init error, 2 - SDL_Image error, 3 - TTF_Font error
int main(void)
{
    SDL_Renderer *renderer = initSDL();
    if (renderer == NULL)
    {
        printf("ERROR: Couldn't initialize SDL\n");
        return 1;
    }

    SDL_Texture *tiles = IMG_LoadTexture(renderer, "tiles.png");
    if (tiles == NULL)
    {
        printf("ERROR: Couldn't load tiles texture\n");
        return 2;
    }

    TTF_Init();
    TTF_Font *font = TTF_OpenFont("font.ttf", 50);
    if (!font)
    {
        printf("ERROR: Couldn't open font\n");
        return 3;
    }

    int result;
    char filename[64];
    filename[0] = '\0';

    do
    {
        result = mainMenu(renderer, tiles, font, filename);
        switch (result)
        {
        case 1:
            result = playLevel(renderer, tiles, font, filename);
            break;
        case 2:
            result = editLevel(renderer, tiles, font, filename);
            break;
        default:
            break;
        }
    } while (result != 0);

    SDL_DestroyTexture(tiles);
    TTF_CloseFont(font);
    SDL_Quit();

    return 0;
}