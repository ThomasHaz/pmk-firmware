#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>
#include <stdbool.h>

// Hardware Configuration
#define MATRIX_ROWS 4
#define MATRIX_COLS 6
#define DEBOUNCE_MS 5

// // Network Configuration - INFRASTRUCTURE MODE
// // All devices connect to your home WiFi
#define WIFI_SSID "MY_NETWORK"     // Replace with your WiFi name
#define WIFI_PASS "NET_PASS" // Replace with your WiFi password

// Static IP addresses (on your home network)
// Choose IPs that won't conflict with other devices
#define DONGLE_IP "172.16.13.10"  // Change to match your network (192.168.1.x or 192.168.0.x)
#define LEFT_IP   "172.16.13.11"
#define RIGHT_IP  "172.16.13.12"


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
// GPIO 0 & 1 can be used for matrix since we're using WiFi instead of UART
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