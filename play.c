#include "play.h"
#include "file.h"
#include "input.h"
#include "coordinates.h"
#include "tiles.h"

#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <stdbool.h>

#ifdef DEBUGMALLOC
#include "debugmalloc.h"
#endif

typedef struct PlayState
{
    Level *firstLevel;
    Level *level;
    Level *backupLevel;
    Coordinates player;
    int result;
    bool ctrl;
    bool edited;
    bool finished;
    bool unsaved;
    char *filename;
    SDL_Renderer *renderer;
    SDL_Texture *tiles;
    TTF_Font *font;
} PlayState;

// Convert ot tileState to tile
// Returns brown floor on error
static Tile tileStateToTile(TileState state)
{
    switch (state)
    {
    case wallS:
        return wall;
    case playerS:
        return player;
    case playerOnTargetS:
        return player;
    case crateS:
        return crate;
    case crateOnTargetS:
        return crateOnTarget;
    case targetS:
        return target;
    case floorTileS:
        return brownFloor;
    default:
        return brownFloor;
    }
}

// Get tile at coordinates
// Returns invalidS for tiles outside of play area
static TileState getTileState(Level *level, int x, int y)
{
    if (x < 0 || y < 0 || x >= level->size.x || y >= level->size.y)
    {
        return invalidS;
    }
    return level->tiles[x + y * level->size.x];
}

// Set state of tile at coordinates
// Does nothing for tiles outside of play area
static void setTileState(Level *level, int x, int y, TileState state)
{
    if (x < 0 || y < 0 || x >= level->size.x || y >= level->size.y)
    {
        return;
    }
    level->tiles[x + y * level->size.x] = state;
}

// Render current state of play to renderer
// State must include proper renderer, tiles, and font
static void render(PlayState *state)
{
    // easier to work with variables this way
    SDL_Renderer *renderer = state->renderer;
    SDL_Texture *tiles = state->tiles;
    TTF_Font *font = state->font;
    Level *level = state->level;

    SDL_RenderClear(renderer);

    SDL_Color white = {255, 255, 255, 255};

    // start positions for level to center it on screen
    int startX = 1 + (19 - level->size.x) / 2;
    int startY = 0 + (11 - level->size.y) / 2;

    renderTiles(renderer, tiles, state->finished ? greenFloor : greyFloor, 0, 0, 19, 11);                             // background for entire window, based on level finishedness
    renderTiles(renderer, tiles, brownFloor, startX, startY, startX + level->size.x - 1, startY + level->size.y - 1); // background for level area

    for (int i = 0; i < level->size.x; i++) // render tiles except floors
    {
        for (int j = 0; j < level->size.y; j++)
        {
            Tile tile = tileStateToTile(level->tiles[i + j * level->size.x]);
            if (tile != brownFloor)
                renderTile(renderer, tiles, tile, startX + i, startY + j);
        }
    }

    // render player
    renderTile(renderer, tiles, player, state->player.x + startX, state->player.y + startY);

    // control buttons
    renderTile(renderer, tiles, home, 0, 0);
    renderTile(renderer, tiles, retry, 0, 1);
    renderTile(renderer, tiles, save, 0, 2);

    // player control buttons
    renderTile(renderer, tiles, up, 0, 6);
    renderTile(renderer, tiles, left, 0, 7);
    renderTile(renderer, tiles, right, 0, 8);
    renderTile(renderer, tiles, down, 0, 9);

    if (state->level->prev != NULL) // prevoius button is previous level exists
        renderTile(renderer, tiles, left, 0, 11);
    renderFont(renderer, font, white, level->name, 10, 11, true, true); // level name
    if (state->level->next != NULL)                                     // next button in next level exists
        renderTile(renderer, tiles, right, 19, 11);

    SDL_RenderPresent(renderer); // render creation
}

// Check if all targets are covered by crates
// Returns true if ^ true
static bool checkFinished(PlayState *state)
{
    for (int i = 0; i < state->level->size.x * state->level->size.y; i++)
    {
        TileState tile = state->level->tiles[i];
        if (tile == targetS)
        {
            return false;
        }
    }
    return true;
}

