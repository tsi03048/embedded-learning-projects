#include "debounce.h"
#define DEBOUNCE_SAMPLES 20u
void debounce_init(Debounce *d, uint8_t raw)
{
    d->stable = raw ? 1u : 0u;
    d->count = 0u;
}
uint8_t debounce_step(Debounce *d, uint8_t raw)
{
    raw = raw ? 1u : 0u;
    if (raw == d->stable) {
        d->count = 0u;
        return 0u;
    }
    ++d->count;
    if (d->count < DEBOUNCE_SAMPLES) return 0u;
    d->stable = raw;
    d->count = 0u;
    return raw == 0u ? 1u : 0u;
}
