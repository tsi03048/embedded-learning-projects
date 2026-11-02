#include <stdint.h>

typedef enum { BUS_OK, BUS_NACK, BUS_TIMEOUT } BusStatus;
typedef BusStatus (*ReadTwoBytes)(uint8_t address, uint8_t reg, uint8_t out[2]);

/* Caller may use *out_c only when BUS_OK is returned. */
static BusStatus read_temperature(ReadTwoBytes read_bus, float *out_c)
{
    uint8_t raw[2];
    uint16_t combined;
    int32_t steps;
    BusStatus status = read_bus(0x48u, 0x00u, raw);
    if (status != BUS_OK) return status;
    combined = ((uint16_t)raw[0] << 8) | raw[1];
    steps = (int32_t)(combined >> 4);
    if (steps >= 2048) steps -= 4096;
    *out_c = (float)steps * 0.0625f;
    return BUS_OK;
}

/* These mock functions do not communicate with hardware. */
static BusStatus mock_25c(uint8_t address, uint8_t reg, uint8_t out[2])
{
    if (address != 0x48u || reg != 0x00u) return BUS_NACK;
    out[0] = 0x19u;
    out[1] = 0x00u;
    return BUS_OK;
}

static BusStatus mock_timeout(uint8_t address, uint8_t reg, uint8_t out[2])
{
    (void)address; (void)reg; (void)out;
    return BUS_TIMEOUT;
}

/* Standalone C test: 0 = expected results; nonzero = check failed. */
int main(void)
{
    float value = 123.0f;
    if (read_temperature(mock_25c, &value) != BUS_OK || value != 25.0f) return 1;
    value = 123.0f;
    if (read_temperature(mock_timeout, &value) != BUS_TIMEOUT || value != 123.0f) return 2;
    return 0;
}
