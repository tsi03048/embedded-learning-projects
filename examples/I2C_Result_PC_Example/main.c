#include <stdint.h>

typedef enum { READ_OK, READ_NACK, READ_TIMEOUT } ReadStatus;

typedef struct {
    ReadStatus status;
    uint8_t msb;
    uint8_t lsb;
} MockReply;

/* Simulated input only. No actual I2C communication is performed. */
static ReadStatus mock_read(const MockReply *reply, uint8_t bytes[2])
{
    if (reply->status != READ_OK) return reply->status;
    bytes[0] = reply->msb;
    bytes[1] = reply->lsb;
    return READ_OK;
}

static ReadStatus read_temperature(const MockReply *reply, float *temperature)
{
    uint8_t bytes[2];
    uint16_t combined;
    int32_t steps;
    ReadStatus status = mock_read(reply, bytes);
    if (status != READ_OK) return status;
    combined = ((uint16_t)bytes[0] << 8) | bytes[1];
    steps = (int32_t)(combined >> 4);
    if (steps >= 2048) steps -= 4096;
    *temperature = (float)steps * 0.0625f;
    return READ_OK;
}

/* 0 means all checks passed; nonzero identifies a failed check. */
int main(void)
{
    const MockReply zero = { READ_OK, 0x00, 0x00 };
    const MockReply positive = { READ_OK, 0x19, 0x00 };
    const MockReply negative = { READ_OK, 0xE7, 0x00 };
    const MockReply nack = { READ_NACK, 0x00, 0x00 };
    const MockReply timeout = { READ_TIMEOUT, 0x00, 0x00 };
    float value = 123.0f;
    if (read_temperature(&zero, &value) != READ_OK || value != 0.0f) return 1;
    if (read_temperature(&positive, &value) != READ_OK || value != 25.0f) return 2;
    if (read_temperature(&negative, &value) != READ_OK || value != -25.0f) return 3;
    value = 123.0f;
    if (read_temperature(&nack, &value) != READ_NACK || value != 123.0f) return 4;
    if (read_temperature(&timeout, &value) != READ_TIMEOUT || value != 123.0f) return 5;
    return 0;
}
