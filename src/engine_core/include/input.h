#pragma once

#include <stdbool.h>

typedef enum key_id {
    KEY_W,
    KEY_R,
    KEY_A,
    KEY_S,
    KEY_D,
    KEY_ESC,
    KEY_SPACE,

    NUM_KEYS
} key_id;


typedef struct key_state {
    bool pressed;
    bool held;
    bool released;
} key_state;

extern key_state keys[NUM_KEYS];
