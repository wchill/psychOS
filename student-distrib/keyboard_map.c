#include "terminal.h"
#include "keyboard_map.h"

const uint8_t keyboard_map[4][KEYBOARD_SIZE] = {
{
    0,          // 00 - Error code
    27,         // 01 - Esc
    '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=',
    '\b',       // 0e - Backspace
    '\t',       // 0f - Tab
    'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']',
    '\n',       // 1c - Enter
    0,          // 1d - Left Control
    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0,          // 2a - Left Shift
    '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/',
    0,          // 36 - Right Shift
    '*',        // 37 - Print Screen / Keypad *
    0,          // 38 - Left Alt
    ' ',        // 39 - Space
    0,          // 3a - Caps Lock
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   // 3b to 44 - F1 to F10
    0,          // 45 - Num Lock
    0,          // 46 - Scroll Lock
    0,          // 47 - Home / Keypad 7
    0,          // 48 - Up / Keypad 8
    0,          // 49 - Page Up / Keypad 9
    '-',        // 4a - Keypad "-"
    0,          // 4b - Left / Keypad 4
    0,          // 4c - Keypad 5
    0,          // 4d - Keypad 6
    0,          // 4e - Keypad "+"
    0,          // 4f - End / Keypad 1
    0,          // 50 - Down / Keypad 2
    0,          // 51 - Page Down / Keypad 3
    0,          // 52 - Insert / Keypad 0
    0,          // 53 - Delete / Keypad "."
    0, 0, 0,    // Nonstandard keys
    0,          // 57 - F11
    0,          // 58 - F11
    0,          // Nonstandard keys
},{
    0,          // 00 - Error code
    27,         // 01 - Esc
    '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+',
    '\b',       // 0e - Backspace
    '\t',       // 0f - Tab
    'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}',
    '\n',       // 1c - Enter
    0,          // 1d - Left Control
    'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
    0,          // 2a - Left Shift
    '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?',
    0,          // 36 - Right Shift
    '*',        // 37 - Print Screen / Keypad *
    0,          // 38 - Left Alt
    ' ',        // 39 - Space
    0,          // 3a - Caps Lock
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   // 3b to 44 - F1 to F10
    0,          // 45 - Num Lock
    0,          // 46 - Scroll Lock
    0,          // 47 - Home / Keypad 7
    0,          // 48 - Up / Keypad 8
    0,          // 49 - Page Up / Keypad 9
    '-',        // 4a - Keypad "-"
    0,          // 4b - Left / Keypad 4
    0,          // 4c - Keypad 5
    0,          // 4d - Keypad 6
    0,          // 4e - Keypad "+"
    0,          // 4f - End / Keypad 1
    0,          // 50 - Down / Keypad 2
    0,          // 51 - Page Down / Keypad 3
    0,          // 52 - Insert / Keypad 0
    0,          // 53 - Delete / Keypad "."
    0, 0, 0,    // Nonstandard keys
    0,          // 57 - F11
    0,          // 58 - F11
    0,          // Nonstandard keys
},{
    0,          // 00 - Error code
    27,         // 01 - Esc
    '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=',
    '\b',       // 0e - Backspace
    '\t',       // 0f - Tab
    'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '[', ']',
    '\n',       // 1c - Enter
    0,          // 1d - Left Control
    'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ';', '\'', '`',
    0,          // 2a - Left Shift
    '\\', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', ',', '.', '/',
    0,          // 36 - Right Shift
    '*',        // 37 - Print Screen / Keypad *
    0,          // 38 - Left Alt
    ' ',        // 39 - Space
    0,          // 3a - Caps Lock
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   // 3b to 44 - F1 to F10
    0,          // 45 - Num Lock
    0,          // 46 - Scroll Lock
    0,          // 47 - Home / Keypad 7
    0,          // 48 - Up / Keypad 8
    0,          // 49 - Page Up / Keypad 9
    '-',        // 4a - Keypad "-"
    0,          // 4b - Left / Keypad 4
    0,          // 4c - Keypad 5
    0,          // 4d - Keypad 6
    0,          // 4e - Keypad "+"
    0,          // 4f - End / Keypad 1
    0,          // 50 - Down / Keypad 2
    0,          // 51 - Page Down / Keypad 3
    0,          // 52 - Insert / Keypad 0
    0,          // 53 - Delete / Keypad "."
    0, 0, 0,    // Nonstandard keys
    0,          // 57 - F11
    0,          // 58 - F11
    0,          // Nonstandard keys
},{
    0,          // 00 - Error code
    27,         // 01 - Esc
    '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+',
    '\b',       // 0e - Backspace
    '\t',       // 0f - Tab
    'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '{', '}',
    '\n',       // 1c - Enter
    0,          // 1d - Left Control
    'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ':', '"', '~',
    0,          // 2a - Left Shift
    '|', 'z', 'x', 'c', 'v', 'b', 'n', 'm', '<', '>', '?',
    0,          // 36 - Right Shift
    '*',        // 37 - Print Screen / Keypad *
    0,          // 38 - Left Alt
    ' ',        // 39 - Space
    0,          // 3a - Caps Lock
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0,   // 3b to 44 - F1 to F10
    0,          // 45 - Num Lock
    0,          // 46 - Scroll Lock
    0,          // 47 - Home / Keypad 7
    0,          // 48 - Up / Keypad 8
    0,          // 49 - Page Up / Keypad 9
    '-',        // 4a - Keypad "-"
    0,          // 4b - Left / Keypad 4
    0,          // 4c - Keypad 5
    0,          // 4d - Keypad 6
    0,          // 4e - Keypad "+"
    0,          // 4f - End / Keypad 1
    0,          // 50 - Down / Keypad 2
    0,          // 51 - Page Down / Keypad 3
    0,          // 52 - Insert / Keypad 0
    0,          // 53 - Delete / Keypad "."
    0, 0, 0,    // Nonstandard keys
    0,          // 57 - F11
    0,          // 58 - F11
    0,          // Nonstandard keys
}};
