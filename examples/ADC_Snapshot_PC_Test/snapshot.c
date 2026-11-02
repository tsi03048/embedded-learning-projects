#include <stdint.h>
#include <stdio.h>

typedef struct {
    uint16_t raw;
    uint32_t tick;
    int valid;
} Sample;

/* Single-threaded simulation: not a real interrupt-safe implementation. */
static void publish(Sample *shared, uint16_t raw, uint32_t tick)
{
    shared->raw = raw;
    shared->tick = tick;
    shared->valid = 1;
}
static Sample copy_serialized(const Sample *shared)
{
    return *shared; /* In this PC example, publish cannot run during copy. */
}
static int check(int ok, const char *name)
{
    printf("%s: %s\n", ok ? "PASS" : "FAIL", name);
    return ok ? 0 : 1;
}
int main(void)
{
    Sample shared = {0u, 0u, 0};
    Sample snapshot;
    uint16_t old_raw;
    uint32_t new_tick;
    int errors = 0;
    snapshot = copy_serialized(&shared);
    errors += check(!snapshot.valid, "before first result");
    publish(&shared, 1000u, 10u);
    old_raw = shared.raw;              /* deliberately interleave a writer */
    publish(&shared, 2000u, 20u);
    new_tick = shared.tick;
    errors += check(old_raw == 1000u && new_tick == 20u, "mixed read reproduced");
    snapshot = copy_serialized(&shared);
    errors += check(snapshot.valid && snapshot.raw == 2000u && snapshot.tick == 20u,
                    "serialized copy returns matching fields");
    publish(&shared, 0u, 30u);
    errors += check(snapshot.raw == 2000u && snapshot.tick == 20u, "copied result remains unchanged");
    snapshot = copy_serialized(&shared);
    errors += check(snapshot.valid && snapshot.raw == 0u && snapshot.tick == 30u,
                    "zero is valid with matching tick");
    shared.valid = 0; /* simulated failure; raw may retain its old value */
    snapshot = copy_serialized(&shared);
    errors += check(!snapshot.valid, "failure marks data invalid");
    printf("Result: %d failure(s)\n", errors);
    return errors ? 1 : 0;
}
