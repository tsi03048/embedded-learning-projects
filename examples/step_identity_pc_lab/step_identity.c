#include <stdio.h>

/* Virtual input only: no ADC, PWM, timer, or motor is used. */
typedef enum { U, V, W } Phase;
typedef enum { RISING, FALLING } Direction;
typedef enum { IGNORED, WAITING, CROSSING } Result;
typedef struct {
    unsigned generation, crossings;
    Phase phase;
    Direction direction;
    int armed, fired, active;
} Detector;
typedef struct { unsigned generation; Phase phase; int value; } Sample;

/* Caller supplies a new, non-repeating generation on each step change. */
static void begin_step(Detector *d, unsigned generation, Phase phase, Direction direction)
{
    d->generation = generation;
    d->phase = phase;
    d->direction = direction;
    d->armed = 0;
    d->fired = 0;
    d->active = 1;
    /* crossings is global diagnostic data and is not reset on step change. */
}

static Result sample(Detector *d, Sample s)
{
    if (!d->active || s.generation != d->generation || s.phase != d->phase || d->fired)
        return IGNORED;
    if ((d->direction == RISING && s.value <= -2) ||
        (d->direction == FALLING && s.value >= 2)) d->armed = 1;
    if (d->armed && ((d->direction == RISING && s.value >= 2) ||
                     (d->direction == FALLING && s.value <= -2))) {
        d->fired = 1;
        ++d->crossings;
        return CROSSING;
    }
    return WAITING;
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
    Detector d = {0, 0, U, RISING, 0, 0, 0};
    check("inactive ignored", sample(&d, (Sample){1, W, -3}) == IGNORED);
    begin_step(&d, 1, W, RISING);
    check("W negative arms", sample(&d, (Sample){1, W, -3}) == WAITING && d.armed);
    begin_step(&d, 2, V, RISING);
    check("new step resets memory", !d.armed && !d.fired);
    check("V positive alone waits", sample(&d, (Sample){2, V, 3}) == WAITING && !d.armed);
    check("old step W ignored", sample(&d, (Sample){1, W, -3}) == IGNORED && !d.armed);
    check("wrong phase ignored", sample(&d, (Sample){2, W, -3}) == IGNORED && !d.armed);
    check("V negative arms", sample(&d, (Sample){2, V, -3}) == WAITING && d.armed);
    check("V crossing", sample(&d, (Sample){2, V, 3}) == CROSSING && d.crossings == 1);
    check("one event per step", sample(&d, (Sample){2, V, 3}) == IGNORED && d.crossings == 1);
    begin_step(&d, 3, V, FALLING);
    check("old generation same phase ignored", sample(&d, (Sample){2, V, 3}) == IGNORED && !d.armed);
    check("global count retained", d.crossings == 1 && !d.fired);
    check("falling arms high", sample(&d, (Sample){3, V, 3}) == WAITING && d.armed);
    check("falling crossing", sample(&d, (Sample){3, V, -3}) == CROSSING && d.crossings == 2);
    printf("%u/%u checks passed\n", total - failures, total);
    return failures ? 1 : 0;
}