// Fill current game state with level data
// This is for resetting a level
static bool fillState(PlayState *state, Level *level)
{
    if (state->level != NULL) // free level if one is loaded
    {
        free(state->level->tiles);
        free(state->level);
    }

    state->level = (Level *)malloc(sizeof(Level)); // allocate memory for each component that could change
    if (state->level == NULL)
        return false;
    memcpy(state->level, level, sizeof(Level)); // copy data

    state->level->tiles = (TileState *)malloc(sizeof(TileState) * (level->size.x * level->size.y));
    if (state->level->tiles == NULL)
        return false;
    memcpy(state->level->tiles, level->tiles, sizeof(TileState) * (level->size.x * level->size.y));

    state->backupLevel = level; // store original level pointer

    for (int i = 0; i < level->size.x; i++) // extract player position and change tile under player
    {
        for (int j = 0; j < level->size.y; j++)
        {
            TileState tile = getTileState(level, i, j);
            if (tile == playerS || tile == playerOnTargetS)
            {
                state->player.x = i;
                state->player.y = j;
                setTileState(state->level, i, j, tile == playerS ? floorTileS : targetS);
            }
        }
    }

    state->edited = false;
    state->finished = checkFinished(state); // chack is level has alerady been finished

    return true;
}

// Copy level data from state to level pointer
// Does not modify data in current state, only overwrites level pointer data
static void writeState(PlayState *state)
{
    if (state->backupLevel == NULL) // cant save to no level
        return;

    memcpy(state->backupLevel->tiles, state->level->tiles, sizeof(TileState) * (state->level->size.x * state->level->size.y)); // copy data
    TileState *currentPlayerPos = state->backupLevel->tiles + state->player.x + state->player.y * state->backupLevel->size.x;  // transfer player position too
    if (*currentPlayerPos == targetS)
        *currentPlayerPos = playerOnTargetS;
    else
        *currentPlayerPos = playerS;
    state->edited = false;
}

// Save current levels to file
// Warns user in case of saving error
// Sets sate->result according to user popup state
static void saveState(PlayState *state)
{
    writeState(state);
    state->edited = false;
    int result = textInput(state->renderer, state->tiles, state->font, "File neve", state->filename, 63);
    if (result == 0)
    {
        state->result = 0;
        return;
    }
    if (result == 2)
        return;

    bool saveSuccess = saveLevel(state->firstLevel, state->filename);
    if (saveSuccess)
    {
        state->unsaved = false;
        if (alertBox(state->renderer, state->tiles, state->font, "Sikeres mentés!") == 0)
            state->result = 0;
    }
    else if (alertBox(state->renderer, state->tiles, state->font, "Sikertelen mentés") == 0)
        state->result = 0;
}

// Free memory used by level state storage
// Does not set level pointer to null (even though it should)
static void freeState(PlayState *state)
{
    if (state->level != NULL)
    {
        free(state->level->tiles);
        free(state->level);
    }
}

// Prompt player to save data or discard
// Returns false on SDL_Quit event
static bool promptEdit(PlayState *state)
{
    int result = dialogBox(state->renderer, state->tiles, state->font, "Szeretnéd eltárolni a módosításaidat?");
    if (result == 0)
    {
        state->result = 0;
        return false;
    }
    if (result == 1)
        writeState(state);
    return true;
}

// Loads next level to state
// Prompts player if works should be stored
static void nextLevel(PlayState *state)
{
    if (state->edited && !state->finished)
    {
        if (!promptEdit(state))
            return;
    }
    if (state->level->next != NULL)
    {
        Level *nextLevel = state->level->next;
        fillState(state, nextLevel);
    }
}

// Loads prevoius level to state
// Prompts player if works should be stored
static void prevLevel(PlayState *state)
{
    if (state->edited && !state->finished)
    {
        if (!promptEdit(state))
            return;
    }
    if (state->level->prev != NULL)
    {
        Level *prevLevel = state->level->prev;
        fillState(state, prevLevel);
    }
}

// Check validity of levels
// Checks number of players and crate count and target count relation
// Returns true if all levels are valid
static bool checkLevels(Level *level)
{

    while (level != NULL)
    {
        int playerCount = 0;
        int crateCount = 0;
        int targetCount = 0;
        for (int i = 0; i < level->size.x * level->size.y; i++)
        {
            if (level->tiles[i] == playerS || level->tiles[i] == playerOnTargetS)
            {
                playerCount++;
            }
            if (level->tiles[i] == crateS || level->tiles[i] == crateOnTargetS)
            {
                crateCount++;
            }
            if (level->tiles[i] == targetS || level->tiles[i] == crateOnTargetS || level->tiles[i] == playerOnTargetS)
            {
                targetCount++;
            }
        }
        if (playerCount != 1 || crateCount < targetCount)
        {
            return false;
        }
        level = level->next;
    }
    return true;
}

