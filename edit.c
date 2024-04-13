#include "edit.h"
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

typedef struct EditState
{
    Level *firstLevel;
    Level *level;
    Coordinates edit;
    int selection;
    int result;
    bool ctrl;
    bool unsaved;
    char *filename;
    SDL_Renderer *renderer;
    SDL_Texture *tiles;
    TTF_Font *font;
} EditState;

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
/*
// Get tile at coordinates
// Returns invalidS for tiles outside of play area
static TileState getTileState(Level *level, int x, int y)
{
    if (x < 0 || y < 0 || x >= level->size.x || y >= level->size.y)
    {
        return invalidS;
    }
    return level->tiles[x + y * level->size.x];
}*/

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
static void render(EditState *state)
{
    // easier to work with variables this way
    SDL_Renderer *renderer = state->renderer;
    SDL_Texture *tiles = state->tiles;
    TTF_Font *font = state->font;
    Level *level = state->level;

    SDL_RenderClear(renderer);

    SDL_Color white = {255, 255, 255, 255};
    renderTiles(renderer, tiles, greyFloor, 0, 0, 19, 11); // background for entire window, based on level finishedness
    renderTile(renderer, tiles, home, 0, 0);               // control buttons
    renderTile(renderer, tiles, save, 0, 1);
    renderTile(renderer, tiles, add, 1, 11);
    renderTile(renderer, tiles, add, 18, 11);

    if (state->level != NULL)
    {
        for (int i = 0; i < 7; i++)
        {
            renderTile(renderer, tiles, tileStateToTile(i + 1), 0, i + 2);
            if (i + 1 == playerOnTargetS)
                renderTile(renderer, tiles, target, 0, i + 2);
            if (i == state->selection)
                renderTile(renderer, tiles, selection, 0, i + 2);
        }
        // start positions for level to center it on screen
        int startX = 1 + (19 - level->size.x) / 2;
        int startY = 0 + (11 - level->size.y) / 2;

        renderTiles(renderer, tiles, brownFloor, startX, startY, startX + level->size.x - 1, startY + level->size.y - 1); // background for level area

        for (int i = 0; i < level->size.x; i++) // render tiles except floors
        {
            for (int j = 0; j < level->size.y; j++)
            {
                TileState tileState = level->tiles[i + j * level->size.x];
                Tile tile = tileStateToTile(tileState);
                if (tile != brownFloor)
                    renderTile(renderer, tiles, tile, startX + i, startY + j);
                if (tileState == playerOnTargetS)
                    renderTile(renderer, tiles, target, startX + i, startY + j);
            }
        }

        // render selection
        renderTile(renderer, tiles, selection, state->edit.x + startX, state->edit.y + startY);

        if (state->level->prev != NULL) // prevoius button is previous level exists
            renderTile(renderer, tiles, left, 0, 11);
        renderFont(renderer, font, white, level->name, 10, 11, true, true); // level name
        if (state->level->next != NULL)                                     // next button in next level exists
            renderTile(renderer, tiles, right, 19, 11);
        renderTile(renderer, tiles, delete, 0, 10);
    }

    SDL_RenderPresent(renderer); // render creation
}

// Save current levels to file
// Warns user in case of saving error
// Sets sate->result according to user popup state
static void saveState(EditState *state)
{
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
        {
            state->result = 0;
        }
    }
    else
    {
        if (alertBox(state->renderer, state->tiles, state->font, "Sikertelen mentés") == 0)
        {
            state->result = 0;
        }
    }
}

