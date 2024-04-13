#include "file.h"

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>

#ifdef DEBUGMALLOC
#include "debugmalloc.h"
#endif

typedef struct LoadState
{
    char line[64];
    int lineLength;
    char name[64];
    TileState level[209]; // 11 x 19 squares
    int rowIndex;
    int maxLength;
} LoadState;

// Check if given character is a valig sokoban tile
// Returns true if valid
static bool checkTile(char tile)
{
    if (tile == '#' ||
        tile == '@' ||
        tile == '+' ||
        tile == '$' ||
        tile == '*' ||
        tile == '.' ||
        tile == ' ')
        return true;
    return false;
}

// Convert character to tilestate
// Returns proper tile on success, invalidS on faliure
static TileState charToTile(char tile)
{
    switch (tile)
    {
    case '#':
        return wallS;
    case '@':
        return playerS;
    case '+':
        return playerOnTargetS;
    case '$':
        return crateS;
    case '*':
        return crateOnTargetS;
    case '.':
        return targetS;
    case ' ':
        return floorTileS;
    default:
        return invalidS;
    }
}

// Convert tile to character
// Returns sokoban char on success, space on faliure
static char tileToChar(TileState tile)
{
    switch (tile)
    {
    case wallS:
        return '#';
    case playerS:
        return '@';
    case playerOnTargetS:
        return '+';
    case crateS:
        return '$';
    case crateOnTargetS:
        return '*';
    case targetS:
        return '.';
    case floorTileS:
        return ' ';
    default:
        return ' ';
    }
}

// Fill entire level with floor tiles, except first one
// First tile is invalidS
static void emptyLevel(TileState *level)
{
    level[0] = '\0';
    for (int i = 1; i < 209; i++)
    {
        level[i] = floorTileS;
    }
}

// Add level to current linked list
// If level is too large, nothing will be saved and will result in success
// Returns 2 on faliure, 0 on success
static int addLevel(LoadState *state, Level **first, Level **current)
{
    if (state->maxLength <= 19 && state->rowIndex <= 11) // if level is not too large
    {
        if (*first == NULL) // if linked list is empty
        {
            *first = *current = (Level *)malloc(sizeof(Level)); // allocate memory for first element
            if (current == NULL)                                // if failed, return and free memory
            {
                unloadLevel(*first);
                return 2;
            }
            (*current)->prev = NULL; // set next and prev to null
            (*current)->next = NULL;
        }
        else
        {
            Level *new = (Level *)malloc(sizeof(Level)); // allocate memory for new element
            if (new == NULL)                             // if failed, return and free memory
            {
                unloadLevel(*first);
                return 2;
            }
            (*current)->next = new; // appent to current list
            new->prev = *current;   // also do it backwards
            new->next = NULL;       // this is the last element
            *current = new;         // make the new element the current element
        }

        (*current)->size.x = state->maxLength; // set sizes
        (*current)->size.y = state->rowIndex;
        (*current)->tiles = (TileState *)malloc(sizeof(TileState) * (state->maxLength * state->rowIndex)); // allocate memory for tiles
        if ((*current)->tiles == NULL)                                                                     // if failed, return and free memory
        {
            unloadLevel(*first);
            return 2;
        }
        (*current)->name = (char *)malloc(sizeof(char) * (strlen(state->name) + 1)); // allocate memory for name
        if ((*current)->name == NULL)                                                // if failed, return and free memory
        {
            unloadLevel(*first);
            return 2;
        }

        strcpy((*current)->name, state->name); // copy name

        for (int i = 0; i < state->rowIndex; i++) // copy tiles to new place
        {
            for (int j = 0; j < state->maxLength; j++)
            {
                // state level storage is 19 x 11, but the new level storage is only as big as big as it needs to be
                // so cannot use memcpy
                (*current)->tiles[j + state->maxLength * i] = state->level[j + 19 * i];
            }
        }
    }

    emptyLevel(state->level); // empty current level
    state->name[0] = 0;       // set variables to inital values
    state->rowIndex = 0;
    state->maxLength = 0;
    return 0; // success
}

