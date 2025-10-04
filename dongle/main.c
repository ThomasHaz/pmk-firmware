#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "cyw43.h"
#include "hardware/timer.h"
#include "lwip/pbuf.h"
#include "lwip/udp.h"
#include "lwip/netif.h"
#include "tusb.h"
#include "bsp/board.h"

#include "config.h"
#include "protocol.h"
#include "features.h"
#include "usb_hid.h"
#include "mouse.h"
#include "timer.h"

// State Management
typedef struct {
    matrix_state_t left_matrix;
    matrix_state_t right_matrix;
    uint32_t left_last_seen;
    uint32_t right_last_seen;
    uint16_t left_last_sequence;
    uint16_t right_last_sequence;
    bool left_connected;
    bool right_connected;
} dongle_state_t;

static dongle_state_t state = {0};
static struct udp_pcb *udp_pcb = NULL;
static keyboard_packet_t tx_packet;
static keyboard_packet_t rx_packet;
static bool wifi_ready = false;

extern void process_key_event(uint8_t row, uint8_t col, bool pressed);

uint16_t calculate_checksum(const keyboard_packet_t *packet) {
    uint16_t sum = 0;
    const uint8_t *data = (const uint8_t *)packet;
    for (size_t i = 0; i < offsetof(keyboard_packet_t, checksum); i++) {
        sum += data[i];
    }
    return sum;
}

bool validate_packet_checksum(const keyboard_packet_t *packet) {
    return calculate_checksum(packet) == packet->checksum;
}

static void process_matrix_update(uint8_t device_id, matrix_state_t *matrix) {
    matrix_state_t *target = (device_id == DEVICE_LEFT) ? 
                            &state.left_matrix : &state.right_matrix;
    
    // Process only the CHANGES, not the entire matrix
    for (uint8_t row = 0; row < MATRIX_ROWS; row++) {
        uint8_t old_row = target->rows[row];
        uint8_t new_row = matrix->rows[row];
        
        // Use the changed_mask if provided, otherwise calculate it
        uint8_t changed = matrix->changed_mask[row];
        if (changed == 0) {
            changed = old_row ^ new_row;
        }
        
        if (changed == 0) continue; // No changes in this row
        
        for (uint8_t col = 0; col < MATRIX_COLS; col++) {
            if (!(changed & (1 << col))) continue; // This key didn't change
            
            uint8_t actual_col = (device_id == DEVICE_RIGHT) ? 
                                col + MATRIX_COLS : col;
            
            uint8_t layer = get_highest_layer();
            uint16_t keycode = get_keycode_at(layer, row, actual_col);
            
            bool is_pressed = (new_row & (1 << col)) != 0;
            bool was_pressed = (old_row & (1 << col)) != 0;
            
            // Only process if state actually changed
            if (is_pressed != was_pressed) {
                // Process through the feature pipeline first
                if (!process_combo(keycode, is_pressed)) continue;
                if (!process_tap_dance(keycode, is_pressed, row, actual_col)) continue;
                if (!process_modtap(keycode, is_pressed, row, actual_col)) continue;
                if (!process_layer_keycode(keycode, is_pressed)) continue;
                if (!process_oneshot_layer(keycode, is_pressed)) continue;
                
                // Process mouse keycodes
                if ((keycode & 0xFF00) == 0x7100) {
                    process_mouse_keycode(keycode, is_pressed);
                    continue;
                }
                
                // Process auto-clicker keycodes
                if ((keycode & 0xFF00) == 0x7200) {
                    process_autoclicker_keycode(keycode, is_pressed);
                    continue;
                }
                
                // Handle special keycodes
                if (keycode == RESET && is_pressed) {
                    clear_keyboard();
                    send_hid_report();
                    continue;
                }
                
                // Finally, handle normal keys
                if (is_pressed) {
                    register_key(keycode);
                } else {
                    unregister_key(keycode);
                }
            }
        }
        
        // Update stored state for this row
        target->rows[row] = new_row;
    }
    
    // Send HID report after processing all changes
    send_hid_report();
    
    // Update last seen timestamp
    uint32_t now = timer_read();
    if (device_id == DEVICE_LEFT) {
        state.left_last_seen = now;
        state.left_connected = true;
    } else {
        state.right_last_seen = now;
        state.right_connected = true;
    }
}

static void send_ack_packet(uint8_t device_id, uint16_t sequence) {
    if (udp_pcb == NULL) return;
    
    tx_packet.type = PACKET_SYNC_RESPONSE;
    tx_packet.device_id = DEVICE_DONGLE;
    tx_packet.sequence = sequence;
    tx_packet.timestamp = timer_read();
    tx_packet.checksum = calculate_checksum(&tx_packet);
    
    struct pbuf *p = pbuf_alloc(PBUF_TRANSPORT, sizeof(tx_packet), PBUF_RAM);
    if (p != NULL) {
        memcpy(p->payload, &tx_packet, sizeof(tx_packet));
        
        // Send ACK back to the appropriate half
        ip_addr_t target_addr;
        if (device_id == DEVICE_LEFT) {
            ipaddr_aton(LEFT_IP, &target_addr);
        } else {
            ipaddr_aton(RIGHT_IP, &target_addr);
        }
        
        udp_sendto(udp_pcb, p, &target_addr, KB_PORT);
        pbuf_free(p);
    }
}

