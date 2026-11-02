#include <stdint.h>

/* Board adaptation: all functions must return promptly. */
extern uint32_t port_now_us(void);
extern void port_cs_low(void);
extern void port_cs_high(void);
extern uint32_t port_spi_status(void);
extern void port_spi_write(uint8_t value);
extern uint8_t port_spi_read(void);

#define F_RXNE (1u << 0)
#define F_TXE  (1u << 1)
#define F_MODF (1u << 5)
#define F_OVR  (1u << 6)
#define F_BSY  (1u << 7)
/* Demonstration values only; validate against actual device/timer. */
#define SETUP_US 10u
#define HOLD_US  10u
#define TIMEOUT_US 100000u

enum { ST_IDLE, ST_SETUP, ST_TXE, ST_RXNE, ST_END, ST_HOLD, ST_DONE, ST_FAULT };
enum { ERR_NONE, ERR_SPI, ERR_TIMEOUT };
uint8_t state = ST_IDLE;
uint8_t error_code = ERR_NONE, failed_state = ST_IDLE;
uint8_t result_valid = 0u, id_match = 0u;
uint8_t id[3];
static uint8_t temp_id[3], byte_index;
static uint32_t entered;

static void enter(uint8_t next)
{
    entered = port_now_us();
    state = next;
}

uint8_t id_start(void)
{
    if (state != ST_IDLE) return 0u;
    result_valid = 0u;
    id_match = 0u;
    error_code = ERR_NONE;
    byte_index = 0u;
    port_cs_low();
    enter(ST_SETUP);
    return 1u;
}

void id_task(void)
{
    uint32_t sr, elapsed;
    uint8_t value;
    if ((state == ST_IDLE) || (state == ST_DONE) || (state == ST_FAULT)) return;
    sr = port_spi_status();
    if ((sr & (F_MODF | F_OVR)) != 0u) {
        failed_state = state;
        error_code = ERR_SPI;
        result_valid = 0u;
        state = ST_FAULT;
        return;
    }
    elapsed = (uint32_t)(port_now_us() - entered);
    if (((state == ST_TXE) || (state == ST_RXNE) || (state == ST_END)) &&
        (elapsed >= TIMEOUT_US)) {
        failed_state = state;
        error_code = ERR_TIMEOUT;
        result_valid = 0u;
        state = ST_FAULT;
        return;
    }
    if (state == ST_SETUP) {
        if (elapsed >= SETUP_US) enter(ST_TXE);
    } else if (state == ST_TXE) {
        if ((sr & F_TXE) != 0u) {
            if (byte_index == 0u) port_spi_write(0x9Fu);
            else port_spi_write(0x00u);
            enter(ST_RXNE);
        }
    } else if (state == ST_RXNE) {
        if ((sr & F_RXNE) != 0u) {
            value = port_spi_read();
            if (byte_index != 0u) temp_id[byte_index - 1u] = value;
            byte_index++;
            if (byte_index < 4u) enter(ST_TXE);
            else enter(ST_END);
        }
    } else if (state == ST_END) {
        if (((sr & F_TXE) != 0u) && ((sr & F_BSY) == 0u)) enter(ST_HOLD);
    } else if (state == ST_HOLD) {
        if (elapsed >= HOLD_US) {
            port_cs_high();
            id[0] = temp_id[0]; id[1] = temp_id[1]; id[2] = temp_id[2];
            if ((id[0] == 0x1Fu) && (id[1] == 0x24u) && (id[2] == 0x00u)) id_match = 1u;
            result_valid = 1u;
            state = ST_DONE;
        }
    }
}
