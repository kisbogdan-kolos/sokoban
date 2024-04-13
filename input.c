#include "input.h"
#include "tiles.h"
#include "coordinates.h"

#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <stdbool.h>

// Current state of input
typedef struct InputState
{
    char *promptText;
    char *enteredText;
    int maxLength;
    bool upperCase;
    int result;
    SDL_Renderer *renderer;
    SDL_Texture *tiles;
    TTF_Font *font;
} InputState;

// Render input to renderer based on menu state
// Menu state must include proper renderer, tiles and font
static void renderInput(InputState *state)
{
    SDL_Renderer *renderer = state->renderer;
    SDL_Texture *tiles = state->tiles;
    TTF_Font *font = state->font;

    SDL_RenderClear(renderer);

    // brown-ish color for texts
    SDL_Color color = {205, 140, 74, 255};
    SDL_Color white = {255, 255, 255, 255};

    // background floor
    renderTiles(renderer, tiles, greenFloor, 0, 0, 19, 11);

    // key layout for on-screen keyboard
    char *keysLC = "0123456789öüqwertzuiopőúasdfghjkléáűíyxcvbnm,.-ó";  // lowercase
    char *keysUC = "§'\"+!%/=()ÖÜQWERTZUIOPŐÚASDFGHJKLÉÁŰÍYXCVBNM?:_Ó"; // uppercase
    int keyIndex = 0;                                                   // current index of key string

    // render keyboard
    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 12; j++)
        {
            char key[3];                                                     // current key text, characters with accent require 2 chars + \0 at the end
            key[0] = state->upperCase ? keysUC[keyIndex] : keysLC[keyIndex]; // get first char of key based on keyboard case
            keyIndex++;                                                      // next char in key string
            if ((key[0] < 32 || key[0] > 127) && key[0] != 0)                // if current key has an invalid code <==> accent character ==> next character is needed
            {
                key[1] = state->upperCase ? keysUC[keyIndex] : keysLC[keyIndex]; // get next character of key string (to complete accent character)
                keyIndex++;                                                      // next char in key string
                key[2] = '\0';                                                   // terminate string
            }
            else
                key[1] = '\0'; // terminate string

            // renter font and key to correct coordinates
            Coordinates pos = {j + 4, i + 6};
            renderTileC(renderer, tiles, blank, pos);
            renderFontC(renderer, font, color, key, pos, true, true);
        }
    }

    // space bar
    renderTile(renderer, tiles, blankR, 6, 10);
    renderTiles(renderer, tiles, blankLR, 7, 10, 12, 10);
    renderTile(renderer, tiles, blankL, 13, 10);

    // shift keys
    renderTile(renderer, tiles, blank, 3, 9);
    renderFont(renderer, font, color, "^", 3, 9, true, true);
    renderTile(renderer, tiles, blank, 16, 9);
    renderFont(renderer, font, color, "^", 16, 9, true, true);

    // backspace
    renderTile(renderer, tiles, blankR, 16, 6);
    renderTile(renderer, tiles, blankL, 17, 6);
    renderFont(renderer, font, color, "<-", 16, 6, false, true);

    // OK button
    renderTile(renderer, tiles, blankR, 17, 1);
    renderTile(renderer, tiles, blankL, 18, 1);
    renderFont(renderer, font, color, "OK", 17, 1, false, true);

    // back button
    renderTile(renderer, tiles, blankR, 17, 3);
    renderTile(renderer, tiles, blankL, 18, 3);
    renderFont(renderer, font, color, "<-", 17, 3, false, true);

    // render current text with border and prompt
    renderFont(renderer, font, white, state->promptText, 1, 1, false, true);
    renderTiles(renderer, tiles, greyFloor, 1, 2, 15, 4);
    renderTiles(renderer, tiles, wall, 1, 2, 15, 2);
    renderTiles(renderer, tiles, wall, 1, 4, 15, 4);
    renderTile(renderer, tiles, wall, 1, 3);
    renderTile(renderer, tiles, wall, 15, 3);
    renderFont(renderer, font, white, state->enteredText, 2, 3, false, true);

    SDL_RenderPresent(renderer);
}

// Delete last character of current text
static void deleteChar(InputState *state)
{
    char *text = state->enteredText; // for simpler code
    int length = strlen(text);       // get current length

    if (length >= 1) // if there are characters in string
    {
        if ((text[length - 2] < 32 || text[length - 2] > 127) && text[length - 2] != 0 && // last 2 characters are invalid <==> make an accent character together
            (text[length - 1] < 32 || text[length - 1] > 127) && text[length - 1] != 0 &&
            length >= 2) // and length is correct for accent character
        {
            text[length - 2] = 0; // set string end
        }
        else
        {
            text[length - 1] = 0; // set string end
        }
    }
}