// Create level object based on user input
// Return values: 0 - SDL_Quit, 1 - success/malloc error (based on *level), 2 - user canceled
static int createLevel(EditState *state, Level **level)
{

    Coordinates size = {0, 0};
    do
    {
        char sizeString[64]; // create empty string
        sizeString[0] = '\0';
        int result = textInput(state->renderer, state->tiles, state->font, "Méret (szélesség x magasság)", sizeString, 63); // prompt user size
        if (result == 0)
            return 0;
        if (result == 2)
            return 2;
        int count = sscanf(sizeString, "%dx%d", &size.x, &size.y);                // read size from string
        if (count != 2 || size.x < 1 || size.y < 1 || size.x > 19 || size.y > 11) // if size is invalid / sscanf failed
        {
            if (alertBox(state->renderer, state->tiles, state->font, "Hibás méret!") == 0)
            {
                return 0;
            }
        }
    } while (size.x < 1 || size.y < 1 || size.x > 19 || size.y > 11); // while size not correct
    *level = (Level *)malloc(sizeof(Level));                          // allocate memory for new level
    if (*level == NULL)                                               // return if failed
        return 1;
    (*level)->name = (char *)malloc(sizeof(char) * 17); // allocate memory for name
    if ((*level)->name == NULL)                         // return if failed
    {
        free(*level); // free allocated memory before return
        return 1;
    }
    strcpy((*level)->name, "Névtelen szint"); // fill name
    (*level)->size = size;
    (*level)->tiles = (TileState *)malloc(size.x * size.y * sizeof(TileState)); // allocate memory for tiles
    if ((*level)->tiles == NULL)                                                // return if failed
    {
        free((*level)->name); // free allocated memory before return
        free(*level);
        return 1;
    }
    (*level)->prev = NULL; // set linked list pointers
    (*level)->next = NULL;
    return 1;
}

// Add new level to current list of levels
// Dir: true - before current level, false - after current level
static void addLevel(EditState *state, bool dir)
{
    Level *newLevel = NULL;
    int result = createLevel(state, &newLevel); // create new level based on user input
    if (result != 1)                            // return if not success/malloc error
    {
        if (result == 0) // set state if needed
            state->result = 0;
        return;
    }
    if (newLevel == NULL) // if memory allocation error
    {
        if (alertBox(state->renderer, state->tiles, state->font, "Memóriafoglalási hiba") == 0)
        {
            state->result = 0;
        }
        return;
    }
    if (dir)
    {                             // after current level
        if (state->level == NULL) // linked list is empty
        {
            state->firstLevel = newLevel; // first level is this level
            newLevel->prev = NULL;        // no other levels
            newLevel->next = NULL;
        }
        else
        {                                  // linked list is not empty
            newLevel->prev = state->level; // set prev and next levels
            newLevel->next = state->level->next;
            if (state->level->next != NULL)          // if not adding after last element
                state->level->next->prev = newLevel; // set previous pointer of next element to this one
            state->level->next = newLevel;           // set next pointer of current element to this one
        }
    }
    else
    {                             // before current level
        if (state->level == NULL) // if linked list is empty
        {
            state->firstLevel = newLevel; // first level is this level
            newLevel->prev = NULL;        // no other levels
            newLevel->next = NULL;
        }
        else
        {
            newLevel->next = state->level; // set prev and next levels
            newLevel->prev = state->level->prev;
            if (state->level->prev != NULL)          // if not inserting before first element
                state->level->prev->next = newLevel; // set previous pointer of next element to this one
            else
                state->firstLevel = newLevel; // else set first element to this one (as this is the new first one)
            state->level->prev = newLevel;    // set prevoius pointer of current element to this one
        }
    }
    state->level = newLevel; // set current level to new one
    state->unsaved = true;   // modified file
    state->edit.x = 0;       // set selection
    state->edit.y = 0;
}

