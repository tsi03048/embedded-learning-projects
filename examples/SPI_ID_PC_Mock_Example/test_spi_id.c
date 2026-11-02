#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/* Compile this file only: it includes the exact lesson implementation. */
#include "spi_id_state.c"

static uint32_t mock_time, mock_status;
static uint8_t mock_cs, sent[8], incoming[4], send_count, read_count;
uint32_t port_now_us(void) { return mock_time; }
void port_cs_low(void) { mock_cs = 0u; }
void port_cs_high(void) { mock_cs = 1u; }
uint32_t port_spi_status(void) { return mock_status; }
void port_spi_write(uint8_t v) { if (send_count < 8u) sent[send_count++] = v; }
uint8_t port_spi_read(void) { return incoming[read_count++]; }

#define CHECK(x) do { if (!(x)) { printf("FAIL line %d: %s\n", __LINE__, #x); exit(1); } } while (0)

static void reset_fixture(void)
{
    unsigned i;
    mock_time = 0u; mock_status = 0u; mock_cs = 1u;
    send_count = 0u; read_count = 0u;
    state = ST_IDLE; error_code = ERR_NONE; failed_state = ST_IDLE;
    result_valid = 0u; id_match = 0u; byte_index = 0u; entered = 0u;
    for (i = 0; i < 3u; ++i) { id[i] = 0u; temp_id[i] = 0u; }
    incoming[0] = 0xEEu; incoming[1] = 0x1Fu;
    incoming[2] = 0x24u; incoming[3] = 0x00u;
}

static void start_and_enter_txe(void)
{
    CHECK(id_start() == 1u); CHECK(mock_cs == 0u); CHECK(result_valid == 0u);
    mock_time += SETUP_US; id_task(); CHECK(state == ST_TXE);
}

static void exchange_one(void)
{
    mock_status = F_TXE; id_task(); CHECK(state == ST_RXNE);
    mock_status = F_RXNE; id_task();
}

static void finish_transaction(void)
{
    unsigned i;
    start_and_enter_txe();
    for (i = 0; i < 4u; ++i) exchange_one();
    CHECK(state == ST_END); CHECK(result_valid == 0u); CHECK(mock_cs == 0u);
    mock_status = F_TXE; id_task(); CHECK(state == ST_HOLD);
    mock_time += HOLD_US; id_task();
}

int main(void)
{
    reset_fixture(); finish_transaction();
    CHECK(state == ST_DONE && mock_cs == 1u && result_valid == 1u && id_match == 1u);
    CHECK(send_count == 4u && read_count == 4u);
    CHECK(sent[0] == 0x9Fu && sent[1] == 0u && sent[2] == 0u && sent[3] == 0u);
    CHECK(id[0] == 0x1Fu && id[1] == 0x24u && id[2] == 0u);
    puts("PASS normal ID");

    reset_fixture(); incoming[1] = 0xAAu; finish_transaction();
    CHECK(state == ST_DONE && result_valid == 1u && id_match == 0u && id[0] == 0xAAu);
    puts("PASS completed but ID mismatch");

    reset_fixture(); start_and_enter_txe();
    mock_status = F_TXE; id_task(); CHECK(state == ST_RXNE);
    mock_status = 0u; mock_time += TIMEOUT_US; id_task();
    CHECK(state == ST_FAULT && error_code == ERR_TIMEOUT && failed_state == ST_RXNE);
    CHECK(result_valid == 0u && mock_cs == 0u);
    puts("PASS RX timeout: result invalid, CS remains low");

    reset_fixture(); start_and_enter_txe(); mock_status = F_OVR; id_task();
    CHECK(state == ST_FAULT && error_code == ERR_SPI && result_valid == 0u && mock_cs == 0u);
    puts("PASS injected SPI error");

    reset_fixture(); mock_time = UINT32_MAX - 4u;
    CHECK(id_start() == 1u); mock_time += SETUP_US; id_task(); CHECK(state == ST_TXE);
    puts("PASS timer wrap");
    return 0;
}