static void udp_recv_callback(void *arg, struct udp_pcb *pcb, struct pbuf *p,
                             const ip_addr_t *addr, u16_t port) {
    (void)arg; (void)pcb; (void)addr; (void)port;
    
    if (p != NULL) {
        if (p->tot_len >= sizeof(keyboard_packet_t)) {
            pbuf_copy_partial(p, &rx_packet, sizeof(keyboard_packet_t), 0);
            
            if (validate_packet_checksum(&rx_packet)) {
                if (rx_packet.type == PACKET_MATRIX_UPDATE) {
                    // Check sequence number to avoid processing duplicates
                    uint16_t *last_seq = (rx_packet.device_id == DEVICE_LEFT) ? 
                                        &state.left_last_sequence : 
                                        &state.right_last_sequence;
                    
                    // Allow for some out-of-order packets (sequence within 10 of last)
                    int16_t seq_diff = (int16_t)(rx_packet.sequence - *last_seq);
                    
                    if (seq_diff > 0 || seq_diff < -100) {
                        // New packet or very old packet (likely wrapped around)
                        process_matrix_update(rx_packet.device_id, 
                                            (matrix_state_t *)rx_packet.data);
                        *last_seq = rx_packet.sequence;
                        
                        // Send ACK back
                        send_ack_packet(rx_packet.device_id, rx_packet.sequence);
                        
                        // Very brief LED flash for feedback
                        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
                        sleep_us(100);
                        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
                    }
                    // else: duplicate or slightly out-of-order, still send ACK
                    else {
                        send_ack_packet(rx_packet.device_id, rx_packet.sequence);
                    }
                    
                } else if (rx_packet.type == PACKET_HEARTBEAT) {
                    uint32_t now = timer_read();
                    if (rx_packet.device_id == DEVICE_LEFT) {
                        state.left_last_seen = now;
                        state.left_connected = true;
                    } else if (rx_packet.device_id == DEVICE_RIGHT) {
                        state.right_last_seen = now;
                        state.right_connected = true;
                    }
                }
            }
        }
        pbuf_free(p);
    }
}

