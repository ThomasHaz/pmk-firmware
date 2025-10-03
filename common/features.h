#ifndef FEATURES_H
#define FEATURES_H

#include <stdint.h>
#include <stdbool.h>
#include "config.h"
#include "keycodes.h"

// Feature state structure
typedef struct {
    uint8_t layer_state;
    uint32_t feature_flags;
    bool oneshot_active;
    uint8_t oneshot_layer;
} feature_state_t;

// Function declarations for all features
void init_features(void);
void features_task(void);

// Layer functions
void layer_on(uint8_t layer);
void layer_off(uint8_t layer);
void layer_toggle(uint8_t layer);
void layer_clear(void);
uint8_t get_highest_layer(void);
uint8_t get_layer_state(void);
bool process_layer_keycode(uint16_t keycode, bool pressed);

// Mod-tap functions
bool process_modtap(uint16_t keycode, bool pressed, uint8_t row, uint8_t col);
void modtap_task(void);

// One-shot functions
bool process_oneshot_layer(uint16_t keycode, bool pressed);
void oneshot_task(void);
uint8_t get_oneshot_layer(void);
bool is_oneshot_active(void);

// Combo functions
bool process_combo(uint16_t keycode, bool pressed);
void init_combos(void);

// Tap dance functions
bool process_tap_dance(uint16_t keycode, bool pressed, uint8_t row, uint8_t col);
void tap_dance_task(void);

// Utility functions
uint32_t get_feature_flags(void);
uint16_t get_keycode_at(uint8_t layer, uint8_t row, uint8_t col);

#endif // FEATURES_H