#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>
#include <stdbool.h>

// Hardware Configuration
#define MATRIX_ROWS 4
#define MATRIX_COLS 6
#define DEBOUNCE_MS 5

// Network Configuration - AP MODE
// The dongle creates an access point, keyboard halves connect to it
#define WIFI_SSID "KBSPLIT"
#define WIFI_PASS "keyboard"  // Simple 8-char password
#define WIFI_CHANNEL 6             // Fixed channel 1-11

// IP Configuration for AP mode
#define DONGLE_IP "192.168.4.1"    // AP gateway address
#define LEFT_IP   "192.168.4.2"
#define RIGHT_IP  "192.168.4.3"
#define SUBNET_MASK "255.255.255.0"

#define KB_PORT 4242
#define PACKET_TIMEOUT_MS 1000

// Feature Configuration
#define TAPPING_TERM 200
#define ONESHOT_TIMEOUT 3000
#define COMBO_TERM 30
#define TAP_DANCE_TERM 200
#define MAX_LAYERS 8
#define MAX_COMBOS 32
#define MAX_TAP_DANCE 16
#define MAX_MODTAP_KEYS 8

// Pin Definitions for Matrix (GPIO pins)
#define ROW_PINS {2, 3, 4, 5}
#define COL_PINS {10, 11, 12, 13, 14, 15}

// LED Pins (optional)
#define LED_CAPS_LOCK_PIN 16
#define LED_NUM_LOCK_PIN 17
#define LED_SCROLL_LOCK_PIN 18

// Device Identification
typedef enum {
    DEVICE_DONGLE = 0,
    DEVICE_LEFT = 1,
    DEVICE_RIGHT = 2
} device_type_t;

// Feature flags
#define FEATURE_MODTAP    (1 << 0)
#define FEATURE_LAYERS    (1 << 1)
#define FEATURE_ONESHOT   (1 << 2)
#define FEATURE_COMBOS    (1 << 3)
#define FEATURE_TAPDANCE  (1 << 4)

#endif // CONFIG_H