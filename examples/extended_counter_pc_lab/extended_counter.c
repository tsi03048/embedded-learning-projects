#include <stdint.h>
#include <stdio.h>

/* Virtual coherent snapshots. This code never reads a hardware timer. */
typedef struct { uint32_t wraps; unsigned counter; } Snapshot;

static int elapsed_ticks(Snapshot first, Snapshot last, uint64_t *ticks)
{
    uint64_t a, b;
    if (first.counter >= 100 || last.counter >= 100) return 0;
    a = (uint64_t)first.wraps * 100u + first.counter;
    b = (uint64_t)last.wraps * 100u + last.counter;
    if (b < a) return 0;
    *ticks = b - a;
    return 1;
}

static unsigned total, failures;
static void check(const char *label, int ok)
{
    ++total;
    if (!ok) ++failures;
    printf("%s: %s\n", label, ok ? "PASS" : "FAIL");
}

int main(void)
{
    uint64_t ticks = 0;
    check("one wrap: 8", elapsed_ticks((Snapshot){3, 95}, (Snapshot){4, 3}, &ticks) && ticks == 8);
    check("extra wrap: 108", elapsed_ticks((Snapshot){3, 95}, (Snapshot){5, 3}, &ticks) && ticks == 108);
    check("no wrap: 5", elapsed_ticks((Snapshot){3, 10}, (Snapshot){3, 15}, &ticks) && ticks == 5);
    check("same snapshot: 0", elapsed_ticks((Snapshot){3, 42}, (Snapshot){3, 42}, &ticks) && ticks == 0);
    check("same counter one wrap: 100", elapsed_ticks((Snapshot){3, 42}, (Snapshot){4, 42}, &ticks) && ticks == 100);
    check("invalid counter rejected", !elapsed_ticks((Snapshot){3, 100}, (Snapshot){4, 3}, &ticks));
    check("backwards snapshot rejected", !elapsed_ticks((Snapshot){4, 3}, (Snapshot){3, 95}, &ticks));
    check("wide multiply before add", elapsed_ticks((Snapshot){0, 0}, (Snapshot){UINT32_MAX, 99}, &ticks)
          && ticks == (uint64_t)UINT32_MAX * 100u + 99u);
    printf("%u/%u checks passed\n", total - failures, total);
    return failures ? 1 : 0;
}
