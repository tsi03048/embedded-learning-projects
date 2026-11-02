#include <stdint.h>
#include <stdio.h>

/* Counts one chosen edge type on the measured shaft. 0 means invalid input. */
static int estimate_rpm(uint32_t edges, uint32_t edges_per_rev,
                        uint32_t window_ms, uint32_t *rpm)
{
    uint64_t numerator, denominator, result;
    if (edges_per_rev == 0u || window_ms == 0u) return 0;
    numerator = (uint64_t)edges * 60000u;
    denominator = (uint64_t)edges_per_rev * window_ms;
    result = numerator / denominator; /* truncate fractional RPM */
    if (result > UINT32_MAX) return 0;
    *rpm = (uint32_t)result;
    return 1;
}
static int check(int ok, const char *name)
{
    printf("%s: %s\n", ok ? "PASS" : "FAIL", name);
    return ok ? 0 : 1;
}
int main(void)
{
    uint32_t rpm = 999u;
    int ok, errors = 0;
    ok=estimate_rpm(4u,20u,200u,&rpm);
    errors+=check(ok && rpm==60u,"four edges in 200ms");
    ok=estimate_rpm(20u,20u,200u,&rpm);
    errors+=check(ok && rpm==300u,"one revolution in 200ms");
    ok=estimate_rpm(1u,20u,200u,&rpm);
    errors+=check(ok && rpm==15u,"one edge resolution");
    ok=estimate_rpm(0u,20u,200u,&rpm);
    errors+=check(ok && rpm==0u,"zero observed edges");
    ok=estimate_rpm(1u,7u,1000u,&rpm);
    errors+=check(ok && rpm==8u,"fraction truncated");
    rpm=999u; ok=estimate_rpm(1u,0u,200u,&rpm);
    errors+=check(!ok && rpm==999u,"zero edges per revolution rejected");
    rpm=999u; ok=estimate_rpm(1u,20u,0u,&rpm);
    errors+=check(!ok && rpm==999u,"zero time rejected");
    rpm=999u; ok=estimate_rpm(UINT32_MAX,1u,1u,&rpm);
    errors+=check(!ok && rpm==999u,"result overflow rejected");
    ok=estimate_rpm(20u,20u,1000u,&rpm);
    errors+=check(ok && rpm==60u,"one revolution per second");
    printf("Result: %d failure(s)\n",errors);
    return errors ? 1 : 0;
}
