#include <stdint.h>
#include <stdio.h>

/* Virtual 8-bit up-counter: one tick per increment, wraps at 255. */
static uint8_t elapsed(uint8_t now, uint8_t before)
{
    /* uint8_t operands may be promoted to int; cast the result back to 8 bits. */
    return (uint8_t)(now - before);
}

static unsigned tests, failures;
static void check(const char *label, unsigned actual, unsigned expected)
{
    ++tests;
    if (actual != expected) ++failures;
    printf("%s: %s (got %u, expected %u)\n", label,
           actual == expected ? "PASS" : "FAIL", actual, expected);
}

int main(void)
{
    unsigned start, ticks, exhaustive_failures = 0;
    check("no wrap", elapsed(105, 100), 5);
    check("wrap 250 to 4", elapsed(4, 250), 10);
    check("255 to 0", elapsed(0, 255), 1);
    check("same value", elapsed(42, 42), 0);
    check("largest observable interval", elapsed(249, 250), 255);
    check("timeout before limit", elapsed(13, 250) >= 20, 0);
    check("timeout at limit", elapsed(14, 250) >= 20, 1);

    /* Verify every start and every duration 0..255 against a virtual counter. */
    for (start = 0; start < 256; ++start) {
        for (ticks = 0; ticks < 256; ++ticks) {
            uint8_t now = (uint8_t)(start + ticks);
            if (elapsed(now, (uint8_t)start) != ticks) ++exhaustive_failures;
        }
    }
    check("all 65536 start/duration pairs", exhaustive_failures, 0);
    printf("%u/%u checks passed\n", tests - failures, tests);
    return failures ? 1 : 0;
}