// Paprikás krumplit főz
// A state-ben benne kell lennie minden alapanyagnak
// Sikertelen főzés esetén a state->result -2 lesz
// Mosatlan edények száma mindig egyel nő
static void deleteCurrent(EditState *state)
{
    if (state->level == NULL)
        return;
    int result = dialogBox(state->renderer, state->tiles, state->font, "Biztosan törlöd a szintet?");
    if (result != 1)
    {
        if (result == 0)
            state->result = 0;
        return;
    }
    state->unsaved = true;
    state->edit.x = 0;
    state->edit.y = 0;
    if (state->level->prev == NULL && state->level->next == NULL)
    {
        free(state->level->tiles);
        free(state->level->name);
        free(state->level);
        state->level = NULL;
        state->firstLevel = NULL;
        return;
    }
    if (state->level->prev == NULL)
    {
        state->firstLevel = state->level->next;
        state->firstLevel->prev = NULL;
        free(state->level->tiles);
        free(state->level->name);
        free(state->level);
        state->level = state->firstLevel;
        return;
    }
    if (state->level->next == NULL)
    {
        state->level->prev->next = NULL;
        Level *prev = state->level->prev;
        free(state->level->tiles);
        free(state->level->name);
        free(state->level);
        state->level = prev;
        return;
    }
    state->level->prev->next = state->level->next;
    state->level->next->prev = state->level->prev;
    Level *next = state->level->next;
    free(state->level->tiles);
    free(state->level->name);
    free(state->level);
    state->level = next;
}

// Swtiches to next level if possible
static void nextLevel(EditState *state)
{
    if (state->level->next == NULL)
        return;
    state->level = state->level->next;
    state->edit.x = 0;
    state->edit.y = 0;
}

// Swtiches to prevoius level if possible
static void prevLevel(EditState *state)
{
    if (state->level->prev == NULL)
        return;
    state->level = state->level->prev;
    state->edit.x = 0;
    state->edit.y = 0;
}

// Exit to main menu
// Prompts user if work is unsaved
static void handleExitToMenu(EditState *state)
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

// Process selection movement
// Direction: 0 - left, 1 - up, 2 - right, 3 - down
// Returns true if rerender is needed
static bool moveSelection(int dir, EditState *state)
{
    switch (dir)
    {
    case 0: // left
        if (state->edit.x > 0)
        {
            state->edit.x--;
            return true;
        }
        return false;
    case 1: // up
        if (state->edit.y > 0)
        {
            state->edit.y--;
            return true;
        }
        return false;
    case 2: // right
        if (state->edit.x < state->level->size.x - 1)
        {
            state->edit.x++;
            return true;
        }
        return false;
    case 3: // down
        if (state->edit.y < state->level->size.y - 1)
        {
            state->edit.y++;
            return true;
        }
        return false;
    default:
        return false;
    }
}

// Set tile on edit coordinates to selected tile
static void modifyTile(EditState *state)
{
    if (state->edit.x >= 0 && state->edit.x < state->level->size.x && state->edit.y >= 0 && state->edit.y < state->level->size.y)
    {
        state->unsaved = true;
        setTileState(state->level, state->edit.x, state->edit.y, state->selection + 1);
    }
}

// Rename current level
static void renameLevel(EditState *state)
{
    if (state->level == NULL)
        return;
    char name[64];
    name[0] = 0;
    if (state->level->name != NULL)
        strcpy(name, state->level->name);
    int result = textInput(state->renderer, state->tiles, state->font, "Szint neve", name, 63);
    if (result != 1)
    {
        if (result == 0)
            state->result = 0;
        return;
    }
    free(state->level->name);
    int len = strlen(name);
    state->level->name = (char *)malloc((len + 1) * sizeof(char));
    strcpy(state->level->name, name);
    state->unsaved = true;
}

