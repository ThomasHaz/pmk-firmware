#!/bin/bash

# Build script for wireless split keyboard

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}Building Wireless Split Keyboard Firmware for Pico 2 W${NC}"

# Check if PICO_SDK_PATH is set
if [ -z "$PICO_SDK_PATH" ]; then
    echo -e "${YELLOW}Warning: PICO_SDK_PATH not set. Using default path.${NC}"
    export PICO_SDK_PATH=~/pico-sdk
fi


cp "$PICO_SDK_PATH/external/pico_sdk_import.cmake" .

# Apply static_assert patch to SDK
# Backup original file
cp "$PICO_SDK_PATH/src/rp2_common/hardware_dma/include/hardware/dma.h" "$PICO_SDK_PATH/src/rp2_common/hardware_dma/include/hardware/dma.h.bak"

# Apply the patch - this adds empty string messages to both static_assert calls
sed -i '' 's/static_assert(DMA_CH0_TRANS_COUNT_MODE_VALUE_ENDLESS == 0xf);/_Static_assert(DMA_CH0_TRANS_COUNT_MODE_VALUE_ENDLESS == 0xf, "");/' "$PICO_SDK_PATH/src/rp2_common/hardware_dma/include/hardware/dma.h"
sed -i '' 's/static_assert(DMA_CH0_TRANS_COUNT_MODE_LSB == 28);/_Static_assert(DMA_CH0_TRANS_COUNT_MODE_LSB == 28, "");/' "$PICO_SDK_PATH/src/rp2_common/hardware_dma/include/hardware/dma.h"


# Check SDK version for RP2350 support
if [ ! -f "$PICO_SDK_PATH/src/rp2350/hardware_regs/include/hardware/platform_defs.h" ]; then
    echo -e "${RED}Error: Your Pico SDK doesn't support RP2350 (Pico 2)${NC}"
    echo "Please update your SDK:"
    echo "  cd $PICO_SDK_PATH"
    echo "  git pull"
    echo "  git submodule update --init"
    exit 1
fi

# Clean build directories
rm -rf build/dongle build/left build/right

# Create build directories
mkdir -p build/dongle build/left build/right

# # Build dongle
echo -e "${GREEN}Building Dongle...${NC}"
cd build/dongle
cmake ../../dongle -DPICO_BOARD=pico2_w -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
cd ../..

# Build left half
echo -e "${GREEN}Building Left Half...${NC}"
cd build/left
cmake ../../left_half -DPICO_BOARD=pico2_w -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
cd ../..

# Build right half
echo -e "${GREEN}Building Right Half...${NC}"
cd build/right
cmake ../../right_half -DPICO_BOARD=pico2_w -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
cd ../..

# Restore original SDK file
rm "$PICO_SDK_PATH/src/rp2_common/hardware_dma/include/hardware/dma.h"
mv "$PICO_SDK_PATH/src/rp2_common/hardware_dma/include/hardware/dma.h.bak" "$PICO_SDK_PATH/src/rp2_common/hardware_dma/include/hardware/dma.h"

echo -e "${GREEN}Build complete!${NC}"
echo ""
echo "UF2 files created:"
echo "  Dongle:     build/dongle/keyboard_dongle.uf2"
echo "  Left Half:  build/left/keyboard_left.uf2"
echo "  Right Half: build/right/keyboard_right.uf2"
echo ""
echo "To flash Pico 2 W:"
echo "  1. Hold BOOTSEL button while connecting Pico 2 W"
echo "  2. Copy the appropriate UF2 file to the RP2350 drive"
echo "  3. The device will reboot automatically"