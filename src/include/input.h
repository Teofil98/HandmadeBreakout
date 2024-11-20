#pragma once

enum key_id {
    KEY_W,
    KEY_A,
    KEY_S,
    KEY_D,
    KEY_ESC,
    KEY_SPACE,

    NUM_KEYS
};


struct key_state {
    bool pressed;
    bool held;
    bool released;
};

extern key_state keys[NUM_KEYS];
