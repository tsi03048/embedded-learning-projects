#include <stdint.h>
#include <stdio.h>

typedef enum { REQUEST_STOP, INCREASE, DECREASE, KEEP, NO_DECISION } Decision;
#define BAND_RPM 10

/* Teaching decision only; unsigned speed magnitudes, no actual motor control. */
static Decision compare_speed(uint32_t target, uint32_t measured, int valid)
{
    int64_t error;
    if (target == 0u) return REQUEST_STOP;
    if (!valid) return NO_DECISION;
    error = (int64_t)target - (int64_t)measured;
    if (error > BAND_RPM) return INCREASE;
    if (error < -BAND_RPM) return DECREASE;
    return KEEP;
}
static int check(int ok, const char *name)
{
    printf("%s: %s\n", ok ? "PASS" : "FAIL", name);
    return ok ? 0 : 1;
}
int main(void)
{
    int errors = 0;
    errors += check(compare_speed(300u, 240u, 1) == INCREASE, "below target");
    errors += check(compare_speed(300u, 350u, 1) == DECREASE, "above target");
    errors += check(compare_speed(300u, 300u, 1) == KEEP, "at target");
    errors += check(compare_speed(300u, 290u, 1) == KEEP, "lower band boundary");
    errors += check(compare_speed(300u, 310u, 1) == KEEP, "upper band boundary");
    errors += check(compare_speed(300u, 289u, 1) == INCREASE, "just below band");
    errors += check(compare_speed(300u, 311u, 1) == DECREASE, "just above band");
    errors += check(compare_speed(300u, 0u, 0) == NO_DECISION, "invalid feedback");
    errors += check(compare_speed(300u, 0u, 1) == INCREASE, "valid zero speed");
    errors += check(compare_speed(0u, 120u, 0) == REQUEST_STOP, "zero target requests stop");
    errors += check(compare_speed(UINT32_MAX, 1u, 1) == INCREASE, "large positive difference");
    errors += check(compare_speed(1u, UINT32_MAX, 1) == DECREASE, "large negative difference");
    printf("Result: %d failure(s)\n", errors);
    return errors ? 1 : 0;
}
