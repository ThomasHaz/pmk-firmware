#include <string.h>
#include "tusb.h"
#include "pico/unique_id.h"

#define _PID_MAP(itf, n)  ( (CFG_TUD_##itf) << (n) )
#define USB_PID           (0x4000 | _PID_MAP(CDC, 0) | _PID_MAP(MSC, 1) | _PID_MAP(HID, 2) | \
                           _PID_MAP(MIDI, 3) | _PID_MAP(VENDOR, 4) )

//--------------------------------------------------------------------+
// Device Descriptors
//--------------------------------------------------------------------+
tusb_desc_device_t const desc_device =
{
    .bLength            = sizeof(tusb_desc_device_t),
    .bDescriptorType    = TUSB_DESC_DEVICE,
    .bcdUSB             = 0x0200,
    .bDeviceClass       = 0x00,
    .bDeviceSubClass    = 0x00,
    .bDeviceProtocol    = 0x00,
    .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,

    .idVendor           = 0xCafe,
    .idProduct          = USB_PID,
    .bcdDevice          = 0x0100,

    .iManufacturer      = 0x01,
    .iProduct           = 0x02,
    .iSerialNumber      = 0x03,

    .bNumConfigurations = 0x01
};

uint8_t const * tud_descriptor_device_cb(void)
{
    return (uint8_t const *) &desc_device;
}

//--------------------------------------------------------------------+
// HID Report Descriptors
//--------------------------------------------------------------------+

// Keyboard Report Descriptor
uint8_t const desc_hid_keyboard_report[] =
{
    TUD_HID_REPORT_DESC_KEYBOARD(HID_REPORT_ID(1))
};

// Mouse Report Descriptor with extended buttons
uint8_t const desc_hid_mouse_report[] =
{
    TUD_HID_REPORT_DESC_MOUSE(HID_REPORT_ID(2))
};

// Combined report descriptor
uint8_t const desc_hid_report[] =
{
    TUD_HID_REPORT_DESC_KEYBOARD(HID_REPORT_ID(1)),
    TUD_HID_REPORT_DESC_MOUSE(HID_REPORT_ID(2))
};

uint8_t const * tud_hid_descriptor_report_cb(uint8_t instance)
{
    (void) instance;
    return desc_hid_report;
}

//--------------------------------------------------------------------+
// Configuration Descriptor
//--------------------------------------------------------------------+

enum
{
    ITF_NUM_HID,
    ITF_NUM_TOTAL
};

#define CONFIG_TOTAL_LEN  (TUD_CONFIG_DESC_LEN + TUD_HID_DESC_LEN)

#define EPNUM_HID   0x81

uint8_t const desc_configuration[] =
{
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN, TUSB_DESC_CONFIG_ATT_REMOTE_WAKEUP, 100),
    TUD_HID_DESCRIPTOR(ITF_NUM_HID, 0, HID_ITF_PROTOCOL_NONE, sizeof(desc_hid_report), EPNUM_HID, CFG_TUD_HID_EP_BUFSIZE, 5)
};

uint8_t const * tud_descriptor_configuration_cb(uint8_t index)
{
    (void) index;
    return desc_configuration;
}

//--------------------------------------------------------------------+
// String Descriptors
//--------------------------------------------------------------------+

char const* string_desc_arr [] =
{
    (const char[]) { 0x09, 0x04 },
    "Pico",
    "Pico Wireless Keyboard + Mouse",
    NULL,
};

static uint16_t _desc_str[32];

uint16_t const* tud_descriptor_string_cb(uint8_t index, uint16_t langid)
{
    (void) langid;
    uint8_t chr_count;

    if (index == 0)
    {
        memcpy(&_desc_str[1], string_desc_arr[0], 2);
        chr_count = 1;
    }
    else
    {
        if (index == 3)
        {
            pico_unique_board_id_t board_id;
            pico_get_unique_board_id(&board_id);
            
            chr_count = 0;
            for (int i = 0; i < PICO_UNIQUE_BOARD_ID_SIZE_BYTES; i++)
            {
                chr_count += sprintf((char*)&_desc_str[1 + chr_count], "%02x", board_id.id[i]);
                if (chr_count >= 30) break;
            }
        }
        else if (index >= sizeof(string_desc_arr) / sizeof(string_desc_arr[0]))
        {
            return NULL;
        }
        else
        {
            const char* str = string_desc_arr[index];
            chr_count = strlen(str);
            if (chr_count > 31) chr_count = 31;

            for (uint8_t i = 0; i < chr_count; i++)
            {
                _desc_str[1 + i] = str[i];
            }
        }
    }

    _desc_str[0] = (TUSB_DESC_STRING << 8 ) | (2 * chr_count + 2);
    return _desc_str;
}