// Handle SDL key down event
// Returns true if erernder is needed
static bool handleKeydown(InputState *state, SDL_Keycode key)
{
    bool shift = !state->upperCase;
    switch (key)
    {
    case 0xe5: // right shift
    case 0xe1: // left shift
        state->upperCase = true;
        return true;
    case 0x28: // enter
        state->result = 1;
        return false;
    case 0x29: // esc
        state->result = 2;
        return false;
    case 0x2a: // backspace
        deleteChar(state);
        break;
    case 0x2c: // space
        strcat(state->enteredText, " ");
        break;
    case 0x35: // 0 / §
        strcat(state->enteredText, shift ? "0" : "§");
        break;
    case 0x1e: // 1 / '
        strcat(state->enteredText, shift ? "1" : "'");
        break;
    case 0x1f: // 2 / "
        strcat(state->enteredText, shift ? "2" : "\"");
        break;
    case 0x20: // 3 / +
        strcat(state->enteredText, shift ? "3" : "+");
        break;
    case 0x21: // 4 / !
        strcat(state->enteredText, shift ? "4" : "!");
        break;
    case 0x22: // 5 / %
        strcat(state->enteredText, shift ? "5" : "%");
        break;
    case 0x23: // 6 / /
        strcat(state->enteredText, shift ? "6" : "/");
        break;
    case 0x24: // 7 / =
        strcat(state->enteredText, shift ? "7" : "=");
        break;
    case 0x25: // 8 / (
        strcat(state->enteredText, shift ? "8" : "(");
        break;
    case 0x26: // 9 / )
        strcat(state->enteredText, shift ? "9" : ")");
        break;
    case 0x27: // ö / Ö
        strcat(state->enteredText, shift ? "ö" : "Ö");
        break;
    case 0x2d: // ü / Ü
        strcat(state->enteredText, shift ? "ü" : "Ü");
        break;
    case 0x14: // q / Q
        strcat(state->enteredText, shift ? "q" : "Q");
        break;
    case 0x1a: // w / W
        strcat(state->enteredText, shift ? "w" : "W");
        break;
    case 0x08: // e / E
        strcat(state->enteredText, shift ? "e" : "E");
        break;
    case 0x15: // r / R
        strcat(state->enteredText, shift ? "r" : "R");
        break;
    case 0x17: // t / T
        strcat(state->enteredText, shift ? "t" : "T");
        break;
    case 0x1c: // z / Z
        strcat(state->enteredText, shift ? "z" : "Z");
        break;
    case 0x18: // u / U
        strcat(state->enteredText, shift ? "u" : "U");
        break;
    case 0x0c: // i / I
        strcat(state->enteredText, shift ? "i" : "I");
        break;
    case 0x12: // o / O
        strcat(state->enteredText, shift ? "o" : "O");
        break;
    case 0x13: // p / P
        strcat(state->enteredText, shift ? "p" : "P");
        break;
    case 0x2f: // ő / Ő
        strcat(state->enteredText, shift ? "ő" : "Ő");
        break;
    case 0x30: // ú / Ú
        strcat(state->enteredText, shift ? "ú" : "Ú");
        break;
    case 0x04: // a / A
        strcat(state->enteredText, shift ? "a" : "A");
        break;
    case 0x16: // s / S
        strcat(state->enteredText, shift ? "s" : "S");
        break;
    case 0x07: // d / D
        strcat(state->enteredText, shift ? "d" : "D");
        break;
    case 0x09: // f / F
        strcat(state->enteredText, shift ? "f" : "F");
        break;
    case 0x0a: // g / G
        strcat(state->enteredText, shift ? "g" : "G");
        break;
    case 0x0b: // h / H
        strcat(state->enteredText, shift ? "h" : "H");
        break;
    case 0x0d: // j / J
        strcat(state->enteredText, shift ? "j" : "J");
        break;
    case 0x0e: // k / K
        strcat(state->enteredText, shift ? "k" : "K");
        break;
    case 0x0f: // l / L
        strcat(state->enteredText, shift ? "l" : "L");
        break;
    case 0x33: // é / É
        strcat(state->enteredText, shift ? "é" : "É");
        break;
    case 0x34: // á / Á
        strcat(state->enteredText, shift ? "á" : "Á");
        break;
    case 0x31: // ű / Ű
        strcat(state->enteredText, shift ? "ű" : "Ű");
        break;
    case 0x64: // í / Í
        strcat(state->enteredText, shift ? "í" : "Í");
        break;
    case 0x1d: // y / Y
        strcat(state->enteredText, shift ? "y" : "Y");
        break;
    case 0x1b: // x / X
        strcat(state->enteredText, shift ? "x" : "X");
        break;
    case 0x06: // c / C
        strcat(state->enteredText, shift ? "c" : "C");
        break;
    case 0x19: // v / V
        strcat(state->enteredText, shift ? "v" : "V");
        break;
    case 0x05: // b / B
        strcat(state->enteredText, shift ? "b" : "B");
        break;
    case 0x11: // n / N
        strcat(state->enteredText, shift ? "n" : "N");
        break;
    case 0x10: // m / M
        strcat(state->enteredText, shift ? "m" : "M");
        break;
    case 0x36: // , / ?
        strcat(state->enteredText, shift ? "," : "?");
        break;
    case 0x37: // . / :
        strcat(state->enteredText, shift ? "." : ":");
        break;
    case 0x38: // - / _
        strcat(state->enteredText, shift ? "-" : "_");
        break;
    case 0x2e: // ó / Ó
        strcat(state->enteredText, shift ? "ó" : "Ó");
        break;
    default:
        return false;
        break;
    }
    return true;
}