int main() {
    stdio_init_all();
    sleep_ms(1000);
    
    printf("\n=== Wireless Keyboard Dongle (AP Mode) ===\n");
    
    // USB first - this is critical for keyboard detection
    printf("1. USB init...\n");
    board_init();
    tusb_init();
    
    // Wait for USB enumeration with continuous task polling
    printf("2. Waiting for USB enumeration...\n");
    uint32_t start = to_ms_since_boot(get_absolute_time());
    while (!tud_mounted() && (to_ms_since_boot(get_absolute_time()) - start) < 5000) {
        tud_task();
        sleep_ms(1);
    }
    
    if (tud_mounted()) {
        printf("   USB MOUNTED - Keyboard detected by host\n");
    } else {
        printf("   USB TIMEOUT - but continuing anyway\n");
    }
    
    // Keep USB task running during stabilization
    printf("3. Stabilizing USB connection...\n");
    start = to_ms_since_boot(get_absolute_time());
    while ((to_ms_since_boot(get_absolute_time()) - start) < 1000) {
        tud_task();
        sleep_ms(1);
    }
    printf("   OK - USB should be stable now\n");
    
    // Features (before WiFi)
    printf("4. Features init...\n");
    init_features();
    init_combos();
    mouse_init();
    autoclicker_init();
    printf("   OK\n");
    
    // NOW start WiFi - but keep tud_task() running
    printf("\n=== Starting WiFi (USB task will keep running) ===\n");
    
    // WiFi chip initialization with USB task running
    printf("5. WiFi chip init...\n");
    if (cyw43_arch_init()) {
        printf("   FATAL\n");
        while(1) {
            tud_task();
            sleep_ms(100);
        }
    }
    printf("   OK\n");
    
    // Keep USB alive during AP setup
    printf("6. Enabling AP mode...\n");
    cyw43_arch_enable_ap_mode(WIFI_SSID, WIFI_PASS, CYW43_AUTH_WPA2_AES_PSK);
    printf("   OK - AP name: %s\n", WIFI_SSID);
    
    // Give the AP time to initialize while keeping USB alive
    printf("7. AP stabilizing...\n");
    start = to_ms_since_boot(get_absolute_time());
    while ((to_ms_since_boot(get_absolute_time()) - start) < 2000) {
        tud_task();
        cyw43_arch_poll();
        sleep_ms(1);
    }
    printf("   OK\n");
    
    // Configure IP address
    printf("8. Configuring IP...\n");
    ip4_addr_t ip, mask;
    ipaddr_aton(DONGLE_IP, &ip);
    ipaddr_aton(SUBNET_MASK, &mask);
    
    struct netif *netif = &cyw43_state.netif[CYW43_ITF_AP];
    netif_set_addr(netif, &ip, &mask, &ip);
    printf("   IP: %s\n", DONGLE_IP);
    
    // Note: Using static IPs on halves, no DHCP server needed
    printf("   (Keyboard halves use static IPs - no DHCP needed)\n");
    
    // Give network stack time to stabilize while keeping USB alive
    start = to_ms_since_boot(get_absolute_time());
    while ((to_ms_since_boot(get_absolute_time()) - start) < 1000) {
        tud_task();
        cyw43_arch_poll();
        sleep_ms(1);
    }
    
    // Setup UDP
    printf("9. Setting up UDP...\n");
    udp_pcb = udp_new();
    if (udp_pcb != NULL && udp_bind(udp_pcb, IP_ADDR_ANY, KB_PORT) == ERR_OK) {
        udp_recv(udp_pcb, udp_recv_callback, NULL);
        wifi_ready = true;
        printf("   OK - Listening on port %d\n", KB_PORT);
        
        // Long blink = ready (with USB task running)
        for (int i = 0; i < 3; i++) {
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
            for (int j = 0; j < 50; j++) {
                tud_task();
                cyw43_arch_poll();
                sleep_ms(10);
            }
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
            for (int j = 0; j < 50; j++) {
                tud_task();
                cyw43_arch_poll();
                sleep_ms(10);
            }
        }
    } else {
        printf("   FAILED\n");
        // Fast blinks = UDP failed
        while(1) {
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
            tud_task();
            sleep_ms(100);
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
            tud_task();
            sleep_ms(100);
        }
    }
    
    printf("\n=== Dongle ready! Waiting for keyboard halves... ===\n");
    
    // Initialize state
    memset(&state, 0, sizeof(state));
    
    uint32_t last_status_check = 0;
    uint32_t last_usb_check = 0;
    uint32_t last_feature_task = 0;
    
    // Main loop - USB TASK MUST BE FIRST
    while (1) {
        uint32_t now = timer_read();
        
        // USB task is highest priority - run frequently
        tud_task();
        
        // WiFi polling for AP mode
        cyw43_arch_poll();
        
        // Send HID reports if needed
        send_hid_report();
        
        // Run feature tasks periodically (every 5ms)
        if (now - last_feature_task > 5) {
            mouse_task();
            autoclicker_task();
            modtap_task();
            tap_dance_task();
            oneshot_task();
            last_feature_task = now;
        }
        
        // Check for disconnected halves (timeout after 2 seconds)
        if (state.left_connected && (now - state.left_last_seen > 2000)) {
            state.left_connected = false;
            // Clear any stuck keys from left half
            for (uint8_t row = 0; row < MATRIX_ROWS; row++) {
                for (uint8_t col = 0; col < MATRIX_COLS; col++) {
                    if (state.left_matrix.rows[row] & (1 << col)) {
                        uint8_t layer = get_highest_layer();
                        uint16_t keycode = get_keycode_at(layer, row, col);
                        unregister_key(keycode);
                    }
                }
                state.left_matrix.rows[row] = 0;
            }
            send_hid_report();
            printf("Left half disconnected - cleared keys\n");
        }
        
        if (state.right_connected && (now - state.right_last_seen > 2000)) {
            state.right_connected = false;
            // Clear any stuck keys from right half
            for (uint8_t row = 0; row < MATRIX_ROWS; row++) {
                for (uint8_t col = 0; col < MATRIX_COLS; col++) {
                    if (state.right_matrix.rows[row] & (1 << col)) {
                        uint8_t layer = get_highest_layer();
                        uint16_t keycode = get_keycode_at(layer, row, col + MATRIX_COLS);
                        unregister_key(keycode);
                    }
                }
                state.right_matrix.rows[row] = 0;
            }
            send_hid_report();
            printf("Right half disconnected - cleared keys\n");
        }
        
        // LED status indicator every 2 seconds
        if (now - last_status_check > 2000) {
            last_status_check = now;
            
            if (state.left_connected && state.right_connected) {
                // Both connected - 2 quick blinks
                for (int i = 0; i < 2; i++) {
                    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
                    sleep_ms(100);
                    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
                    sleep_ms(100);
                }
            } else if (state.left_connected || state.right_connected) {
                // One connected - 1 blink
                cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
                sleep_ms(200);
                cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
            } else {
                // None connected - long blink
                cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
                sleep_ms(500);
                cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
            }
        }
        
        // Small consistent delay
        sleep_us(1000);  // 1ms = 1000 Hz loop rate
    }
    
    return 0;
}