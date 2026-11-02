#include <stdint.h>
#include <stdio.h>

typedef enum { WAITING, FRESH, STALE } SampleState;
typedef struct {
    SampleState state;
    uint32_t last_tick;
    uint16_t last_raw;
} SampleMonitor;

/* Times are simulated unsigned 32-bit ticks, not hardware milliseconds. */
#define LIMIT 500u
static void begin(SampleMonitor *m, uint32_t now)
{
    m->state = WAITING;
    m->last_tick = now;
    m->last_raw = 0u;
}
/* Single owner; event indicates a NEW successful conversion observed now. */
static void step(SampleMonitor *m, uint32_t now, int new_sample, uint16_t raw)
{
    if (new_sample) {
        m->last_raw = raw;
        m->last_tick = now;
        m->state = FRESH;
    } else if ((uint32_t)(now - m->last_tick) >= LIMIT) {
        m->state = STALE;
    }
}
static int check(int ok, const char *name)
{
    printf("%s: %s\n", ok ? "PASS" : "FAIL", name);
    return ok ? 0 : 1;
}
int main(void)
{
    SampleMonitor m;
    int errors = 0;
    begin(&m, 0u);
    step(&m, 499u, 0, 0u);
    errors += check(m.state == WAITING, "waiting before deadline");
    step(&m, 500u, 0, 0u);
    errors += check(m.state == STALE, "first sample timeout");
    step(&m, 501u, 1, 0u);
    errors += check(m.state == FRESH && m.last_raw == 0u, "zero is valid on recovery");
    step(&m, 1000u, 0, 0u);
    errors += check(m.state == FRESH, "fresh before deadline");
    step(&m, 1001u, 0, 0u);
    errors += check(m.state == STALE, "stale at deadline");
    step(&m, 1100u, 1, 2048u);
    errors += check(m.state == FRESH && m.last_raw == 2048u, "new sample restores fresh");
    step(&m, 1600u, 1, 2049u);
    errors += check(m.state == FRESH && m.last_raw == 2049u, "sample wins at deadline");
    begin(&m, UINT32_MAX - 200u);
    step(&m, 298u, 0, 0u);
    errors += check(m.state == WAITING, "tick wrap before deadline");
    step(&m, 299u, 0, 0u);
    errors += check(m.state == STALE, "tick wrap at deadline");
    printf("Result: %d failure(s)\n", errors);
    return errors ? 1 : 0;
}
