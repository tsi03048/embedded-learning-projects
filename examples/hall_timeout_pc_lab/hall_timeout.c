#include <stdint.h>
#include <stdio.h>

/* Educational example: valid order 110,100,101,001,011,010. */
typedef enum { STOPPED, WAITING, ACTIVE, FAULT } Status;
typedef struct {
    Status status;
    int last_hall;
    uint32_t last_change_ms;
} Monitor;

static int position(int hall)
{
    static const int order[] = {6, 4, 5, 1, 3, 2};
    int i;
    for (i = 0; i < 6; ++i) if (order[i] == hall) return i;
    return -1;
}

/* Calling start explicitly clears the previous fault in this model. */
static void start(Monitor *m, int hall, uint32_t now)
{
    m->last_hall = hall;
    m->last_change_ms = now;
    m->status = position(hall) < 0 ? FAULT : WAITING;
}

static void stop(Monitor *m) { m->status = STOPPED; }

static void sample(Monitor *m, int hall, uint32_t now, uint32_t timeout)
{
    int before, after;
    if (m->status == STOPPED || m->status == FAULT) return;
    before = position(m->last_hall);
    after = position(hall);
    if (after < 0) { m->status = FAULT; return; }
    /* Check the deadline before accepting a late transition. */
    if ((uint32_t)(now - m->last_change_ms) >= timeout) {
        m->status = FAULT;
        return;
    }
    if (before == after) return;
    if ((before + 1) % 6 != after && (before + 5) % 6 != after) {
        m->status = FAULT;
        return;
    }
    m->last_hall = hall;
    m->last_change_ms = now;
    m->status = ACTIVE;
}

static unsigned checks, failures;
static void check(const char *label, Status actual, Status expected)
{
    ++checks;
    if (actual != expected) ++failures;
    printf("%s: %s\n", label, actual == expected ? "PASS" : "FAIL");
}

int main(void)
{
    Monitor m = {STOPPED, 0, 0};
    const uint32_t limit = 100; /* milliseconds; for this exercise only */
    sample(&m, 0, 1000, limit);
    check("stopped", m.status, STOPPED);
    start(&m, 6, 0);
    check("start", m.status, WAITING);
    sample(&m, 6, 99, limit);
    check("unchanged before limit", m.status, WAITING);
    sample(&m, 6, 100, limit);
    check("exactly at limit", m.status, FAULT);
    sample(&m, 4, 101, limit);
    check("fault remains", m.status, FAULT);
    start(&m, 6, 200);
    sample(&m, 4, 250, limit);
    check("forward transition", m.status, ACTIVE);
    sample(&m, 4, 349, limit);
    check("timer resets on transition", m.status, ACTIVE);
    sample(&m, 4, 350, limit);
    check("active timeout", m.status, FAULT);
    start(&m, 6, 400);
    sample(&m, 2, 401, limit);
    check("reverse transition", m.status, ACTIVE);
    start(&m, 6, 500);
    sample(&m, 5, 501, limit);
    check("skipped transition", m.status, FAULT);
    start(&m, 0, 600);
    check("invalid start", m.status, FAULT);
    start(&m, 6, 700);
    sample(&m, 7, 701, limit);
    check("invalid sample", m.status, FAULT);
    start(&m, 6, 800);
    sample(&m, 4, 900, limit);
    check("late transition", m.status, FAULT);
    start(&m, 6, UINT32_MAX - 20u);
    sample(&m, 4, 10, limit);
    check("clock wrap", m.status, ACTIVE);
    stop(&m);
    sample(&m, 0, 2000, limit);
    check("stop clears monitoring", m.status, STOPPED);
    printf("%u/%u passed\n", checks - failures, checks);
    return failures ? 1 : 0;
}
