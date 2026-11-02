#include <stdio.h>

/* Synthetic signed differences relative to a chosen reference; not ADC values. */
typedef enum { RISING, FALLING } Direction;
typedef struct { int last_nonzero_sign; int detected; Direction direction; } Detector;

static void begin_step(Detector *d, Direction direction)
{
    d->last_nonzero_sign = 0;
    d->detected = 0;
    d->direction = direction;
}

/* Returns 1 once per step when the expected transition is observed. */
static int sample(Detector *d, int difference)
{
    int sign;
    int match;
    if (d->detected || difference == 0) return 0;
    sign = difference > 0 ? 1 : -1;
    match = (d->direction == RISING && d->last_nonzero_sign == -1 && sign == 1) ||
            (d->direction == FALLING && d->last_nonzero_sign == 1 && sign == -1);
    d->last_nonzero_sign = sign;
    if (!match) return 0;
    d->detected = 1;
    return 1;
}

static unsigned total, failed;
static void expect(const char *name, int ok)
{
    ++total;
    if (!ok) ++failed;
    printf("%s: %s\n", name, ok ? "PASS" : "FAIL");
}

int main(void)
{
    Detector d;
    begin_step(&d, RISING);
    expect("first negative is baseline", sample(&d, -3) == 0);
    expect("zero is not enough", sample(&d, 0) == 0);
    expect("zero plateau", sample(&d, 0) == 0);
    expect("rising detected", sample(&d, 2) == 1);
    expect("only once", sample(&d, -1) == 0 && sample(&d, 1) == 0);
    begin_step(&d, FALLING);
    expect("reset removes old baseline", sample(&d, -2) == 0);
    expect("wrong direction ignored", sample(&d, 3) == 0);
    expect("falling detected", sample(&d, -1) == 1);
    begin_step(&d, RISING);
    expect("initial positive no crossing", sample(&d, 2) == 0);
    expect("unexpected falling ignored", sample(&d, -2) == 0);
    expect("later rising detected", sample(&d, 1) == 1);
    begin_step(&d, FALLING);
    expect("zeros alone not crossing", sample(&d, 0) == 0 && sample(&d, 0) == 0);
    printf("%u/%u passed\n", total - failed, total);
    return failed ? 1 : 0;
}
