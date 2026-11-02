#include <stdio.h>

/* Single-threaded virtual model: no ADC, timer, or motor hardware. */
typedef enum { U, V, W } Phase;
typedef struct { unsigned generation; Phase phase; } Context;
typedef struct { int busy; Context origin; } Slot;
typedef struct { Context origin; int value; } Result;

/* 1 means the virtual request was accepted; 0 means it was rejected. */
static int begin(Slot *slot, Context origin)
{
    if (slot->busy) return 0;
    slot->origin = origin;
    slot->busy = 1;
    return 1;
}

/* Failed completion leaves *result unchanged. */
static int complete(Slot *slot, int value, Result *result)
{
    if (!slot->busy) return 0;
    result->origin = slot->origin;
    result->value = value;
    slot->busy = 0;
    return 1;
}

static int matches(Result result, Context current)
{
    return result.origin.generation == current.generation && result.origin.phase == current.phase;
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
    Context a = {1, W}, b = {2, V};
    Slot slot = {0, {0, U}};
    Result r = {{0, U}, 0};
    check("no completion while idle", !complete(&slot, 9, &r));
    check("A accepted", begin(&slot, a));
    check("B rejected while busy", !begin(&slot, b));
    check("A metadata retained", slot.busy && slot.origin.generation == 1 && slot.origin.phase == W);
    check("A completes", complete(&slot, -3, &r));
    check("result belongs to A", r.origin.generation == 1 && r.origin.phase == W && r.value == -3);
    check("A result not current B", !matches(r, b));
    check("duplicate completion rejected", !complete(&slot, 0, &r));
    check("B accepted after completion", begin(&slot, b));
    check("B result belongs to B", complete(&slot, 3, &r) && matches(r, b) && r.value == 3);
    printf("%u/%u checks passed\n", total - failures, total);
    return failures ? 1 : 0;
}
