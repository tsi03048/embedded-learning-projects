#include <stdio.h>

/* Simplified learning model; all times and signal values are imaginary. */
typedef enum { IDLE, ALIGN, RAMP, RUN, FAULT } State;
typedef struct { State state; unsigned ramp_start, last_event; int armed; } Model;

static void start(Model *m) { if (m->state == IDLE) m->state = ALIGN; }
static void align_done(Model *m, unsigned now)
{
    if (m->state == ALIGN) {
        m->state = RAMP; m->ramp_start = now; m->last_event = now; m->armed = 0;
    }
}
static void tick(Model *m, unsigned now)
{
    if ((m->state == RAMP || m->state == RUN) && now - m->last_event >= 200)
        m->state = FAULT;
}
static void sample(Model *m, unsigned now, int signal)
{
    unsigned phase;
    tick(m, now);
    if (m->state != RAMP && m->state != RUN) return;
    if (now - m->ramp_start < 80) return;
    phase = now % 50;
    if (phase < 42 || phase > 46) return;
    if (signal <= -2) m->armed = 1;
    if (m->armed && signal >= 2) {
        m->last_event = now; m->armed = 0;
        if (m->state == RAMP) m->state = RUN;
    }
}
static void stop(Model *m) { if (m->state != FAULT) m->state = IDLE; }
static void recover(Model *m, int cleared)
{
    if (m->state == FAULT && cleared) m->state = IDLE;
}
static const char *state_name(State state)
{
    static const char *names[] = {"IDLE", "ALIGN", "RAMP", "RUN", "FAULT"};
    return names[state];
}
static unsigned total, failed;
static void expect(const char *scenario, State got, State wanted)
{
    int ok = got == wanted;
    ++total;
    if (!ok) ++failed;
    printf("%s: %s (got %s, expected %s)\n", scenario, ok ? "PASS" : "FAIL",
           state_name(got), state_name(wanted));
}
static void begin_ramp(Model *m)
{
    m->state = IDLE; m->ramp_start = 0; m->last_event = 0; m->armed = 0;
    start(m); align_done(m, 10);
}
int main(void)
{
    Model m;
    begin_ramp(&m); sample(&m, 92, -3); sample(&m, 146, 3);
    expect("1 normal crossing", m.state, RUN);
    stop(&m); expect("1 stop", m.state, IDLE);

    begin_ramp(&m); tick(&m, 210);
    expect("2 no signal timeout", m.state, FAULT);

    begin_ramp(&m); sample(&m, 100, -3); sample(&m, 150, 3); tick(&m, 210);
    expect("3 outside PWM window", m.state, FAULT);

    begin_ramp(&m); sample(&m, 92, -3); sample(&m, 146, 1); tick(&m, 210);
    expect("4 below upper threshold", m.state, FAULT);

    begin_ramp(&m); sample(&m, 92, -3); sample(&m, 146, 3); tick(&m, 346);
    expect("5 run signal timeout", m.state, FAULT);
    recover(&m, 0); expect("5 recovery denied", m.state, FAULT);
    recover(&m, 1); expect("5 recovery returns idle", m.state, IDLE);
    start(&m); expect("5 separate start", m.state, ALIGN);

    printf("%u/%u checks passed\n", total - failed, total);
    return failed ? 1 : 0;
}
