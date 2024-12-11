#include "joypad.h"

void retro_core_input_poll(void) {}

/**
 * Queries for input for player 'port'.
 *
 * @param port Which player 'port' to query.
 * @param device Which device type to query for.
 * @param index The input index to retrieve. (?)
 * @param id The ID of which value to query, like \c RETRO_DEVICE_ID_JOYPAD_B.
 * @returns Depends on the provided arguments, but will return 0 if their values are unsupported 
 * by the frontend or the backing physical device. Also, in general it will return 0 (RELEASED) or 1 (PRESSED).
 */
int16_t retro_core_input_state(unsigned port, unsigned device, unsigned index, unsigned id) {
    return core_handler.joypads[port][id];
}