// Check if current level is in a win state
// Win state is when all targets are covered by crates
// Prompts player on this event
// Returns nothing
// Use checkFinished for most cases
static void checkWinState(PlayState *state)
{
    if (checkFinished(state))
    {
        state->finished = true;
        writeState(state);
        if (alertBox(state->renderer, state->tiles, state->font, "Sikeresen tejesítetted a pályát!") == 0)
            state->result = 0;
    }
}

// Move player left
// Returns true if rerender is needed
static bool moveLeft(PlayState *state)
{
    Coordinates *player = &state->player;
    if (getTileState(state->level, player->x - 1, player->y) == crateS ||
        getTileState(state->level, player->x - 1, player->y) == crateOnTargetS) // crate in front of player
    {
        if (getTileState(state->level, player->x - 2, player->y) == floorTileS ||
            getTileState(state->level, player->x - 2, player->y) == targetS) // crate can move
        {
            if (getTileState(state->level, player->x - 1, player->y) == crateS)   // crate not on target
                setTileState(state->level, player->x - 1, player->y, floorTileS); // replace crate with floor
            else
                setTileState(state->level, player->x - 1, player->y, targetS);      // replace player with target
            if (getTileState(state->level, player->x - 2, player->y) == floorTileS) // crate going to be on floor
                setTileState(state->level, player->x - 2, player->y, crateS);       // replace with crate
            else
                setTileState(state->level, player->x - 2, player->y, crateOnTargetS); // replace with crate on target
            player->x--;                                                              // move player
            checkWinState(state);
            return true;
        }
        return false;
    }
    if (getTileState(state->level, player->x - 1, player->y) == floorTileS ||
        getTileState(state->level, player->x - 1, player->y) == targetS) // player can move
    {
        player->x--; // move player
        return true;
    }
    return false;
}

// Move player up
// Returns true if rerender is needed
static bool moveUp(PlayState *state)
{
    Coordinates *player = &state->player;
    if (getTileState(state->level, player->x, player->y - 1) == crateS ||
        getTileState(state->level, player->x, player->y - 1) == crateOnTargetS) // crate in front of player
    {
        if (getTileState(state->level, player->x, player->y - 2) == floorTileS ||
            getTileState(state->level, player->x, player->y - 2) == targetS) // crate can move
        {
            if (getTileState(state->level, player->x, player->y - 1) == crateS)   // crate not on target
                setTileState(state->level, player->x, player->y - 1, floorTileS); // replace crate with floor
            else
                setTileState(state->level, player->x, player->y - 1, targetS);      // replace player with target
            if (getTileState(state->level, player->x, player->y - 2) == floorTileS) // crate going to be on floor
                setTileState(state->level, player->x, player->y - 2, crateS);       // replace with crate
            else
                setTileState(state->level, player->x, player->y - 2, crateOnTargetS); // replace with crate on target
            player->y--;                                                              // move player
            checkWinState(state);
            return true;
        }
        return false;
    }
    if (getTileState(state->level, player->x, player->y - 1) == floorTileS ||
        getTileState(state->level, player->x, player->y - 1) == targetS) // player can move
    {
        player->y--; // move player
        return true;
    }
    return false;
}

// Move player right
// Returns true if rerender is needed
static bool moveRight(PlayState *state)
{
    Coordinates *player = &state->player;
    if (getTileState(state->level, player->x + 1, player->y) == crateS ||
        getTileState(state->level, player->x + 1, player->y) == crateOnTargetS) // crate in front of player
    {
        if (getTileState(state->level, player->x + 2, player->y) == floorTileS ||
            getTileState(state->level, player->x + 2, player->y) == targetS) // crate can move
        {
            if (getTileState(state->level, player->x + 1, player->y) == crateS)   // crate not on target
                setTileState(state->level, player->x + 1, player->y, floorTileS); // replace crate with floor
            else
                setTileState(state->level, player->x + 1, player->y, targetS);      // replace player with target
            if (getTileState(state->level, player->x + 2, player->y) == floorTileS) // crate going to be on floor
                setTileState(state->level, player->x + 2, player->y, crateS);       // replace with crate
            else
                setTileState(state->level, player->x + 2, player->y, crateOnTargetS); // replace with crate on target
            player->x++;                                                              // move player
            checkWinState(state);
            return true;
        }
        return false;
    }
    if (getTileState(state->level, player->x + 1, player->y) == floorTileS ||
        getTileState(state->level, player->x + 1, player->y) == targetS) // player can move
    {
        player->x++; // move player
        return true;
    }
    return false;
}

