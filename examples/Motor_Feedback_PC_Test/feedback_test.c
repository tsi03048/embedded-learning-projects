#include <stdint.h>
#include <stdio.h>

typedef enum { WAITING, RECENT_EDGE, NO_RECENT_EDGE, SENSOR_FAULT } FeedbackState;
typedef struct { FeedbackState state; uint32_t last_tick; } Feedback;
#define LIMIT_TICKS 1000u /* simulated ticks; not a real motor setting */

static void begin(Feedback *f, uint32_t now)
{
    f->state = WAITING;
    f->last_tick = now;
}
/* Fault is separately injected; no pulses alone cannot diagnose it. */
static void step(Feedback *f, uint32_t now, int edge, int fault)
{
    if (f->state == SENSOR_FAULT) return; /* latched until begin() */
    if (fault) {
        f->state = SENSOR_FAULT;
    } else if (edge) {
        f->last_tick = now;
        f->state = RECENT_EDGE;
    } else if ((uint32_t)(now - f->last_tick) >= LIMIT_TICKS) {
        f->state = NO_RECENT_EDGE;
    }
}
static int check(int ok, const char *name)
{
    printf("%s: %s\n", ok ? "PASS" : "FAIL", name);
    return ok ? 0 : 1;
}
int main(void)
{
    Feedback f;
    int errors = 0;
    begin(&f, 0u);
    step(&f, 999u, 0, 0);
    errors += check(f.state == WAITING, "waiting before limit");
    step(&f, 1000u, 0, 0);
    errors += check(f.state == NO_RECENT_EDGE, "no first edge at limit");
    step(&f, 1100u, 1, 0);
    errors += check(f.state == RECENT_EDGE, "new edge observed");
    step(&f, 2099u, 0, 0);
    errors += check(f.state == RECENT_EDGE, "edge still recent");
    step(&f, 2100u, 0, 0);
    errors += check(f.state == NO_RECENT_EDGE, "no recent edge at limit");
    step(&f, 2101u, 1, 0);
    errors += check(f.state == RECENT_EDGE, "late edge observed");
    step(&f, 3101u, 1, 0);
    errors += check(f.state == RECENT_EDGE, "edge wins at limit");
    step(&f, 3102u, 1, 1);
    errors += check(f.state == SENSOR_FAULT, "fault wins over edge");
    step(&f, 3200u, 1, 0);
    errors += check(f.state == SENSOR_FAULT, "fault stays latched");
    begin(&f, UINT32_MAX - 200u); /* reset for PC test only */
    step(&f, 798u, 0, 0);
    errors += check(f.state == WAITING, "tick wrap before limit");
    step(&f, 799u, 0, 0);
    errors += check(f.state == NO_RECENT_EDGE, "tick wrap at limit");
    printf("Result: %d failure(s)\n", errors);
    return errors ? 1 : 0;
}
