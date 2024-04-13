#ifndef FILE_H
#define FILE_H

#include <stdbool.h>
#include "coordinates.h"

typedef enum TileState
{
    invalidS = 0,
    wallS,
    playerS,
    playerOnTargetS,
    crateS,
    crateOnTargetS,
    targetS,
    floorTileS
} TileState;

typedef struct Level
{
    Coordinates size;
    TileState *tiles;
    char *name;
    struct Level *prev;
    struct Level *next;
} Level;

typedef struct LoadLevelResult
{
    int result;
    Level *level;
} LoadLevelResult;

LoadLevelResult loadLevel(char *filename);
void unloadLevel(Level *level);

bool saveLevel(Level *level, char *filename);

#endif