// Move player down
// Returns true if rerender is needed
static bool moveDown(PlayState *state)
{
    Coordinates *player = &state->player;
    if (getTileState(state->level, player->x, player->y + 1) == crateS ||
        getTileState(state->level, player->x, player->y + 1) == crateOnTargetS) // crate in front of player
    {
        if (getTileState(state->level, player->x, player->y + 2) == floorTileS ||
            getTileState(state->level, player->x, player->y + 2) == targetS) // crate can move
        {
            if (getTileState(state->level, player->x, player->y + 1) == crateS)   // crate not on target
                setTileState(state->level, player->x, player->y + 1, floorTileS); // replace crate with floor
            else
                setTileState(state->level, player->x, player->y + 1, targetS);      // replace player with target
            if (getTileState(state->level, player->x, player->y + 2) == floorTileS) // crate going to be on floor
                setTileState(state->level, player->x, player->y + 2, crateS);       // replace with crate
            else
                setTileState(state->level, player->x, player->y + 2, crateOnTargetS); // replace with crate on target
            player->y++;                                                              // move player
            checkWinState(state);
            return true;
        }
        return false;
    }
    if (getTileState(state->level, player->x, player->y + 1) == floorTileS ||
        getTileState(state->level, player->x, player->y + 1) == targetS) // player can move
    {
        player->y++; // move player
        return true;
    }
    return false;
}

// Process player movement
// Direction: 0 - left, 1 - up, 2 - right, 3 - down
// Returns true if rerender is needed
static bool processMovement(int dir, PlayState *state)
{
    state->edited = true;
    state->unsaved = true;

    switch (dir)
    {
    case 0: // left
        return moveLeft(state);
    case 1: // up
        return moveUp(state);
    case 2: // right
        return moveRight(state);
    case 3: // down
        return moveDown(state);
    default:
        return false;
    }
}

// Handle exit to menu
// Prompts player is work is nusaved, does not save work for player
// Sets state->reuslt according to user input
static void handleExitToMenu(PlayState *state)
{
    if (state->unsaved)
    {
        int dialogResult = dialogBox(state->renderer, state->tiles, state->font, "Biztosan szeretnél mentés nélkül kilépni?");
        if (dialogResult == 0)
        {
            state->result = 0;
        }
        if (dialogResult == 1)
        {
            state->result = 1;
        }
    }
    else
    {
        state->result = 1;
    }
}

// Handle SDL key down event
// Returns true if erernder is needed
static bool handleKeydown(PlayState *state, SDL_Scancode key)
{
    switch (key)
    {
    case 0x50: // left arrow
    case 0x04: // letter A
        if (state->ctrl)
            return false;
        return processMovement(0, state);
    case 0x52: // up arrow
    case 0x1A: // letter W
        if (state->ctrl)
            return false;
        return processMovement(1, state);
    case 0x4F: // right arrow
    case 0x07: // letter D
        if (state->ctrl)
            return false;
        return processMovement(2, state);
    case 0x51: // down arrow
    case 0x16: // letter S
        if (state->ctrl)
        {
            saveState(state);
            state->ctrl = false;
            return true;
        }
        return processMovement(3, state);
    case 0x15: // letter r
        if (!state->ctrl)
            return false;
        fillState(state, state->backupLevel);
        return true;
    case 0x29: // esc
        if (state->ctrl)
            return false;
        handleExitToMenu(state);
        return true;
    case 0x4b: // page up
        if (state->ctrl)
            return false;
        nextLevel(state);
        return true;
    case 0x4e: // page down
        if (state->ctrl)
            return false;
        prevLevel(state);
        return true;
    case 0xe0: // left ctrl
    case 0xe4: // right ctrl
        state->ctrl = true;
        return false;
    default:
        // printf("0x%02x\n", event.key.keysym.scancode);
        return false;
    }
}