// Handle SDL key down event
// Returns true if erernder is needed
static bool handleKeydown(EditState *state, SDL_Scancode key)
{
    switch (key)
    {
    case 0x50: // left arrow
    case 0x04: // letter A
        if (state->ctrl)
            return false;
        return moveSelection(0, state);
    case 0x52: // up arrow
    case 0x1A: // letter W
        if (state->ctrl)
            return false;
        return moveSelection(1, state);
    case 0x4F: // right arrow
    case 0x07: // letter D
        if (state->ctrl)
            return false;
        return moveSelection(2, state);
    case 0x51: // down arrow
    case 0x16: // letter S
        if (state->ctrl)
        {
            saveState(state);
            state->ctrl = false;
            return true;
        }
        return moveSelection(3, state);
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
    case 0x4c: // delete
        if (state->ctrl)
            return false;
        deleteCurrent(state);
        return true;
    case 0x4a: // home
        if (state->ctrl)
            return false;
        addLevel(state, false);
        return true;
    case 0x4d: // end
        if (state->ctrl)
            return false;
        addLevel(state, true);
        return true;
    case 0x15: // letter r
        if (state->ctrl)
            return false;
        renameLevel(state);
        return true;
    case 0x28: // enter
    case 0x2c: // spacebar
        if (state->ctrl)
            return false;
        modifyTile(state);
        return true;
    case 0x1e: // 1 key
        if (state->ctrl)
            return false;
        state->selection = 0;
        return true;
    case 0x1f: // 2 key
        if (state->ctrl)
            return false;
        state->selection = 1;
        return true;
    case 0x20: // 3 key
        if (state->ctrl)
            return false;
        state->selection = 2;
        return true;
    case 0x21: // 4 key
        if (state->ctrl)
            return false;
        state->selection = 3;
        return true;
    case 0x22: // 5 key
        if (state->ctrl)
            return false;
        state->selection = 4;
        return true;
    case 0x23: // 6 key
        if (state->ctrl)
            return false;
        state->selection = 5;
        return true;
    case 0x24: // 7 key
        if (state->ctrl)
            return false;
        state->selection = 6;
        return true;
    default:
        // printf("0x%02x\n", event.key.keysym.scancode);
        return false;
    }
}

// Handle SDL key up event
// Returns true if erernder is needed
static bool handleKeyup(EditState *state, SDL_Scancode key)
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
// Returns true if rerender is needed
static bool handleClick(EditState *state, int x, int y)
{
    if (clickTile(0, 0, x, y)) // back to main menu
    {
        handleExitToMenu(state);
        return true;
    }
    if (clickTile(0, 1, x, y)) // save level
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
    if (clickTile(1, 11, x, y)) // add level left
    {
        addLevel(state, false);
        return true;
    }
    if (clickTile(18, 11, x, y)) // add level right
    {
        addLevel(state, true);
        return true;
    }
    if (clickTile(0, 10, x, y)) // delete level
    {
        deleteCurrent(state);
        return true;
    }
    if (state->level != NULL)
    {
        for (int i = 0; i < 7; i++)
        {
            if (clickTile(0, i + 2, x, y))
            {
                state->selection = i;
                return true;
            }
        }
        int startX = 1 + (19 - state->level->size.x) / 2;
        int startY = 0 + (11 - state->level->size.y) / 2;

        for (int i = 0; i < state->level->size.x; i++)
        {
            for (int j = 0; j < state->level->size.y; j++)
            {
                if (clickTile(startX + i, startY + j, x, y))
                {
                    state->edit.x = i;
                    state->edit.y = j;
                    modifyTile(state);
                    return true;
                }
            }
        }

        if (clickTiles(2, 11, 17, 11, x, y))
        {
            renameLevel(state);
            return true;
        }
    }
    return false;
}

// Handles SDL event
// Returns true if rerender is needed
static bool handleEvent(SDL_Event event, EditState *state)
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

// Load and edit levels given in file
// Returns 0 on SDL_Quit, 1 on exit to menu
int editLevel(SDL_Renderer *renderer, SDL_Texture *tiles, TTF_Font *font, char *filename)
{
    LoadLevelResult result = loadLevel(filename);
    switch (result.result)
    {
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

    EditState state;
    state.level = NULL;
    state.firstLevel = result.level;
    state.level = result.level;
    state.result = -1;
    state.renderer = renderer;
    state.tiles = tiles;
    state.font = font;
    state.ctrl = false;
    state.unsaved = false;
    state.filename = filename;
    state.edit.x = 0;
    state.edit.y = 0;
    state.selection = 0;

    render(&state);

    SDL_Event ev;
    while (SDL_WaitEvent(&ev))
    {
        bool rerender = handleEvent(ev, &state);
        if (state.result != -1) // if result was set
        {
            unloadLevel(state.firstLevel);
            return state.result; // return to main
        }
        if (rerender) // if rerender is needed
            render(&state);
    }

    return 0;
}
