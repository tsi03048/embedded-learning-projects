#include <stdio.h>

/* Toy model: synthetic values and a monotonic virtual clock, not motor firmware. */
typedef enum { IDLE, ALIGN, RAMP, RUN, FAULT } State;
typedef struct {
    State state;
    unsigned step_start, last_signal;
    int armed;
} Model;

static void start(Model *m)
{
    if (m->state == IDLE) m->state = ALIGN;
}

static void align_done(Model *m, unsigned now)
{
    if (m->state != ALIGN) return;
    m->state = RAMP;
    m->step_start = now;
    m->last_signal = now;
    m->armed = 0;
}

static void tick(Model *m, unsigned now)
{
    if ((m->state == RAMP || m->state == RUN) && now - m->last_signal >= 200)
        m->state = FAULT;
}

/* For this fixed virtual step: rising crossing, OFF window 42..49 us. */
static void sample(Model *m, unsigned now, int difference)
{
    unsigned phase;
    tick(m, now);
    if (m->state != RAMP && m->state != RUN) return;
    if (now - m->step_start < 80) return;
    phase = now % 50;
    if (phase < 42 || phase > 46) return; /* 3 us must finish by 49. */
    if (difference <= -2) m->armed = 1;
    if (m->armed && difference >= 2) {
        m->last_signal = now;
        if (m->state == RAMP) m->state = RUN;
        m->armed = 0;
    }
}

static void stop(Model *m)
{
    if (m->state != FAULT) m->state = IDLE;
}

static void reset_fault(Model *m, int cleared)
{
    if (m->state == FAULT && cleared) m->state = IDLE;
}

static unsigned total, failures;
static void check(const char *name, int condition)
{
    ++total;
    if (!condition) ++failures;
    printf("%s: %s\n", name, condition ? "PASS" : "FAIL");
}

int main(void)
{
    Model m = {IDLE, 0, 0, 0};
    start(&m); check("start", m.state == ALIGN);
    align_done(&m, 10); check("alignment event", m.state == RAMP);
    sample(&m, 42, -3); check("blanking ignores sample", m.armed == 0);
    sample(&m, 92, -3); check("negative arms", m.armed == 1);
    sample(&m, 100, 3); check("outside window ignored", m.state == RAMP && m.armed == 1);
    sample(&m, 142, 1); check("small positive waits", m.state == RAMP);
    sample(&m, 146, 3); check("crossing enters run", m.state == RUN && m.last_signal == 146);
    tick(&m, 345); check("before timeout", m.state == RUN);
    tick(&m, 346); check("at timeout", m.state == FAULT);
    stop(&m); check("stop keeps fault", m.state == FAULT);
    reset_fault(&m, 0); check("uncleared fault", m.state == FAULT);
    reset_fault(&m, 1); check("separate reset to idle", m.state == IDLE);
    start(&m); align_done(&m, 400); tick(&m, 600);
    check("ramp timeout", m.state == FAULT);
    printf("%u/%u passed\n", total - failures, total);
    return failures ? 1 : 0;
}
