#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>
#include <stdbool.h>
#include "config.h"

// Packet Types
typedef enum {
    PACKET_MATRIX_UPDATE = 0x01,
    PACKET_SYNC_REQUEST = 0x02,
    PACKET_SYNC_RESPONSE = 0x03,
    PACKET_LAYER_CHANGE = 0x04,
    PACKET_LED_UPDATE = 0x05,
    PACKET_FEATURE_STATE = 0x06,
    PACKET_HEARTBEAT = 0x07,
    PACKET_BATTERY_STATUS = 0x08
} packet_type_t;

// Matrix State for Transmission
typedef struct __attribute__((packed)) {
    uint8_t rows[MATRIX_ROWS];
    uint8_t changed_mask[MATRIX_ROWS];
} matrix_state_t;

// Feature Sync State
typedef struct __attribute__((packed)) {
    uint8_t active_layers;
    uint8_t oneshot_layer;
    bool oneshot_active;
    uint8_t modtap_states[8];
    uint32_t feature_flags;
    uint8_t led_state;
} feature_sync_t;

// Main Packet Structure
typedef struct __attribute__((packed)) {
    uint8_t type;
    uint8_t device_id;
    uint16_t sequence;
    uint32_t timestamp;
    uint8_t data[32];
    uint16_t checksum;
} keyboard_packet_t;

// Function declarations
uint16_t calculate_checksum(const keyboard_packet_t *packet);
bool validate_packet_checksum(const keyboard_packet_t *packet);

#endif // PROTOCOL_H