#ifndef TILES_H
#define TILES_H

#include "coordinates.h"

#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <stdbool.h>

typedef enum Tile
{
    greenFloor,
    greyFloor,
    brownFloor,
    wall,
    player,
    crate,
    crateOnTarget,
    target,
    up,
    down,
    left,
    right,
    home,
    save,
    add,
    blank,
    blankL,
    blankR,
    blankLR,
    retry,
    selection,
    delete
} Tile;

void renderTile(SDL_Renderer *renderer, SDL_Texture *tiles, Tile tile, int x, int y);
void renderTileC(SDL_Renderer *renderer, SDL_Texture *tiles, Tile tile, Coordinates pos);
void renderTiles(SDL_Renderer *renderer, SDL_Texture *tiles, Tile tile, int x1, int y1, int x2, int y2);
void renderTilesC(SDL_Renderer *renderer, SDL_Texture *tiles, Tile tile, Coordinates pos1, Coordinates pos2);

void renderFont(SDL_Renderer *renderer, TTF_Font *font, SDL_Color color, char *text, int x, int y, bool centeredX, bool centeredY);
void renderFontC(SDL_Renderer *renderer, TTF_Font *font, SDL_Color color, char *text, Coordinates pos, bool centeredX, bool centeredY);

bool clickTile(int tileX, int tileY, int clickX, int clickY);
bool clickTiles(int tileX1, int tileY1, int tileX2, int tileY2, int clickX, int clickY);

#endif