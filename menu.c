#include "menu.h"
#include "tiles.h"
#include "coordinates.h"
#include "input.h"

#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <stdbool.h>

#ifdef DEBUGMALLOC
#include "debugmalloc.h"
#endif

// Current state of menu
// x, y: coordinates of player, filename: current filename, can be for opening and saving
typedef struct MenuState
{
    Coordinates crate;
    Coordinates player;
    char *filename;
    int result;
    SDL_Renderer *renderer;
    SDL_Texture *tiles;
    TTF_Font *font;
} MenuState;

// Render menu to renderer based on menu state
// Menu state must include proper renderer, tiles and font
static void renderMenu(MenuState *state)
{
    // easier to work with variables this way
    SDL_Renderer *renderer = state->renderer;
    SDL_Texture *tiles = state->tiles;
    TTF_Font *font = state->font;

    SDL_RenderClear(renderer);

    // white color for texts
    SDL_Color color = {255, 255, 255, 255};

    // background floor
    renderTiles(renderer, tiles, greyFloor, 0, 0, 19, 11);

    // option selector walls and floor
    renderTiles(renderer, tiles, brownFloor, 1, 1, 6, 10);
    renderTiles(renderer, tiles, wall, 1, 1, 1, 10);
    renderTiles(renderer, tiles, wall, 2, 1, 5, 1);
    renderTiles(renderer, tiles, wall, 6, 1, 6, 10);
    renderTiles(renderer, tiles, wall, 2, 10, 5, 10);

    // option selector crate and targets
    renderTileC(renderer, tiles, crate, state->crate);

    renderTile(renderer, tiles, target, 4, 4);
    renderTile(renderer, tiles, target, 4, 7);
    renderTile(renderer, tiles, target, 2, 2);
    renderTile(renderer, tiles, target, 5, 2);
    renderTile(renderer, tiles, target, 2, 9);
    renderTile(renderer, tiles, target, 5, 9);

    // option selector player
    renderTileC(renderer, tiles, player, state->player);

    // render buttons to control player
    renderTiles(renderer, tiles, greenFloor, 14, 5, 18, 9);
    renderTiles(renderer, tiles, wall, 14, 5, 18, 5);
    renderTiles(renderer, tiles, wall, 14, 9, 18, 9);
    renderTiles(renderer, tiles, wall, 14, 6, 14, 8);
    renderTiles(renderer, tiles, wall, 18, 6, 18, 8);
    renderTile(renderer, tiles, up, 16, 6);
    renderTile(renderer, tiles, down, 16, 8);
    renderTile(renderer, tiles, left, 15, 7);
    renderTile(renderer, tiles, right, 17, 7);

    // render texts
    renderFont(renderer, font, color, "Sokoban", 10, 1, false, true);

    renderFont(renderer, font, color, "Játék kezdése", 7, 4, false, true);
    renderFont(renderer, font, color, "Szintszerkesztő", 7, 7, false, true);
    renderFont(renderer, font, color, "Kilépésch", 7, 9, false, true);
    renderFont(renderer, font, color, "Kilépésch", 7, 2, false, true);

    renderFont(renderer, font, color, "Told a ládát a megfelelő opció előtti célra!", 0, 11, false, true);

    SDL_RenderPresent(renderer);
}

// Process current position of crate and set menu result accordingly
// Returns true if crate movement is allowed
static bool processCratePosition(MenuState *state, int crateX, int crateY)
{
    if ((crateX == 2 || crateX == 5) && (crateY == 2 || crateY == 9)) // exit
    {
        state->result = 0;
        return false;
    }
    if (crateX == 4 && crateY == 4) // start game
    {
        // call text input for filename
        int result = textInput(state->renderer, state->tiles, state->font, "File neve", state->filename, 63);
        switch (result)
        {
        case 0: // exit program
            state->result = 0;
            return false;
            break;
        case 1: // file input successfull, return 1
            state->result = 1;
            return false;
            break;
        case 2: // file input cancelled
            return false;
            break;
        default:
            return false;
            break;
        }
    }
    if (crateX == 4 && crateY == 7) // level edtor
    {
        // call text input for filename
        int result = textInput(state->renderer, state->tiles, state->font, "File neve", state->filename, 63);
        switch (result)
        {
        case 0: // exit program
            state->result = 0;
            return false;
            break;
        case 1: // file input successfull, return 1
            state->result = 2;
            return false;
            break;
        case 2: // file input cancelled
            return false;
            break;
        default:
            return false;
            break;
        }
    }
    return true;
}