// Handle SDL key up event
// Returns true if erernder is needed
static bool handleKeyup(InputState *state, SDL_Keycode key)
{
    switch (key)
    {
    case 0xe5: // right shift
    case 0xe1: // left shift
        state->upperCase = false;
        break;
    default:
        return false;
        break;
    }
    return true;
}

// Handle SDL mouse click
// Returns true if rerender is needed
static bool handleClick(InputState *state, int x, int y)
{
    bool shift = !state->upperCase;
    if (clickTile(3, 9, x, y)) // left shift
        state->upperCase = !state->upperCase;
    if (clickTile(16, 9, x, y)) // right shift
        state->upperCase = !state->upperCase;
    if (clickTiles(6, 10, 13, 10, x, y)) // space
        strcat(state->enteredText, " ");
    if (clickTiles(16, 6, 17, 6, x, y)) // backspace
        deleteChar(state);
    if (clickTiles(17, 1, 18, 1, x, y)) // ok button
        state->result = 1;
    if (clickTiles(17, 3, 18, 3, x, y)) // back button
        state->result = 2;

    if (clickTile(4, 6, x, y))
        strcat(state->enteredText, shift ? "0" : "§");
    if (clickTile(5, 6, x, y))
        strcat(state->enteredText, shift ? "1" : "'");
    if (clickTile(6, 6, x, y))
        strcat(state->enteredText, shift ? "2" : "\"");
    if (clickTile(7, 6, x, y))
        strcat(state->enteredText, shift ? "3" : "+");
    if (clickTile(8, 6, x, y))
        strcat(state->enteredText, shift ? "4" : "!");
    if (clickTile(9, 6, x, y))
        strcat(state->enteredText, shift ? "5" : "%");
    if (clickTile(10, 6, x, y))
        strcat(state->enteredText, shift ? "6" : "/");
    if (clickTile(11, 6, x, y))
        strcat(state->enteredText, shift ? "7" : "=");
    if (clickTile(12, 6, x, y))
        strcat(state->enteredText, shift ? "8" : "(");
    if (clickTile(13, 6, x, y))
        strcat(state->enteredText, shift ? "9" : ")");
    if (clickTile(14, 6, x, y))
        strcat(state->enteredText, shift ? "ö" : "Ö");
    if (clickTile(15, 6, x, y))
        strcat(state->enteredText, shift ? "ü" : "Ü");
    if (clickTile(4, 7, x, y))
        strcat(state->enteredText, shift ? "q" : "Q");
    if (clickTile(5, 7, x, y))
        strcat(state->enteredText, shift ? "w" : "W");
    if (clickTile(6, 7, x, y))
        strcat(state->enteredText, shift ? "e" : "E");
    if (clickTile(7, 7, x, y))
        strcat(state->enteredText, shift ? "r" : "R");
    if (clickTile(8, 7, x, y))
        strcat(state->enteredText, shift ? "t" : "T");
    if (clickTile(9, 7, x, y))
        strcat(state->enteredText, shift ? "z" : "Z");
    if (clickTile(10, 7, x, y))
        strcat(state->enteredText, shift ? "u" : "U");
    if (clickTile(11, 7, x, y))
        strcat(state->enteredText, shift ? "i" : "I");
    if (clickTile(12, 7, x, y))
        strcat(state->enteredText, shift ? "o" : "O");
    if (clickTile(13, 7, x, y))
        strcat(state->enteredText, shift ? "p" : "P");
    if (clickTile(14, 7, x, y))
        strcat(state->enteredText, shift ? "ő" : "Ő");
    if (clickTile(15, 7, x, y))
        strcat(state->enteredText, shift ? "ú" : "Ú");
    if (clickTile(4, 8, x, y))
        strcat(state->enteredText, shift ? "a" : "A");
    if (clickTile(5, 8, x, y))
        strcat(state->enteredText, shift ? "s" : "S");
    if (clickTile(6, 8, x, y))
        strcat(state->enteredText, shift ? "d" : "D");
    if (clickTile(7, 8, x, y))
        strcat(state->enteredText, shift ? "f" : "F");
    if (clickTile(8, 8, x, y))
        strcat(state->enteredText, shift ? "g" : "G");
    if (clickTile(9, 8, x, y))
        strcat(state->enteredText, shift ? "h" : "H");
    if (clickTile(10, 8, x, y))
        strcat(state->enteredText, shift ? "j" : "J");
    if (clickTile(11, 8, x, y))
        strcat(state->enteredText, shift ? "k" : "K");
    if (clickTile(12, 8, x, y))
        strcat(state->enteredText, shift ? "l" : "L");
    if (clickTile(13, 8, x, y))
        strcat(state->enteredText, shift ? "é" : "É");
    if (clickTile(14, 8, x, y))
        strcat(state->enteredText, shift ? "á" : "Á");
    if (clickTile(15, 8, x, y))
        strcat(state->enteredText, shift ? "ű" : "Ű");
    if (clickTile(4, 9, x, y))
        strcat(state->enteredText, shift ? "í" : "Í");
    if (clickTile(5, 9, x, y))
        strcat(state->enteredText, shift ? "y" : "Y");
    if (clickTile(6, 9, x, y))
        strcat(state->enteredText, shift ? "x" : "X");
    if (clickTile(7, 9, x, y))
        strcat(state->enteredText, shift ? "c" : "C");
    if (clickTile(8, 9, x, y))
        strcat(state->enteredText, shift ? "v" : "V");
    if (clickTile(9, 9, x, y))
        strcat(state->enteredText, shift ? "b" : "B");
    if (clickTile(10, 9, x, y))
        strcat(state->enteredText, shift ? "n" : "N");
    if (clickTile(11, 9, x, y))
        strcat(state->enteredText, shift ? "m" : "M");
    if (clickTile(12, 9, x, y))
        strcat(state->enteredText, shift ? "," : "?");
    if (clickTile(13, 9, x, y))
        strcat(state->enteredText, shift ? "." : ":");
    if (clickTile(14, 9, x, y))
        strcat(state->enteredText, shift ? "-" : "_");
    if (clickTile(15, 9, x, y))
        strcat(state->enteredText, shift ? "ó" : "Ó");
    return true;
}

