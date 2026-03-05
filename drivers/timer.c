/* =============================================================================
 * MyOS - Timer Driver
 * ============================================================================= */

#include <stdint.h>

static uint32_t ticks = 0;

void timer_init(void) {
    /* Initialize PIT */
}

uint32_t timer_get_ticks(void) {
    return ticks;
}
