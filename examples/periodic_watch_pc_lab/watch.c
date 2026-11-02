#include <stdio.h>

/* Monotonic virtual milliseconds; no clock wrap in this teaching model. */
enum { LIMIT_MS = 20 };
typedef enum { WAITING, FRESH, TIMEOUT } Status;
typedef struct {
    Status status;
    unsigned start_ms, last_ms;
    int have_sample;
} Monitor;

static void begin(Monitor *m, unsigned now)
{
    m->status = WAITING;
    m->start_ms = now;
    m->last_ms = 0;
    m->have_sample = 0;
}

/* Call independently even if no sample has arrived. */
static void tick(Monitor *m, unsigned now)
{
    unsigned reference;
    if (m->status == TIMEOUT) return;
    reference = m->have_sample ? m->last_ms : m->start_ms;
    if (now - reference >= LIMIT_MS) m->status = TIMEOUT;
}

/* Each new virtual measurement has a unique increasing timestamp. */
static void receive(Monitor *m, unsigned now, unsigned measured, int valid)
{
    tick(m, now); /* A late record does not automatically clear timeout. */
    if (m->status == TIMEOUT || !valid || measured > now || measured < m->start_ms)
        return;
    if (now - measured >= LIMIT_MS) return;
    if (m->have_sample && measured <= m->last_ms) return;
    m->last_ms = measured;
    m->have_sample = 1;
    m->status = FRESH;
}

static unsigned total, failures;
static void check(const char *name, int ok)
{
    ++total;
    if (!ok) ++failures;
    printf("%s: %s\n", name, ok ? "PASS" : "FAIL");
}

int main(void)
{
    Monitor m;
    begin(&m, 90); tick(&m, 109);
    check("first sample still waiting", m.status == WAITING && !m.have_sample);
    tick(&m, 110); check("first sample timeout", m.status == TIMEOUT);
    receive(&m, 111, 111, 1); check("late sample keeps timeout", m.status == TIMEOUT);

    begin(&m, 90); receive(&m, 100, 100, 1);
    check("first valid sample", m.status == FRESH && m.last_ms == 100);
    tick(&m, 115); check("before limit", m.status == FRESH);
    tick(&m, 125); check("late polling detects timeout", m.status == TIMEOUT);

    begin(&m, 90); receive(&m, 100, 100, 1);
    receive(&m, 110, 100, 1); check("duplicate does not refresh", m.last_ms == 100);
    tick(&m, 120); check("exact limit timeout", m.status == TIMEOUT);

    begin(&m, 90); receive(&m, 100, 100, 1);
    receive(&m, 110, 110, 0); check("invalid does not refresh", m.last_ms == 100);
    receive(&m, 115, 115, 1); check("new sample refreshes", m.status == FRESH && m.last_ms == 115);
    tick(&m, 134); check("new deadline not reached", m.status == FRESH);
    tick(&m, 135); check("new deadline reached", m.status == TIMEOUT);

    begin(&m, 90); receive(&m, 95, 96, 1);
    check("future sample ignored", m.status == WAITING && !m.have_sample);
    printf("%u/%u passed\n", total - failures, total);
    return failures ? 1 : 0;
}