// Handle input event
// Returns true if rerender is needed
static bool handleEvent(SDL_Event event, InputState *state)
{
    switch (event.type)
    {
    case SDL_KEYDOWN:
        return handleKeydown(state, event.key.keysym.scancode);
    case SDL_KEYUP:
        return handleKeyup(state, event.key.keysym.scancode);
    case SDL_MOUSEBUTTONDOWN:
        return handleClick(state, event.button.x, event.button.y);
    case SDL_QUIT: // return on quit signal
        state->result = 0;
        return false;
    default:
        return false;
        break;
    }
}

// Input text from user
// Prompt text is displayed to user, input is written to enteredText
// Returns 0 on exit, 1 on success and 2 on user cancel
int textInput(SDL_Renderer *renderer, SDL_Texture *tiles, TTF_Font *font, char *promptText, char *enteredText, int maxLength)
{
    InputState state;

    // set renderer, tiles and font to state for easier function calls
    state.renderer = renderer;
    state.tiles = tiles;
    state.font = font;

    // initialize state variables
    state.promptText = promptText;
    state.enteredText = enteredText;
    state.maxLength = maxLength;
    state.upperCase = false;
    state.result = -1;

    // render initialized state
    renderInput(&state);

    SDL_Event ev;
    while (SDL_WaitEvent(&ev))
    {
        bool rerender = handleEvent(ev, &state);
        if (state.result != -1) // if result was set
        {
            return state.result; // return with result
        }
        if (rerender)
            renderInput(&state);
    }

    return 0;
}

