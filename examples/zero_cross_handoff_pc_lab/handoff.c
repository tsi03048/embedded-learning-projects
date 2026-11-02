#include <stdint.h>
#include <stdio.h>

/* A learning model: all times are virtual milliseconds. */
enum { MIN_MS = 3, MAX_MS = 7, REQUIRED_INTERVALS = 3, DEADLINE_MS = 50 };
typedef enum { COLLECTING, READY, FAILED } Status;
typedef struct {
    Status status;
    uint32_t last_ms;
    int have_last;
    unsigned good_intervals;
} Check;

static void begin(Check *c)
{
    c->status = COLLECTING;
    c->last_ms = 0;
    c->have_last = 0;
    c->good_intervals = 0;
}

/* A tick with no crossing still checks the overall startup deadline. */
static void tick(Check *c, uint32_t now)
{
    if (c->status == COLLECTING && now >= DEADLINE_MS) c->status = FAILED;
}

/* expected and observed phase identifiers are synthetic inputs, not GPIO. */
static void event(Check *c, uint32_t now, int expected, int observed)
{
    uint32_t interval;
    if (c->status != COLLECTING) return;
    tick(c, now); /* A crossing at the deadline is too late in this model. */
    if (c->status == FAILED) return;
    if (expected < 0 || expected > 2 || observed != expected ||
        (c->have_last && now <= c->last_ms)) {
        c->have_last = 0;
        c->good_intervals = 0;
        return;
    }
    if (!c->have_last) {
        c->last_ms = now;
        c->have_last = 1;
        return;
    }
    interval = now - c->last_ms;
    if (interval < MIN_MS || interval > MAX_MS) {
        c->good_intervals = 0;
    } else {
        ++c->good_intervals;
    }
    c->last_ms = now;
    if (c->good_intervals >= REQUIRED_INTERVALS) c->status = READY;
}

static unsigned total, failed;
static void expect(const char *label, int condition)
{
    ++total;
    if (!condition) ++failed;
    printf("%s: %s\n", label, condition ? "PASS" : "FAIL");
}

int main(void)
{
    Check c;
    begin(&c); event(&c, 4, 2, 2);
    expect("first event has no interval", c.status == COLLECTING && c.good_intervals == 0);
    event(&c, 8, 1, 1); event(&c, 12, 0, 0); event(&c, 16, 2, 2);
    expect("three good intervals", c.status == READY);
    tick(&c, 50);
    expect("ready stays ready", c.status == READY);
    begin(&c); event(&c, 1, 2, 2); event(&c, 4, 1, 1); event(&c, 11, 0, 0); event(&c, 14, 2, 2);
    expect("interval limits included", c.status == READY);
    begin(&c); event(&c, 1, 2, 2); event(&c, 5, 1, 1); event(&c, 9, 0, 2);
    expect("wrong phase resets", c.status == COLLECTING && !c.have_last && !c.good_intervals);
    begin(&c); event(&c, 1, 2, 2); event(&c, 5, 1, 1); event(&c, 13, 0, 0);
    expect("long gap resets streak", c.good_intervals == 0 && c.have_last);
    begin(&c); event(&c, 1, 2, 2); event(&c, 3, 1, 1);
    expect("short gap resets streak", c.good_intervals == 0);
    begin(&c); event(&c, 5, 2, 2); event(&c, 5, 1, 1);
    expect("duplicate time resets", !c.have_last && !c.good_intervals);
    begin(&c); event(&c, 5, 3, 3);
    expect("invalid phase resets", !c.have_last);
    begin(&c); tick(&c, 49); expect("before deadline", c.status == COLLECTING);
    tick(&c, 50); expect("no events timeout", c.status == FAILED);
    event(&c, 51, 2, 2); expect("failed stays failed", c.status == FAILED);
    begin(&c); event(&c, 38, 2, 2); event(&c, 42, 1, 1); event(&c, 46, 0, 0); event(&c, 50, 2, 2);
    expect("event at deadline fails", c.status == FAILED);
    printf("%u/%u passed\n", total - failed, total);
    return failed ? 1 : 0;
}