// Process player movement
// Direction: 0 - left, 1 - up, 2 - right, 3 - down
// Returns true if rerender is needed
static bool processMovement(int dir, MenuState *state)
{
    // get coordinates for simpler code
    Coordinates *player = &state->player;
    Coordinates *crate = &state->crate;

    switch (dir)
    {
    case 0:                                                     // left
        if (player->x == crate->x + 1 && player->y == crate->y) // crate in front of player
        {
            if (crate->x - 1 > 1) // crate can move
            {
                if (processCratePosition(state, crate->x - 1, crate->y))
                {
                    crate->x--;
                    player->x--;
                    return true;
                }
                return true;
            }
            return false;
        }
        if (player->x - 1 > 1) // player can move
        {
            player->x--;
            return true;
        }
        return false;
    case 1:                                                     // up
        if (player->y == crate->y + 1 && player->x == crate->x) // crate in front of player
        {
            if (crate->y - 1 > 1) // crate can move
            {
                if (processCratePosition(state, crate->x, crate->y - 1))
                {
                    crate->y--;
                    player->y--;
                    return true;
                }
                return true;
            }
            return false;
        }
        if (player->y - 1 > 1) // player can move
        {
            player->y--;
            return true;
        }
        return false;
    case 2:                                                     // right
        if (player->x == crate->x - 1 && player->y == crate->y) // crate in front of player
        {
            if (crate->x + 1 < 6) // crate can move
            {
                if (processCratePosition(state, crate->x + 1, crate->y))
                {
                    crate->x++;
                    player->x++;
                    return true;
                }
                return true;
            }
            return false;
        }
        if (player->x + 1 < 6) // player can move
        {
            player->x++;
            return true;
        }
        return false;
    case 3:                                                     // down
        if (player->y == crate->y - 1 && player->x == crate->x) // crate in front of player
        {
            if (crate->y + 1 < 10) // crate can move
            {
                if (processCratePosition(state, crate->x, crate->y + 1))
                {
                    crate->y++;
                    player->y++;
                    return true;
                }
                return true;
            }
            return false;
        }
        if (player->y + 1 < 10) // player can move
        {
            player->y++;
            return true;
        }
        return false;
    default:
        return false;
    }
}

// Handles SDL event
// Returns true if rerender is needed
static bool handleEvent(SDL_Event event, MenuState *state)
{
    switch (event.type)
    {
    case SDL_KEYDOWN:
        switch (event.key.keysym.scancode)
        {
        case 0x50: // left arrow
        case 0x04: // letter A
            return processMovement(0, state);
        case 0x52: // up arrow
        case 0x1A: // letter W
            return processMovement(1, state);
        case 0x4F: // right arrow
        case 0x07: // letter D
            return processMovement(2, state);
        case 0x51: // down arrow
        case 0x16: // letter S
            return processMovement(3, state);
        default:
            return false;
        }
        break;
    case SDL_MOUSEBUTTONDOWN:
        if (clickTile(16, 6, event.button.x, event.button.y)) // up
            return processMovement(1, state);
        if (clickTile(16, 8, event.button.x, event.button.y)) // down
            return processMovement(3, state);
        if (clickTile(15, 7, event.button.x, event.button.y)) // left
            return processMovement(0, state);
        if (clickTile(17, 7, event.button.x, event.button.y)) // right
            return processMovement(2, state);
        return false;
        break;
    case SDL_QUIT: // exit program
        state->result = 0;
        return false;
    default:
        return false;
        break;
    }
}

// Do main menu, returns MenuResult struct on menu exit
// Must include proper tiles and font
// Result: 0 - exit program, 1 - play level, 2 - level editor
int mainMenu(SDL_Renderer *renderer, SDL_Texture *tiles, TTF_Font *font, char *filename)
{
    MenuState state;

    // set renderer, tiles and font for state
    // easier to pass to functions this way
    state.renderer = renderer;
    state.tiles = tiles;
    state.font = font;
    state.filename = filename;

    // initialize state variables
    state.result = -1;
    state.player.x = 3;
    state.player.y = 4;
    state.crate.x = 4;
    state.crate.y = 6;

    // render initialized state
    renderMenu(&state);

    SDL_Event ev;
    while (SDL_WaitEvent(&ev))
    {
        bool rerender = handleEvent(ev, &state);
        if (state.result != -1) // if result was set
        {
            return state.result; // return to main
        }
        if (rerender) // if rerender is needed
            renderMenu(&state);
    }

    return 0;
}