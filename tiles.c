#include "tiles.h"
#include "coordinates.h"

#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <stdbool.h>

#ifdef DEBUGMALLOC
#include "debugmalloc.h"
#endif

// Translate tile type to coordinates of the tile
// Returns SDL_rect
static SDL_Rect toSourceRect(Tile tile)
{
    SDL_Rect src;
    src.w = 64;
    src.h = 64;

    switch (tile)
    {
    case greenFloor:
        src.x = 10;
        src.y = 6;
        break;
    case greyFloor:
        src.x = 11;
        src.y = 6;
        break;
    case brownFloor:
        src.x = 12;
        src.y = 6;
        break;
    case wall:
        src.x = 7;
        src.y = 6;
        break;
    case player:
        src.x = 0;
        src.y = 4;
        break;
    case crate:
        src.x = 3;
        src.y = 0;
        break;
    case crateOnTarget:
        src.x = 3;
        src.y = 1;
        break;
    case target:
        src.x = 12;
        src.y = 3;
        break;
    case up:
        src.x = 2;
        src.y = 0;
        break;
    case down:
        src.x = 2;
        src.y = 1;
        break;
    case left:
        src.x = 1;
        src.y = 0;
        break;
    case right:
        src.x = 1;
        src.y = 1;
        break;
    case home:
        src.x = 2;
        src.y = 2;
        break;
    case save:
        src.x = 4;
        src.y = 0;
        break;
    case add:
        src.x = 1;
        src.y = 2;
        break;
    case blank:
        src.x = 5;
        src.y = 2;
        break;
    case blankR:
        src.x = 4;
        src.y = 1;
        break;
    case blankL:
        src.x = 5;
        src.y = 1;
        break;
    case blankLR:
        src.x = 4;
        src.y = 2;
        break;
    case retry:
        src.x = 5;
        src.y = 0;
        break;
    case selection:
        src.x = 0;
        src.y = 3;
        break;
    case delete:
        src.x = 6;
        src.y = 2;
        break;
    default:
        src.x = 0;
        src.y = 0;
        break;
    }

    src.x *= 64;
    src.y *= 64;

    return src;
}

// Render tile to renderer. Coordinates map to whole blocks
// Texture must include proper tiles file
void renderTile(SDL_Renderer *renderer, SDL_Texture *tiles, Tile tile, int x, int y)
{
    SDL_Rect src = toSourceRect(tile);
    SDL_Rect dst = {64 * x, 64 * y, 64, 64};

    SDL_RenderCopy(renderer, tiles, &src, &dst);
}

// Render tile to renderer. Coordinates map to whole blocks
// Texture must include proper tiles file
void renderTileC(SDL_Renderer *renderer, SDL_Texture *tiles, Tile tile, Coordinates pos)
{
    renderTile(renderer, tiles, tile, pos.x, pos.y);
}

// Render tiles to renderer. Both coordinates are inclusive
// Works same as renderTile
void renderTiles(SDL_Renderer *renderer, SDL_Texture *tiles, Tile tile, int x1, int y1, int x2, int y2)
{
    for (int x = x1; x <= x2; x++)
    {
        for (int y = y1; y <= y2; y++)
        {
            renderTile(renderer, tiles, tile, x, y);
        }
    }
}

// Render tiles to renderer. Both coordinates are inclusive
// Works same as renderTile
void renderTilesC(SDL_Renderer *renderer, SDL_Texture *tiles, Tile tile, Coordinates pos1, Coordinates pos2)
{
    renderTiles(renderer, tiles, tile, pos1.x, pos1.y, pos2.x, pos2.y);
}

// Render text to renderer. Coordinates map to whole blocks
// CenteredX: center font in X direction (false: left)
// CenteredY: center font in Y direction (false: top)
void renderFont(SDL_Renderer *renderer, TTF_Font *font, SDL_Color color, char *text, int x, int y, bool centeredX, bool centeredY)
{
    if (text == NULL)
        return;
    if (*text == '\0')
        return;
    x *= 64;
    y *= 64;
    x += 32;
    y += 32;

    SDL_Surface *textSurface;
    SDL_Texture *textTexture;
    SDL_Rect destination;

    textSurface = TTF_RenderUTF8_Solid(font, text, color);
    textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
    destination.x = centeredX ? x - textSurface->w / 2 : x;
    destination.y = centeredY ? y - textSurface->h / 2 : y;
    destination.w = textSurface->w;
    destination.h = textSurface->h;
    SDL_RenderCopy(renderer, textTexture, NULL, &destination);
    SDL_FreeSurface(textSurface);
    SDL_DestroyTexture(textTexture);
}

// Render text to renderer. Coordinates map to whole blocks
// CenteredX: center font in X direction (false: left)
// CenteredY: center font in Y direction (false: top)
void renderFontC(SDL_Renderer *renderer, TTF_Font *font, SDL_Color color, char *text, Coordinates pos, bool centeredX, bool centeredY)
{
    renderFont(renderer, font, color, text, pos.x, pos.y, centeredX, centeredY);
}

// Checks if click coordinates match tile coordinates
// Tile coordinates map to whole blocks, click coordinates are in pixels
bool clickTile(int tileX, int tileY, int clickX, int clickY)
{
    tileX *= 64;
    tileY *= 64;
    if (
        tileX < clickX && tileX + 64 > clickX && // x direction
        tileY < clickY && tileY + 64 > clickY)   // y direction
    {
        return true;
    }
    return false;
}

// Check if tiles in an area were clicked
// tileX1 <= tileX2, tileY1 <= tileY2
bool clickTiles(int tileX1, int tileY1, int tileX2, int tileY2, int clickX, int clickY)
{
    tileX1 *= 64;
    tileY1 *= 64;
    tileX2 *= 64;
    tileY2 *= 64;
    if (
        tileX1 < clickX && tileX2 + 64 > clickX && // x direction
        tileY1 < clickY && tileY2 + 64 > clickY)   // y direction
    {
        return true;
    }
    return false;
}