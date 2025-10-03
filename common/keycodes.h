#ifndef KEYCODES_H
#define KEYCODES_H

#include <stdint.h>

// Basic Keycodes (QMK-compatible)
enum keycodes {
    KC_NO = 0x00,
    KC_TRNS = 0x01,
    
    // Letters
    KC_A = 0x04,
    KC_B = 0x05,
    KC_C = 0x06,
    KC_D = 0x07,
    KC_E = 0x08,
    KC_F = 0x09,
    KC_G = 0x0A,
    KC_H = 0x0B,
    KC_I = 0x0C,
    KC_J = 0x0D,
    KC_K = 0x0E,
    KC_L = 0x0F,
    KC_M = 0x10,
    KC_N = 0x11,
    KC_O = 0x12,
    KC_P = 0x13,
    KC_Q = 0x14,
    KC_R = 0x15,
    KC_S = 0x16,
    KC_T = 0x17,
    KC_U = 0x18,
    KC_V = 0x19,
    KC_W = 0x1A,
    KC_X = 0x1B,
    KC_Y = 0x1C,
    KC_Z = 0x1D,
    
    // Numbers
    KC_1 = 0x1E,
    KC_2 = 0x1F,
    KC_3 = 0x20,
    KC_4 = 0x21,
    KC_5 = 0x22,
    KC_6 = 0x23,
    KC_7 = 0x24,
    KC_8 = 0x25,
    KC_9 = 0x26,
    KC_0 = 0x27,
    
    // Special Keys
    KC_ENT = 0x28,
    KC_ESC = 0x29,
    KC_BSPC = 0x2A,
    KC_TAB = 0x2B,
    KC_SPC = 0x2C,
    KC_MINS = 0x2D,
    KC_EQL = 0x2E,
    KC_LBRC = 0x2F,
    KC_RBRC = 0x30,
    KC_BSLS = 0x31,
    KC_SCLN = 0x33,
    KC_QUOT = 0x34,
    KC_GRV = 0x35,
    KC_COMM = 0x36,
    KC_DOT = 0x37,
    KC_SLSH = 0x38,
    KC_CAPS = 0x39,
    
    // Function Keys
    KC_F1 = 0x3A,
    KC_F2 = 0x3B,
    KC_F3 = 0x3C,
    KC_F4 = 0x3D,
    KC_F5 = 0x3E,
    KC_F6 = 0x3F,
    KC_F7 = 0x40,
    KC_F8 = 0x41,
    KC_F9 = 0x42,
    KC_F10 = 0x43,
    KC_F11 = 0x44,
    KC_F12 = 0x45,
    
    // Navigation
    KC_INS = 0x49,
    KC_HOME = 0x4A,
    KC_PGUP = 0x4B,
    KC_DEL = 0x4C,
    KC_END = 0x4D,
    KC_PGDN = 0x4E,
    KC_RIGHT = 0x4F,
    KC_LEFT = 0x50,
    KC_DOWN = 0x51,
    KC_UP = 0x52,
    
    // Modifiers
    KC_LCTL = 0xE0,
    KC_LSFT = 0xE1,
    KC_LALT = 0xE2,
    KC_LGUI = 0xE3,
    KC_RCTL = 0xE4,
    KC_RSFT = 0xE5,
    KC_RALT = 0xE6,
    KC_RGUI = 0xE7,
};

// Shifted symbols - these map to the number keys with shift
#define KC_EXLM KC_1    // !
#define KC_AT   KC_2    // @
#define KC_HASH KC_3    // #
#define KC_DLR  KC_4    // $
#define KC_PERC KC_5    // %
#define KC_CIRC KC_6    // ^
#define KC_AMPR KC_7    // &
#define KC_ASTR KC_8    // *
#define KC_LPRN KC_9    // (
#define KC_RPRN KC_0    // )

// Layer Keycodes
#define MO(n) (0x5100 | (n))      // Momentary layer
#define TG(n) (0x5200 | (n))      // Toggle layer
#define TO(n) (0x5300 | (n))      // Move to layer
#define TT(n) (0x5400 | (n))      // Tap toggle
#define OSL(n) (0x5500 | (n))     // One-shot layer

// Mod-Tap Keycodes
#define MT(mod, kc) ((mod) << 8 | (kc))
#define CTL_T(kc) MT(KC_LCTL, kc)
#define SFT_T(kc) MT(KC_LSFT, kc)
#define ALT_T(kc) MT(KC_LALT, kc)
#define GUI_T(kc) MT(KC_LGUI, kc)

// Tap Dance
#define TD(n) (0x5600 | (n))

// System keys
#define RESET 0x5C00

// Combos
#define COMBO_END 0xFFFF

// Shortcuts
#define _______ KC_TRNS
#define XXXXXXX KC_NO

// Macro keycodes for common shortcuts
#define KC_COPY  0x7001
#define KC_PASTE 0x7002
#define KC_CUT   0x7003
#define KC_UNDO  0x7004
#define KC_REDO  0x7005

// Mouse Movement Keycodes (0x7100-0x71FF range)
#define KC_MS_UP    0x7100
#define KC_MS_DOWN  0x7101
#define KC_MS_LEFT  0x7102
#define KC_MS_RIGHT 0x7103

// Mouse Button Keycodes
#define KC_MS_BTN1  0x7104  // Left click
#define KC_MS_BTN2  0x7105  // Right click
#define KC_MS_BTN3  0x7106  // Middle click
#define KC_MS_BTN4  0x7107  // Back button
#define KC_MS_BTN5  0x7108  // Forward button

// Mouse Wheel Keycodes
#define KC_MS_WH_UP    0x7109
#define KC_MS_WH_DOWN  0x710A
#define KC_MS_WH_LEFT  0x710B
#define KC_MS_WH_RIGHT 0x710C

// Mouse Speed Control
#define KC_MS_ACCEL0  0x710D  // Slowest
#define KC_MS_ACCEL1  0x710E  // Slow
#define KC_MS_ACCEL2  0x710F  // Normal (default)

// Auto-Clicker Keycodes (0x7200-0x72FF range)
#define KC_AC_TOGGLE  0x7200  // Toggle auto-clicker on/off
#define KC_AC_LEFT    0x7201  // Auto-click left button
#define KC_AC_RIGHT   0x7202  // Auto-click right button
#define KC_AC_MIDDLE  0x7203  // Auto-click middle button
#define KC_AC_FASTER  0x7204  // Increase click rate
#define KC_AC_SLOWER  0x7205  // Decrease click rate

// Combined Mouse + Modifier Keycodes (for one-handed mouse control)
#define KC_MS_L_SHIFT  0x7210  // Mouse left + shift held
#define KC_MS_L_CTRL   0x7211  // Mouse left + ctrl held

// Mouse sensitivity presets
#define MOUSE_SPEED_SLOW     2
#define MOUSE_SPEED_NORMAL   8
#define MOUSE_SPEED_FAST     16

// Auto-clicker intervals (in milliseconds)
#define AC_INTERVAL_MIN      10    // 100 clicks/sec max
#define AC_INTERVAL_MAX      2000  // 0.5 clicks/sec min
#define AC_INTERVAL_DEFAULT  100   // 10 clicks/sec default
#define AC_INTERVAL_STEP     10    // Adjustment step

// For compatibility with AVR PROGMEM (not needed on ARM but keeps compatibility)
#define PROGMEM

#endif // KEYCODES_H