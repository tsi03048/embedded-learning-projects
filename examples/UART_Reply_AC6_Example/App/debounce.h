#ifndef DEBOUNCE_H
#define DEBOUNCE_H
#include <stdint.h>
typedef struct { uint8_t stable; uint8_t count; } Debounce;
void debounce_init(Debounce *d, uint8_t raw);
/* Call exactly once per 1-ms sample. Returns 1 only on a confirmed press. */
uint8_t debounce_step(Debounce *d, uint8_t raw);
#endif
