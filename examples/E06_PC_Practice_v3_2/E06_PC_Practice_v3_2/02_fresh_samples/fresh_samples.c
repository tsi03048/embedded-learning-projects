#include <stdio.h>

/* Toy model: monotonic virtual milliseconds; no clock wrap within the test. */
typedef struct { int value; unsigned measured_ms; int valid; } Sample;
typedef struct { int armed; int have_last; unsigned last_ms; } Detector;
typedef enum { REJECTED, DUPLICATE, ACCEPTED, CROSSING } Result;

static void init(Detector *d)
{
    d->armed = 0;
    d->have_last = 0;
    d->last_ms = 0;
}

static Result process(Detector *d, unsigned now, Sample s)
{
    /* Reject invalid, future, or older-than-20-ms records; break continuity. */
    if (!s.valid || s.measured_ms > now || now - s.measured_ms > 20) {
        d->armed = 0;
        return REJECTED;
    }
    /* In this toy example each new measurement has a distinct, increasing time. */
    if (d->have_last && s.measured_ms == d->last_ms) return DUPLICATE;
    if (d->have_last && s.measured_ms < d->last_ms) {
        d->armed = 0;
        return REJECTED;
    }
    d->have_last = 1;
    d->last_ms = s.measured_ms;
    if (s.value <= -2) d->armed = 1;
    if (d->armed && s.value >= 2) {
        d->armed = 0;
        return CROSSING;
    }
    return ACCEPTED;
}

static unsigned total, failures;
static void check(const char *name, Result got, Result expected)
{
    int ok = got == expected;
    ++total;
    if (!ok) ++failures;
    printf("%s: %s (got %d, expected %d)\n", name, ok ? "PASS" : "FAIL", (int)got, (int)expected);
}

int main(void)
{
    Detector d;
    init(&d);
    check("fresh negative", process(&d, 100, (Sample){-3, 100, 1}), ACCEPTED);
    check("stale positive", process(&d, 140, (Sample){3, 110, 1}), REJECTED);
    check("fresh positive after gap", process(&d, 145, (Sample){3, 145, 1}), ACCEPTED);
    check("new negative", process(&d, 150, (Sample){-3, 150, 1}), ACCEPTED);
    check("duplicate", process(&d, 151, (Sample){-3, 150, 1}), DUPLICATE);
    check("fresh positive crossing", process(&d, 155, (Sample){3, 155, 1}), CROSSING);
    check("repeat crossing ignored", process(&d, 156, (Sample){3, 155, 1}), DUPLICATE);
    init(&d);
    check("age exactly 20", process(&d, 120, (Sample){-3, 100, 1}), ACCEPTED);
    check("invalid breaks continuity", process(&d, 121, (Sample){3, 121, 0}), REJECTED);
    check("no crossing after invalid", process(&d, 122, (Sample){3, 122, 1}), ACCEPTED);
    check("future timestamp", process(&d, 123, (Sample){3, 124, 1}), REJECTED);
    check("older record", process(&d, 124, (Sample){-3, 121, 1}), REJECTED);
    printf("%u/%u passed\n", total - failures, total);
    return failures ? 1 : 0;
}
