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
    }
}

int main() {
    stdio_init_all();
    sleep_ms(1000);
    
    printf("\n=== Wireless Keyboard Dongle ===\n");
    
    // USB first
    printf("1. USB init...\n");
    board_init();
    tusb_init();
    
    uint32_t start = to_ms_since_boot(get_absolute_time());
    while (!tud_mounted() && (to_ms_since_boot(get_absolute_time()) - start) < 5000) {
        tud_task();
        sleep_ms(10);
    }
    printf("   %s\n", tud_mounted() ? "OK" : "TIMEOUT");
    
    // WiFi chip only (no connection yet)
    printf("2. WiFi chip init...\n");
    if (cyw43_arch_init()) {
        printf("   FATAL\n");
        while(1) sleep_ms(1000);
    }
    printf("   OK\n");
    
    // Features
    printf("3. Features init...\n");
    init_features();
    init_combos();
    mouse_init();
    autoclicker_init();
    printf("   OK\n");
    
    printf("\n=== Starting main loop (will connect WiFi after 5 seconds) ===\n\n");
    
    bool wifi_connect_attempted = false;
    uint32_t loop_start = timer_read();
    uint32_t last_blink = 0;
    bool blink_state = false;
    
    // Main loop
    // Main loop
    uint32_t last_status_check = 0;
    //uint32_t last_blink = 0;
    uint32_t blink_count = 0;
    
    while (1) {
        tud_task();
        cyw43_arch_poll();
        
        uint32_t now = timer_read();
        
        // Connect WiFi after 5 seconds (USB is stable by then)
        if (!wifi_connect_attempted && (now - loop_start) > 5000) {
            wifi_connect_attempted = true;
            
            // 5 quick blinks = trying WiFi
            for (int i = 0; i < 5; i++) {
                cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
                sleep_ms(100);
                cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
                sleep_ms(100);
            }
            
            cyw43_arch_enable_sta_mode();
            
            // Connect (ignore return value)
            cyw43_arch_wifi_connect_async(WIFI_SSID, WIFI_PASS, CYW43_AUTH_WPA2_AES_PSK);
            
            // Wait for connection to establish
            sleep_ms(5000);
            
            // Just assume it worked and set static IP
            ip4_addr_t ip, mask, gw;
            ipaddr_aton(DONGLE_IP, &ip);
            IP4_ADDR(&mask, 255, 255, 128, 0);
            ipaddr_aton(DONGLE_IP, &gw);
            gw.addr = (gw.addr & 0xFFFFFF00) | 0x01;
            
            struct netif *netif = &cyw43_state.netif[CYW43_ITF_STA];
            dhcp_stop(netif);
            netif_set_addr(netif, &ip, &mask, &gw);
            
            // 3 long blinks = WiFi setup complete
            for (int i = 0; i < 3; i++) {
                cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
                sleep_ms(500);
                cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
                sleep_ms(500);
            }
            
            sleep_ms(500);
            
            // Setup UDP
            udp_pcb = udp_new();
            if (udp_pcb != NULL && udp_bind(udp_pcb, IP_ADDR_ANY, KB_PORT) == ERR_OK) {
                udp_recv(udp_pcb, udp_recv_callback, NULL);
                wifi_ready = true;
                
                // 1 very long blink = UDP ready
                cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
                sleep_ms(2000);
                cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
            } else {
                // 10 fast blinks = UDP failed
                for (int i = 0; i < 10; i++) {
                    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
                    sleep_ms(100);
                    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
                    sleep_ms(100);
                }
            }
        }
        
        send_hid_report();
        mouse_task();
        autoclicker_task();
        modtap_task();
        tap_dance_task();
        oneshot_task();
        
        // LED status indicator every 3 seconds
        if (now - last_status_check > 3000) {
            last_status_check = now;
            
            // Check how recent we've seen left half
            uint32_t left_age = now - state.left_last_seen;
            
            if (wifi_ready && left_age < 2000) {
                // Connected and receiving! 2 quick blinks
                for (int i = 0; i < 2; i++) {
                    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
                    sleep_ms(100);
                    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
                    sleep_ms(100);
                }
            } else if (wifi_ready) {
                // WiFi ready but not receiving - 1 long blink
                cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
                sleep_ms(500);
                cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
            } else {
                // No WiFi - stay off
            }
        }
        
        sleep_ms(1);
    }
    
    return 0;
}