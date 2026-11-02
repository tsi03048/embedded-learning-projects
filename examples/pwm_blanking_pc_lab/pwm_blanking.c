#include <stdio.h>

/* Times are virtual integer microseconds, on one non-wrapping timeline. */
typedef enum { BAD_CONFIG, OUTSIDE_STEP, BLANKED, OUTSIDE_WINDOW, ACCEPTED } Result;

static Result classify(unsigned time, unsigned step_start, unsigned step_end,
                       unsigned blank, unsigned period, unsigned on,
                       unsigned guard_start, unsigned guard_end, unsigned adc_time)
{
    unsigned off, phase, window_start, window_end;
    if (period == 0 || on > period || adc_time == 0 || step_start >= step_end)
        return BAD_CONFIG;
    off = period - on;
    if (guard_start > off || guard_end > off - guard_start) return BAD_CONFIG;
    if (time < step_start || time >= step_end || adc_time > step_end - time)
        return OUTSIDE_STEP;
    if (time - step_start < blank) return BLANKED;
    phase = time % period; /* PWM timeline starts at virtual time zero. */
    window_start = on + guard_start;
    window_end = period - guard_end;
    if (phase < window_start || phase >= window_end || adc_time > window_end - phase)
        return OUTSIDE_WINDOW;
    return ACCEPTED;
}

static unsigned total, failures;
static void test(const char *label, Result got, Result expected)
{
    int ok = got == expected;
    ++total;
    if (!ok) ++failures;
    printf("%s: %s (got %d, expected %d)\n", label, ok ? "PASS" : "FAIL", (int)got, (int)expected);
}

int main(void)
{
    /* PWM: period 50, ON 40, OFF 40..50, guards 2 and 1, ADC 3. */
    test("42 blanked", classify(42, 0, 200, 80, 50, 40, 2, 1, 3), BLANKED);
    test("92 accepted", classify(92, 0, 200, 80, 50, 40, 2, 1, 3), ACCEPTED);
    test("at blank end but PWM ON", classify(80, 0, 200, 80, 50, 40, 2, 1, 3), OUTSIDE_WINDOW);
    test("latest exact fit", classify(96, 0, 200, 80, 50, 40, 2, 1, 3), ACCEPTED);
    test("measurement overruns window", classify(97, 0, 200, 80, 50, 40, 2, 1, 3), OUTSIDE_WINDOW);
    test("at step end", classify(200, 0, 200, 80, 50, 40, 2, 1, 3), OUTSIDE_STEP);
    test("measurement overruns step", classify(92, 0, 94, 80, 50, 40, 2, 1, 3), OUTSIDE_STEP);
    test("blank ends at sample", classify(92, 12, 200, 80, 50, 40, 2, 1, 3), ACCEPTED);
    test("different step still blanked", classify(92, 13, 200, 80, 50, 40, 2, 1, 3), BLANKED);
    test("before step", classify(42, 43, 200, 0, 50, 40, 2, 1, 3), OUTSIDE_STEP);
    test("invalid guards", classify(92, 0, 200, 0, 50, 40, 9, 2, 3), BAD_CONFIG);
    test("no OFF time", classify(92, 0, 200, 0, 50, 50, 0, 0, 3), OUTSIDE_WINDOW);
    test("zero period", classify(92, 0, 200, 0, 0, 0, 0, 0, 3), BAD_CONFIG);
    printf("%u/%u passed\n", total - failures, total);
    return failures ? 1 : 0;
}
