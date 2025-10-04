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
    
    for (uint8_t row = 0; row < MATRIX_ROWS; row++) {
        if (matrix->changed_mask[row]) {
            uint8_t old_row = target->rows[row];
            uint8_t new_row = matrix->rows[row];
            uint8_t changes = old_row ^ new_row;
            
            for (uint8_t col = 0; col < MATRIX_COLS; col++) {
                if (changes & (1 << col)) {
                    bool pressed = new_row & (1 << col);
                    uint8_t actual_col = (device_id == DEVICE_RIGHT) ? 
                                        col + MATRIX_COLS : col;
                    
                    // Use full keymap processing with all features
                    process_key_event(row, actual_col, pressed);
                }
            }
            target->rows[row] = new_row;
        }
    }
    
    uint32_t now = timer_read();
    if (device_id == DEVICE_LEFT) {
        state.left_last_seen = now;
    } else {
        state.right_last_seen = now;
    }
}

static void udp_recv_callback(void *arg, struct udp_pcb *pcb, struct pbuf *p,
                             const ip_addr_t *addr, u16_t port) {
    (void)arg; (void)pcb; (void)addr; (void)port;
    
    if (p != NULL) {
        // Flash LED to show we received something
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
        
        if (p->tot_len >= sizeof(keyboard_packet_t)) {
            pbuf_copy_partial(p, &rx_packet, sizeof(keyboard_packet_t), 0);
            
            if (validate_packet_checksum(&rx_packet)) {
                if (rx_packet.type == PACKET_MATRIX_UPDATE) {
                    process_matrix_update(rx_packet.device_id, 
                                        (matrix_state_t *)rx_packet.data);
                } else if (rx_packet.type == PACKET_HEARTBEAT) {
                    uint32_t now = timer_read();
                    if (rx_packet.device_id == DEVICE_LEFT) {
                        state.left_last_seen = now;
                    } else if (rx_packet.device_id == DEVICE_RIGHT) {
                        state.right_last_seen = now;
                    }
                }
            }
        }
        pbuf_free(p);
        
        sleep_ms(10);
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
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
    while (!tud_mounted() && (to_ms_since_boot(get_absolute_time()) - start) < 10000) {
        tud_task();
        sleep_ms(1);
    }
    
    if (tud_mounted()) {
        printf("   USB MOUNTED - Keyboard detected by host\n");
    } else {
        printf("   USB TIMEOUT - but continuing anyway\n");
    }
    
    // Keep USB task running during stabilization
    printf("3. Stabilizing USB connection (keeping tud_task running)...\n");
    start = to_ms_since_boot(get_absolute_time());
    while ((to_ms_since_boot(get_absolute_time()) - start) < 3000) {
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
    while ((to_ms_since_boot(get_absolute_time()) - start) < 2000) {
        tud_task();
        cyw43_arch_poll();
        sleep_ms(1);
    }
    
    // Setup UDP
    printf("11. Setting up UDP...\n");
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
    printf("Testing USB HID by sending 'X' once...\n");
    
    // Test that USB HID actually works
    sleep_ms(2000);
    // register_key(KC_X);
    // send_hid_report();
    // sleep_ms(100);
    // unregister_key(KC_X);
    // send_hid_report();
    
    printf("If you saw 'X' appear, USB HID is working!\n\n");
    
    uint32_t last_status_check = 0;
    
    // Main loop - USB TASK MUST BE FIRST
    while (1) {
        // USB task is highest priority
        tud_task();
        
        // WiFi polling for AP mode
        cyw43_arch_poll();
        
        uint32_t now = timer_read();
        
        // HID and feature processing
        send_hid_report();
        mouse_task();
        autoclicker_task();
        modtap_task();
        tap_dance_task();
        oneshot_task();
        
        // LED status indicator every 2 seconds
        if (now - last_status_check > 2000) {
            last_status_check = now;
            
            uint32_t left_age = now - state.left_last_seen;
            uint32_t right_age = now - state.right_last_seen;
            
            bool left_connected = (left_age < 2000);
            bool right_connected = (right_age < 2000);
            
            if (left_connected && right_connected) {
                // Both connected - 2 quick blinks
                for (int i = 0; i < 2; i++) {
                    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
                    sleep_ms(100);
                    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
                    sleep_ms(100);
                }
            } else if (left_connected || right_connected) {
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
        
        sleep_ms(1);
    }
    
    return 0;
}