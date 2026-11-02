#include <stdio.h>

/* Deterministic teaching model; not a hardware timer or interrupt handler. */
typedef struct { unsigned physical_wraps, software_wraps, counter; } MockTimer;

static void step(MockTimer *t)
{
    if (t->counter == 99) { t->counter = 0; ++t->physical_wraps; }
    else ++t->counter;
}

/* Simulates the delayed update of the software count, not real ISR code. */
static void service(MockTimer *t) { t->software_wraps = t->physical_wraps; }
static unsigned position(unsigned wraps, unsigned counter) { return wraps * 100 + counter; }

static unsigned total, failures;
static void check(const char *name, int ok)
{
    ++total;
    if (!ok) ++failures;
    printf("%s: %s\n", name, ok ? "PASS" : "FAIL");
}

int main(void)
{
    MockTimer t = {3, 3, 98};
    unsigned first, second, low, combined, truth;

    first = t.software_wraps; low = t.counter;
    combined = position(first, low); truth = position(t.physical_wraps, t.counter);
    printf("stable: combined=%u, truth=%u\n", combined, truth);
    check("stable snapshot", combined == 398 && combined == truth);

    t.counter = 99;
    first = t.software_wraps; /* read the old software count */
    step(&t); service(&t);   /* hardware wraps and software updates */
    low = t.counter;         /* read the new counter */
    combined = position(first, low); truth = position(t.physical_wraps, t.counter);
    printf("mixed: combined=%u, truth=%u\n", combined, truth);
    check("mixed snapshot is wrong", combined == 300 && truth == 400);

    t.physical_wraps = 3; t.software_wraps = 3; t.counter = 99;
    first = t.software_wraps;
    step(&t); service(&t);
    low = t.counter; second = t.software_wraps;
    check("double read notices update", first != second && low == 0);

    t.physical_wraps = 3; t.software_wraps = 3; t.counter = 99;
    step(&t); /* hardware wrapped; simulated software update is still pending */
    first = t.software_wraps; low = t.counter; second = t.software_wraps;
    combined = position(first, low); truth = position(t.physical_wraps, t.counter);
    printf("pending: high1=%u, low=%u, high2=%u, combined=%u, truth=%u\n",
           first, low, second, combined, truth);
    check("double read can match while wrong", first == second && combined == 300 && truth == 400);
    service(&t);
    check("service catches up in model", t.software_wraps == 4);

    printf("%u/%u checks passed\n", total - failures, total);
    return failures ? 1 : 0;
}
