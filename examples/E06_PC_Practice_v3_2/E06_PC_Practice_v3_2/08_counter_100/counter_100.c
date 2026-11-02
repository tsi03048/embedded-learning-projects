#include <stdint.h>
#include <stdio.h>

/* Virtual up-counter 0..99; true elapsed duration must be below 100 ticks. */
static int elapsed_100(unsigned before, unsigned now, unsigned *ticks)
{
    if (before >= 100 || now >= 100) return 0;
    if (now >= before) *ticks = now - before;
    else *ticks = (100 - before) + now;
    return 1;
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
    unsigned ticks = 0, start, actual, mismatches = 0;
    check("95 to 3 is 8", elapsed_100(95, 3, &ticks) && ticks == 8);
    check("98 to 2 is 4", elapsed_100(98, 2, &ticks) && ticks == 4);
    check("12 to 19 is 7", elapsed_100(12, 19, &ticks) && ticks == 7);
    check("99 to 0 is 1", elapsed_100(99, 0, &ticks) && ticks == 1);
    check("same value returns zero", elapsed_100(42, 42, &ticks) && ticks == 0);
    check("invalid input rejected", !elapsed_100(100, 3, &ticks));
    check("256-based answer differs", (uint8_t)(3 - 95) == 164);
    for (start = 0; start < 100; ++start) {
        for (actual = 0; actual < 100; ++actual) {
            unsigned now = (start + actual) % 100;
            if (!elapsed_100(start, now, &ticks) || ticks != actual) ++mismatches;
        }
    }
    check("all 10000 virtual pairs", mismatches == 0);
    printf("%u/%u checks passed\n", total - failures, total);
    return failures ? 1 : 0;
}
