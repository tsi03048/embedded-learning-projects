#include <stdio.h>
#include <stddef.h>

typedef enum { RISING, FALLING } Direction;
typedef struct { int armed; int fired; Direction direction; } Detector;

static void begin(Detector *d, Direction direction)
{
    d->armed = 0;
    d->fired = 0;
    d->direction = direction;
}

/* One detection per step. low must be negative; high must be positive. */
static int sample(Detector *d, int x, int low, int high)
{
    if (d->fired) return 0;
    if (d->direction == RISING) {
        if (x <= low) d->armed = 1;
        if (d->armed && x >= high) { d->fired = 1; return 1; }
    } else {
        if (x >= high) d->armed = 1;
        if (d->armed && x <= low) { d->fired = 1; return 1; }
    }
    return 0;
}

/* Returns the zero-based index of the first detection, or -1 if none. */
static int first_detection(const int *values, size_t count, Direction dir, int low, int high)
{
    Detector d;
    size_t i;
    begin(&d, dir);
    for (i = 0; i < count; ++i) {
        if (sample(&d, values[i], low, high)) return (int)i;
    }
    return -1;
}

static unsigned total, failed;
static void expect(const char *name, int got, int wanted)
{
    int ok = got == wanted;
    ++total;
    if (!ok) ++failed;
    printf("%s: %s (got %d, expected %d)\n", name, ok ? "PASS" : "FAIL", got, wanted);
}

int main(void)
{
    static const int noisy[] = {-5, -1, 1, -1, -2, -1, 1, 3};
    static const int falling[] = {5, 1, -1, 1, -3};
    static const int small[] = {-3, -1, 1};
    static const int limits[] = {-2, 0, 2};
    static const int no_start[] = {0, 1, 3};
    static const int large_noise[] = {-5, 3, -4};
    Detector d;
    expect("sign comparison", first_detection(noisy, 8, RISING, -1, 1), 2);
    expect("threshold comparison", first_detection(noisy, 8, RISING, -2, 2), 7);
    expect("falling threshold", first_detection(falling, 5, FALLING, -2, 2), 4);
    expect("small signal no detection", first_detection(small, 3, RISING, -2, 2), -1);
    expect("boundary inclusive", first_detection(limits, 3, RISING, -2, 2), 2);
    expect("no starting side", first_detection(no_start, 3, RISING, -2, 2), -1);
    expect("large noise still triggers", first_detection(large_noise, 3, RISING, -2, 2), 1);
    begin(&d, RISING);
    (void)sample(&d, -3, -2, 2);
    expect("first detection", sample(&d, 3, -2, 2), 1);
    (void)sample(&d, -3, -2, 2);
    expect("only once per step", sample(&d, 3, -2, 2), 0);
    begin(&d, RISING);
    expect("reset clears old arm", sample(&d, 3, -2, 2), 0);
    printf("%u/%u passed\n", total - failed, total);
    return failed ? 1 : 0;
}
