#include <stdio.h>

/* Synthetic records and monotonic virtual milliseconds; not motor firmware. */
typedef struct { unsigned measured; int value, valid, in_window, blanked; } Sample;
typedef struct { unsigned start, last; int have_last, armed, timeout; } Monitor;
typedef enum { IGNORED, REJECTED, DUPLICATE, ACCEPTED, CROSSING } Result;

static void begin(Monitor *m, unsigned now)
{
    m->start = now; m->last = 0; m->have_last = 0; m->armed = 0; m->timeout = 0;
}

static void tick(Monitor *m, unsigned now)
{
    unsigned reference = m->have_last ? m->last : m->start;
    if (!m->timeout && now - reference >= 30) m->timeout = 1;
}

static Result receive(Monitor *m, unsigned now, Sample s)
{
    tick(m, now);
    if (m->timeout) return IGNORED;
    if (!s.valid || s.measured > now || s.measured < m->start || now - s.measured > 10) {
        m->armed = 0;
        return REJECTED;
    }
    if (m->have_last && s.measured == m->last) return DUPLICATE;
    if (m->have_last && s.measured < m->last) { m->armed = 0; return REJECTED; }
    if (!s.in_window || s.blanked) return IGNORED;
    m->last = s.measured; m->have_last = 1;
    if (s.value <= -2) m->armed = 1;
    if (m->armed && s.value >= 2) { m->armed = 0; return CROSSING; }
    return ACCEPTED;
}

static unsigned total, failures;
static void check(const char *name, int ok)
{
    ++total;
    if (!ok) ++failures;
    printf("%s: %s\n", name, ok ? "PASS" : "FAIL");
}

int main(void)
{
    Monitor m; Result r;
    begin(&m, 100);
    r = receive(&m, 100, (Sample){100, -3, 1, 1, 0});
    check("A negative", r == ACCEPTED && m.armed && m.last == 100);
    r = receive(&m, 110, (Sample){110, 3, 1, 1, 0});
    check("A crossing", r == CROSSING && m.last == 110);
    r = receive(&m, 120, (Sample){110, 3, 1, 1, 0});
    check("A duplicate", r == DUPLICATE && m.last == 110);
    tick(&m, 139); check("A before timeout", !m.timeout);
    tick(&m, 140); check("A exact timeout", m.timeout);
    r = receive(&m, 141, (Sample){141, -3, 1, 1, 0});
    check("A timeout stays latched", r == IGNORED && m.timeout);

    begin(&m, 200);
    r = receive(&m, 200, (Sample){200, -3, 1, 1, 0});
    check("B negative", r == ACCEPTED && m.armed);
    r = receive(&m, 220, (Sample){205, 3, 1, 1, 0});
    check("B stale clears continuity", r == REJECTED && !m.armed && m.last == 200);
    r = receive(&m, 225, (Sample){225, 3, 1, 1, 0});
    check("B fresh positive not crossing", r == ACCEPTED && m.last == 225);

    begin(&m, 300);
    r = receive(&m, 305, (Sample){305, -3, 1, 1, 1});
    check("C blanked ignored", r == IGNORED && !m.armed && !m.have_last);
    r = receive(&m, 310, (Sample){310, 3, 1, 1, 0});
    check("C no false crossing", r == ACCEPTED && !m.armed);
    r = receive(&m, 315, (Sample){315, -3, 1, 0, 0});
    check("outside window ignored", r == IGNORED && m.last == 310);
    tick(&m, 340); check("C timeout independent of input", m.timeout);
    printf("%u/%u checks passed\n", total - failures, total);
    return failures ? 1 : 0;
}
