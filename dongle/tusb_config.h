#ifndef _TUSB_CONFIG_H_
#define _TUSB_CONFIG_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BOARD_TUD_RHPORT
#define BOARD_TUD_RHPORT      0
#endif

#ifndef BOARD_TUD_MAX_SPEED
#define BOARD_TUD_MAX_SPEED   OPT_MODE_DEFAULT_SPEED
#endif

#ifndef CFG_TUSB_MCU
#define CFG_TUSB_MCU          OPT_MCU_RP2040
#endif

#ifndef CFG_TUSB_OS
#define CFG_TUSB_OS           OPT_OS_PICO
#endif

#ifndef CFG_TUSB_DEBUG
#define CFG_TUSB_DEBUG        0
#endif

#define CFG_TUD_ENABLED       1
#define CFG_TUD_MAX_SPEED     BOARD_TUD_MAX_SPEED

#ifndef CFG_TUD_ENDPOINT0_SIZE
#define CFG_TUD_ENDPOINT0_SIZE    64
#endif

//------------- CLASS -------------//
#define CFG_TUD_HID               1
#define CFG_TUD_CDC               1  // Enable CDC for USB serial
#define CFG_TUD_MSC               0
#define CFG_TUD_MIDI              0
#define CFG_TUD_VENDOR            0

// HID buffer size
#define CFG_TUD_HID_EP_BUFSIZE    16

// CDC buffer sizes
#define CFG_TUD_CDC_RX_BUFSIZE    256
#define CFG_TUD_CDC_TX_BUFSIZE    256

#ifndef CFG_TUSB_MEM_SECTION
#define CFG_TUSB_MEM_SECTION
#endif

#ifndef CFG_TUSB_MEM_ALIGN
#define CFG_TUSB_MEM_ALIGN       __attribute__ ((aligned(4)))
#endif

#define CFG_TUSB_RHPORT0_MODE     OPT_MODE_DEVICE

#ifdef __cplusplus
}
#endif

#endif /* _TUSB_CONFIG_H_ */