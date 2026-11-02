#include <stdio.h>

/* Event-driven teaching model: no GPIO, timer, ADC, or motor output. */
typedef enum { IDLE, ALIGN, RAMP, RUN, FAULT } State;
typedef enum { START, ALIGN_DONE, HANDOFF_OK, TIMEOUT, DRIVER_FAULT,
               STOP, RECOVER, SIGNAL_RETURNED } Event;

/* permission means start conditions or recovery conditions, as applicable. */
static State next_state(State state, Event event, int permission)
{
    if (state == FAULT) {
        if (event == RECOVER && permission) return IDLE;
        return FAULT;
    }
    if (event == DRIVER_FAULT) return FAULT;
    if (event == STOP) return IDLE;
    switch (state) {
    case IDLE:  return (event == START && permission) ? ALIGN : IDLE;
    case ALIGN: return event == ALIGN_DONE ? RAMP : ALIGN;
    case RAMP:
        if (event == TIMEOUT) return FAULT;
        return event == HANDOFF_OK ? RUN : RAMP;
    case RUN:   return event == TIMEOUT ? FAULT : RUN;
    case FAULT: return FAULT; /* handled above */
    }
    return FAULT;
}

static unsigned total, failures;
static void test(const char *name, State got, State expected)
{
    int ok = got == expected;
    ++total;
    if (!ok) ++failures;
    printf("%s: %s (got %d, expected %d)\n", name, ok ? "PASS" : "FAIL", (int)got, (int)expected);
}

int main(void)
{
    State s = IDLE;
    s = next_state(s, START, 0); test("start blocked", s, IDLE);
    s = next_state(s, START, 1); test("start accepted", s, ALIGN);
    s = next_state(s, HANDOFF_OK, 1); test("handoff ignored in align", s, ALIGN);
    s = next_state(s, ALIGN_DONE, 1); test("alignment complete", s, RAMP);
    s = next_state(s, HANDOFF_OK, 1); test("handoff accepted", s, RUN);
    s = next_state(s, TIMEOUT, 1); test("run timeout", s, FAULT);
    s = next_state(s, SIGNAL_RETURNED, 1); test("signal does not reset", s, FAULT);
    s = next_state(s, STOP, 1); test("stop does not reset", s, FAULT);
    s = next_state(s, RECOVER, 0); test("recovery blocked", s, FAULT);
    s = next_state(s, RECOVER, 1); test("recover to idle", s, IDLE);
    test("new start required", s, IDLE);
    s = next_state(s, START, 1); test("new start", s, ALIGN);
    s = next_state(s, ALIGN_DONE, 1); s = next_state(s, TIMEOUT, 1);
    test("ramp timeout", s, FAULT);
    test("stop from align", next_state(ALIGN, STOP, 1), IDLE);
    test("stop from ramp", next_state(RAMP, STOP, 1), IDLE);
    test("stop from run", next_state(RUN, STOP, 1), IDLE);
    test("driver fault from idle", next_state(IDLE, DRIVER_FAULT, 1), FAULT);
    test("driver fault from run", next_state(RUN, DRIVER_FAULT, 1), FAULT);
    printf("%u/%u passed\n", total - failures, total);
    return failures ? 1 : 0;
}
