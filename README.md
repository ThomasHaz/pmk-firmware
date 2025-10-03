# Wireless Split Keyboard Firmware

A feature-rich wireless split keyboard firmware for Raspberry Pi Pico 2 W devices, utilising a dongle device, plus a device for each keyboard half.

## Hardware Requirements

- 3x Raspberry Pi Pico 2 W boards
- Key switches and diodes for matrix
- USB cable for dongle connection

## Features

- **Wireless Communication**: WiFi-based, low-latency protocol
- **Advanced Key Features**:
  - Mod-Tap (dual-function keys)
  - Layer system with multiple modes
  - One-shot layers
  - Tap dance
  - Key combos
  - Mousekeys
  - Autoclicking functionality
- **Split Keyboard Support**: True wireless split design
- **USB HID**: Standard keyboard interface
- **Debouncing**: Hardware-level key debouncing

## Pin Configuration

Default pin configuration (adjustable in `config.h`):
- **Row pins**: GP0, GP1, GP2, GP3
- **Column pins**: GP4, GP5, GP6, GP7, GP8, GP9

## Building

### Prerequisites

1. Install the Pico SDK:
```bash
git clone https://github.com/raspberrypi/pico-sdk.git
cd pico-sdk
git submodule update --init
export PICO_SDK_PATH=/path/to/pico-sdk
```

2. Install build tools:
```bash
sudo apt install cmake gcc-arm-none-eabi build-essential
```

### Compilation

```bash
chmod +x build.sh
./build.sh
```

This will create three UF2 files in the build directory.

## Flashing

1. Hold the BOOTSEL button while connecting each Pico W
2. Copy the appropriate UF2 file to the RPI-RP2 drive:
   - `keyboard_dongle.uf2` → Dongle Pico W
   - `keyboard_left.uf2` → Left half Pico W
   - `keyboard_right.uf2` → Right half Pico W

## Configuration

### Network Settings

Edit `common/config.h`:
```c
#define WIFI_SSID "KB_SPLIT_NET"
#define WIFI_PASS "securekeyboard123"
```

### Keymap

Edit `keymaps/default/keymap.c` to customize your layout.

### Features

Adjust timing in `common/config.h`:
```c
#define TAPPING_TERM 200      // Mod-tap timing
#define ONESHOT_TIMEOUT 3000  // One-shot layer timeout
#define COMBO_TERM 30         // Combo detection window
```

## Architecture

```
┌─────────────┐         ┌─────────────┐         ┌─────────────┐
│  Left Half  │ WiFi    │   Dongle    │    WiFi │ Right Half  │
│   (Pico W)  ├────────►│   (Pico W)  │◄────────┤  (Pico W)   │
│             │         │      ↓      │         │             │
│ Matrix Scan │         │   USB HID   │         │ Matrix Scan │
└─────────────┘         └──────┬──────┘         └─────────────┘
                               │
                               ▼
                            Computer
```

## Troubleshooting

### Connection Issues
- Ensure all devices are powered
- Check WiFi credentials match
- Verify IP addresses in config

### Key Registration Issues
- Check matrix wiring
- Verify pin assignments
- Test with debug output enabled

### Build Issues
- Ensure PICO_SDK_PATH is set correctly
- Update submodules: `git submodule update --init`
- Check CMake version (3.13+ required)

## License

GPL License - See LICENSE file for details

## Contributing

Pull requests welcome! Please follow the existing code style.