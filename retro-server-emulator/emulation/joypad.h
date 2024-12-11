#ifndef EMULATION_JOYPAD_H
#define EMULATION_JOYPAD_H

#include <stdint.h>

#include "core.h"
#include "../utils.h"

void retro_core_input_poll(void);
int16_t retro_core_input_state(unsigned port, unsigned device, unsigned index, unsigned id);

#endif