// Display alert to user
// Returns 0 on exit and 1 on user dismiss
int alertBox(SDL_Renderer *renderer, SDL_Texture *tiles, TTF_Font *font, char *promptText)
{
    SDL_RenderClear(renderer);

    // brown-ish color for texts
    SDL_Color color = {205, 140, 74, 255};
    SDL_Color white = {255, 255, 255, 255};

    // background floor
    renderTiles(renderer, tiles, greenFloor, 0, 0, 19, 11);

    // render prompt text with border
    renderTiles(renderer, tiles, greyFloor, 1, 1, 18, 3);
    renderTiles(renderer, tiles, wall, 1, 1, 18, 1);
    renderTiles(renderer, tiles, wall, 1, 3, 18, 3);
    renderTile(renderer, tiles, wall, 1, 2);
    renderTile(renderer, tiles, wall, 18, 2);
    renderFont(renderer, font, white, promptText, 2, 2, false, true);

    // OK button
    renderTile(renderer, tiles, blankR, 8, 5);
    renderTiles(renderer, tiles, blankLR, 9, 5, 10, 5);
    renderTile(renderer, tiles, blankL, 11, 5);
    renderFont(renderer, font, color, "OK", 9, 5, false, true);

    SDL_RenderPresent(renderer);

    SDL_Event ev;
    while (SDL_WaitEvent(&ev))
    {
        switch (ev.type)
        {
        case SDL_KEYDOWN:
            if (ev.key.keysym.scancode == 0x28) // enter
                return 1;
            break;
        case SDL_MOUSEBUTTONDOWN:
            if (clickTiles(8, 5, 11, 5, ev.button.x, ev.button.y)) // ok button
                return 1;
            break;
        case SDL_QUIT: // return on quit signal
            return 0;
        default:
            break;
        }
    }

    return 0;
}

// Display yes/no dialog to user
// Returns 0 on exit and 1 on yes and 2 on no
int dialogBox(SDL_Renderer *renderer, SDL_Texture *tiles, TTF_Font *font, char *promptText)
{
    SDL_RenderClear(renderer);

    // brown-ish color for texts
    SDL_Color color = {205, 140, 74, 255};
    SDL_Color white = {255, 255, 255, 255};

    // background floor
    renderTiles(renderer, tiles, greenFloor, 0, 0, 19, 11);

    // render prompt text with border
    renderTiles(renderer, tiles, greyFloor, 1, 1, 18, 3);
    renderTiles(renderer, tiles, wall, 1, 1, 18, 1);
    renderTiles(renderer, tiles, wall, 1, 3, 18, 3);
    renderTile(renderer, tiles, wall, 1, 2);
    renderTile(renderer, tiles, wall, 18, 2);
    renderFont(renderer, font, white, promptText, 2, 2, false, true);

    // OK button
    renderTile(renderer, tiles, blankR, 5, 5);
    renderTiles(renderer, tiles, blankLR, 6, 5, 7, 5);
    renderTile(renderer, tiles, blankL, 8, 5);
    renderFont(renderer, font, color, "OK", 6, 5, false, true);

    // OK button
    renderTile(renderer, tiles, blankR, 11, 5);
    renderTiles(renderer, tiles, blankLR, 12, 5, 13, 5);
    renderTile(renderer, tiles, blankL, 14, 5);
    renderFont(renderer, font, color, "Mégse", 11, 5, false, true);

    SDL_RenderPresent(renderer);

    SDL_Event ev;
    while (SDL_WaitEvent(&ev))
    {
        switch (ev.type)
        {
        case SDL_KEYDOWN:
            if (ev.key.keysym.scancode == 0x28) // enter
                return 1;
            if (ev.key.keysym.scancode == 0x29) // esc
                return 2;
            break;
        case SDL_MOUSEBUTTONDOWN:
            if (clickTiles(5, 5, 8, 5, ev.button.x, ev.button.y)) // ok button
                return 1;
            if (clickTiles(11, 5, 14, 5, ev.button.x, ev.button.y)) // cancel button
                return 2;
            break;
        case SDL_QUIT: // return on quit signal
            return 0;
        default:
            break;
        }
    }

    return 0;
}