// Handle SDL key down up
// Returns true if erernder is needed
static bool handleKeyup(PlayState *state, SDL_Scancode key)
{
    switch (key)
    {
    case 0xe0: // left ctrl
    case 0xe4: // right ctrl
        state->ctrl = false;
        return false;
    default:
        return false;
    }
}

// Handle SDL mouse click
// Returns true if erernder is needed
static bool handleClick(PlayState *state, int x, int y)
{
    if (clickTile(0, 6, x, y)) // up
        return processMovement(1, state);
    if (clickTile(0, 9, x, y)) // down
        return processMovement(3, state);
    if (clickTile(0, 7, x, y)) // left
        return processMovement(0, state);
    if (clickTile(0, 8, x, y)) // right
        return processMovement(2, state);
    if (clickTile(0, 0, x, y))
    { // back to main menu
        handleExitToMenu(state);
        return true;
    }
    if (clickTile(0, 1, x, y)) // restart level
    {
        fillState(state, state->backupLevel);
        return true;
    }
    if (clickTile(0, 2, x, y)) // save level
    {
        saveState(state);
        return true;
    }
    if (clickTile(0, 11, x, y)) // previouse level
    {
        prevLevel(state);
        return true;
    }
    if (clickTile(19, 11, x, y)) // next level
    {
        nextLevel(state);
        return true;
    }
    return false;
}

// Handles SDL event
// Returns true if rerender is needed
static bool handleEvent(SDL_Event event, PlayState *state)
{
    switch (event.type)
    {
    case SDL_KEYDOWN:
        return handleKeydown(state, event.key.keysym.scancode);
    case SDL_KEYUP:
        return handleKeyup(state, event.key.keysym.scancode);
    case SDL_MOUSEBUTTONDOWN:
        return handleClick(state, event.button.x, event.button.y);
    case SDL_QUIT: // exit program
        state->result = 0;
        return false;
    default:
        return false;
        break;
    }
}

// Load and play levels given in file
// Returns 0 on SDL_Quit, 1 on exit to menu
int playLevel(SDL_Renderer *renderer, SDL_Texture *tiles, TTF_Font *font, char *filename)
{
    LoadLevelResult result = loadLevel(filename);
    switch (result.result)
    {
    case 1:
        unloadLevel(result.level);
        return alertBox(renderer, tiles, font, "Nem lehet megnyitni a fájlt");
        break;
    case 2:
        unloadLevel(result.level);
        return alertBox(renderer, tiles, font, "Memóriafoglalási hiba");
        break;
    case 3:
        unloadLevel(result.level);
        return alertBox(renderer, tiles, font, "A fájl hibás karaktereket tartalmaz");
        break;
    case 4:
        switch (dialogBox(renderer, tiles, font, "Néhány szint túl nagy. Biztosan megnyitod?"))
        {
        case 0:
            unloadLevel(result.level);
            return 0;
            break;
        case 1:
            break;
        case 2:
            unloadLevel(result.level);
            return 1;
            break;
        default:
            break;
        }
    default:
        break;
    }

    if (result.level == NULL)
    {
        return alertBox(renderer, tiles, font, "A fájl nem tartalmaz szinteket");
    }
    if (!checkLevels(result.level))
    {
        unloadLevel(result.level);
        return alertBox(renderer, tiles, font, "Néhány szint hibás");
    }

    PlayState state;
    state.level = NULL;
    state.firstLevel = result.level;
    if (!fillState(&state, result.level))
    {
        unloadLevel(result.level);
        return alertBox(renderer, tiles, font, "Memóriafoglalási hiba");
    }
    state.result = -1;
    state.renderer = renderer;
    state.tiles = tiles;
    state.font = font;
    state.ctrl = false;
    state.unsaved = false;
    state.filename = filename;

    render(&state);

    SDL_Event ev;
    while (SDL_WaitEvent(&ev))
    {
        bool rerender = handleEvent(ev, &state);
        if (state.result != -1) // if result was set
        {
            unloadLevel(result.level);
            freeState(&state);
            return state.result; // return to main
        }
        if (rerender) // if rerender is needed
            render(&state);
    }

    return 0;
}