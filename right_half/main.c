#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "hardware/gpio.h"
#include "lwip/pbuf.h"
#include "lwip/udp.h"
#include "lwip/netif.h"
#include "lwip/dhcp.h"

#include "config.h"
#include "protocol.h"
#include "timer.h"
#include "matrix.h"

#define DEVICE_ID DEVICE_RIGHT
#define TEST_MODE 0  // Set to 0 for real matrix

static struct udp_pcb *udp_pcb = NULL;
static keyboard_packet_t tx_packet;
static bool connected = false;

#if !TEST_MODE
static uint8_t previous_matrix[MATRIX_ROWS] = {0};
#endif

// LED blink patterns
void blink_pattern(int count) {
    for (int i = 0; i < count; i++) {
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
        sleep_ms(200);
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
        sleep_ms(200);
    }
    sleep_ms(1000);
}

uint16_t calculate_checksum(const keyboard_packet_t *packet) {
    uint16_t sum = 0;
    const uint8_t *data = (const uint8_t *)packet;
    for (size_t i = 0; i < offsetof(keyboard_packet_t, checksum); i++) {
        sum += data[i];
    }
    return sum;
}

static void send_matrix_update(matrix_state_t *matrix) {
    if (!connected || udp_pcb == NULL) return;
    
    tx_packet.type = PACKET_MATRIX_UPDATE;
    tx_packet.device_id = DEVICE_ID;
    tx_packet.timestamp = timer_read();
    memcpy(tx_packet.data, matrix, sizeof(matrix_state_t));
    tx_packet.checksum = calculate_checksum(&tx_packet);
    
    struct pbuf *p = pbuf_alloc(PBUF_TRANSPORT, sizeof(tx_packet), PBUF_RAM);
    if (p != NULL) {
        memcpy(p->payload, &tx_packet, sizeof(tx_packet));
        
        ip_addr_t dongle_addr;
        ipaddr_aton(DONGLE_IP, &dongle_addr);
        udp_sendto(udp_pcb, p, &dongle_addr, KB_PORT);
        
        pbuf_free(p);
    }
}

int main() {
    // 1 blink = Starting
    if (cyw43_arch_init() == 0) {
        blink_pattern(1);
    }
    
    // 2 blinks = Connecting to WiFi
    blink_pattern(2);
    cyw43_arch_enable_sta_mode();
    
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASS,
                                           CYW43_AUTH_WPA2_AES_PSK, 15000)) {
        // Fast blinks = WiFi failed
        while(1) {
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
            sleep_ms(100);
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
            sleep_ms(100);
        }
    }
    
    // 3 blinks = WiFi connected
    blink_pattern(3);
    
    // Set static IP
    ip4_addr_t ip, mask, gw;
    ipaddr_aton(LEFT_IP, &ip);
    IP4_ADDR(&mask, 255, 255, 128, 0);
    ipaddr_aton(LEFT_IP, &gw);
    gw.addr = (gw.addr & 0xFFFFFF00) | 0x01;
    
    struct netif *netif = &cyw43_state.netif[CYW43_ITF_STA];
    dhcp_stop(netif);
    netif_set_addr(netif, &ip, &mask, &gw);
    
    sleep_ms(500);
    
    // Setup UDP
    udp_pcb = udp_new();
    if (udp_pcb != NULL) {
        udp_bind(udp_pcb, IP_ADDR_ANY, KB_PORT);
        connected = true;
        blink_pattern(4);
    }
    
#if !TEST_MODE
    // Initialize matrix scanning
    matrix_init();
#endif
    
    uint32_t last_blink = 0;
    bool blink_state = false;
    
#if TEST_MODE
    // Test mode - send 'A' every 2 seconds
    uint32_t last_keystroke = 0;
    bool keystroke_pressed = false;
#endif
    
    while (1) {
        uint32_t now = timer_read();
        
#if TEST_MODE
        // Test mode
        if (now - last_keystroke > 2000) {
            matrix_state_t matrix = {0};
            matrix.rows[1] = (1 << 1);
            matrix.changed_mask[1] = (1 << 1);
            send_matrix_update(&matrix);
            keystroke_pressed = true;
            last_keystroke = now;
        } else if (keystroke_pressed && now - last_keystroke > 100) {
            matrix_state_t matrix = {0};
            matrix.changed_mask[1] = (1 << 1);
            send_matrix_update(&matrix);
            keystroke_pressed = false;
        }
#else
        // Real matrix scanning
        matrix_scan();
        
        // Check for changes
        bool has_changes = false;
        matrix_state_t current_matrix = {0};
        
        for (uint8_t row = 0; row < MATRIX_ROWS; row++) {
            current_matrix.rows[row] = matrix_get_row(row);
            
            if (current_matrix.rows[row] != previous_matrix[row]) {
                current_matrix.changed_mask[row] = 
                    current_matrix.rows[row] ^ previous_matrix[row];
                has_changes = true;
            } else {
                current_matrix.changed_mask[row] = 0;
            }
        }
        
        // Send if anything changed
        if (has_changes) {
            send_matrix_update(&current_matrix);
            
            // Update previous state
            for (uint8_t row = 0; row < MATRIX_ROWS; row++) {
                previous_matrix[row] = current_matrix.rows[row];
            }
            
            // Quick flash on send
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
            sleep_ms(50);
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
        }
#endif
        
        // Slow blink = alive
        if (now - last_blink > 1000) {
            blink_state = !blink_state;
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, blink_state);
            last_blink = now;
        }
        
        cyw43_arch_poll();
        sleep_ms(1);
    }
    
    return 0;
}