// Load level based on filename
// Returns result containing linked list of levels and statuc code
// status code: 0 - success, 1 - failed to open file, 2 - failed to allocate memory
// 3 - invalid file format, 4 - valid, but large levels were removed
// linked list contains dinamically allocated memory, so must be freed after use
LoadLevelResult loadLevel(char *filename)
{
    LoadLevelResult result; // initialize variables
    result.result = 0;
    FILE *file;
    Level *first = NULL;
    Level *current;
    LoadState state;

    state.rowIndex = 0;
    state.maxLength = 0;
    emptyLevel(state.level); // clear level

    file = fopen(filename, "r");
    if (file == NULL) // if failed, return
    {
        result.result = 1;
        result.level = NULL;
        return result;
    }

    while (fgets(state.line, 64, file) != NULL) // while lines can be read from file
    {
        state.lineLength = strlen(state.line);
        while (state.line[state.lineLength - 1] == '\n' || state.line[state.lineLength - 1] == '\r') // remove trailing newline and carriage returns
        {
            state.line[state.lineLength - 1] = '\0';
            state.lineLength--;
        }

        if (state.line[0] == ';' && state.lineLength >= 3) // if comment <==> level name
        {
            if (state.line[1] == ' ') // if name contains leading space
                strcpy(state.name, state.line + 2);
            else
                strcpy(state.name, state.line + 1);
        }

        if (checkTile(state.line[0])) // if line starts with valid character
        {
            if (state.lineLength > state.maxLength) // track maximum line length
                state.maxLength = state.lineLength;

            if (state.lineLength <= 19 && state.rowIndex <= 10) // if size is still within limits
            {
                for (int i = 0; i < state.lineLength; i++) // copy each tile while checking if valid
                {
                    char c = state.line[i];
                    if (checkTile(c))
                        state.level[i + 19 * state.rowIndex] = charToTile(c);
                    else
                    {
                        unloadLevel(first); // free memory and return on faliure
                        result.result = 3;
                        result.level = NULL;
                        return result;
                    }
                }

                state.rowIndex++; // move to next row
            }
            else
                result.result = 4; // large levels were removed
        }

        if (state.lineLength <= 2 && state.level[0] != 0) // end of current level
        {
            if (addLevel(&state, &first, &current) == 2) // add level to linked list
            {
                result.result = 2; // return on memory allocation failure
                result.level = NULL;
                return result;
            }
        }
    }

    if (state.level[0] != 0) // if while loop finished and has not added last level, when file ends without empty line at the end
    {
        if (addLevel(&state, &first, &current) == 2)
        {
            result.result = 2;
            result.level = NULL;
            return result;
        }
    }

    fclose(file); // close file

    result.level = first; // set resulting linked list

    return result;
}

// Frees specified level, does not free entire linked list!
static void freeLevel(Level *level)
{
    if (level != NULL)
    {
        free(level->tiles);
        free(level->name);
        free(level);
    }
}

// Free loaded linked list of levels
void unloadLevel(Level *level)
{
    if (level == NULL) // nothing to free
        return;
    while (level->next != NULL) // while there are levels to free
    {
        freeLevel(level->prev); // free said level
        level = level->next;    // go to next one
    }
    freeLevel(level->prev); // free last levels
    freeLevel(level);
}

// Save linked list of levels to specified file
// Returns true on success, false on failure
bool saveLevel(Level *level, char *filename)
{
    FILE *file;

    file = fopen(filename, "w");
    if (file == NULL) // return if file was not opened successfully
        return false;

    while (level != NULL) // while there are levels to save
    {
        fprintf(file, "; %s\n", level->name);   // write name of level
        for (int i = 0; i < level->size.y; i++) // write rows of level
        {
            for (int j = 0; j < level->size.x; j++) // write columns of level
            {
                fputc(tileToChar(level->tiles[j + i * level->size.x]), file); // convert ot character and write
            }
            fputc('\n', file); // newline at end of every line
        }
        fputc('\n', file); // newline to separate levels

        level = level->next; // go to next level
    }

    return fclose(file) == 0; // return file close success
}