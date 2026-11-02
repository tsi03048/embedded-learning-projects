#include <stdio.h>

/* All values are hypothetical integer microseconds, within a single PWM period. */
typedef struct { int found; unsigned start_us; } Window;

static Window find_off_window(unsigned period, unsigned on,
                              unsigned guard_start, unsigned guard_end,
                              unsigned adc_time)
{
    Window result = {0, 0};
    unsigned off, usable;
    if (period == 0 || on > period || adc_time == 0) return result;
    off = period - on;
    if (off == 0 || guard_start > off) return result;
    usable = off - guard_start;
    if (guard_end > usable) return result;
    usable -= guard_end;
    if (adc_time > usable) return result;
    result.found = 1;
    result.start_us = on + guard_start;
    return result;
}

static unsigned total, failures;
static void test(const char *name, Window got, int expected_found, unsigned expected_start)
{
    int ok = got.found == expected_found && (!got.found || got.start_us == expected_start);
    ++total;
    if (!ok) ++failures;
    printf("%s: %s", name, ok ? "PASS" : "FAIL");
    if (got.found) printf(" (start %u us)", got.start_us);
    else printf(" (no window)");
    printf("\n");
}

int main(void)
{
    test("50us 80 percent", find_off_window(50, 40, 2, 1, 3), 1, 42);
    test("90 percent exact fit", find_off_window(50, 45, 1, 1, 3), 1, 46);
    test("not enough time", find_off_window(50, 46, 1, 1, 3), 0, 0);
    test("100 percent", find_off_window(50, 50, 0, 0, 1), 0, 0);
    test("0 percent toy case", find_off_window(50, 0, 2, 1, 3), 1, 2);
    test("guard exceeds off", find_off_window(50, 49, 2, 0, 1), 0, 0);
    test("end guard exceeds remainder", find_off_window(50, 48, 1, 2, 1), 0, 0);
    test("on exceeds period", find_off_window(50, 51, 0, 0, 1), 0, 0);
    test("zero period", find_off_window(0, 0, 0, 0, 1), 0, 0);
    test("zero adc time", find_off_window(50, 40, 0, 0, 0), 0, 0);
    printf("%u/%u passed\n", total - failures, total);
    return failures ? 1 : 0;
}
