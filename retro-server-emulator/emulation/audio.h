#ifndef EMULATION_AUDIO_H
#define EMULATION_AUDIO_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "core.h"
#include "../communication/commands.h"

void retro_core_audio_sample(int16_t left, int16_t right);
size_t retro_core_audio_sample_batch(const int16_t *data, size_t frames);

#endif