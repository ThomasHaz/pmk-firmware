#ifndef MODTAP_H
#define MODTAP_H

#include <stdint.h>
#include <stdbool.h>
#include "config.h"

bool process_modtap(uint16_t keycode, bool pressed, uint8_t row, uint8_t col);
void modtap_task(void);
void modtap_interrupt(void);

#endif // MODTAP_H
