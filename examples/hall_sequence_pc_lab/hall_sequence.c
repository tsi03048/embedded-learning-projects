#include <stdio.h>
#include <stddef.h>

/* A:B:C = bit 2:bit 1:bit 0. This order is only a teaching example. */
static const unsigned char sequence[6] = {6, 4, 5, 1, 3, 2};

typedef enum { INVALID, INITIAL, UNCHANGED, FORWARD, REVERSE, SKIPPED } Result;

static int index_of(int value)
{
    int i;
    for (i = 0; i < 6; ++i) {
        if (sequence[i] == value) return i;
    }
    return -1;
}

/* Compare two samples; -1 means no previous sample exists. */
static Result classify(int previous, int current)
{
    int old_index = index_of(previous);
    int new_index = index_of(current);
    if (new_index < 0) return INVALID;
    if (previous == -1) return INITIAL;
    if (old_index < 0) return INVALID;
    if (old_index == new_index) return UNCHANGED;
    if ((old_index + 1) % 6 == new_index) return FORWARD;
    if ((old_index + 5) % 6 == new_index) return REVERSE;
    return SKIPPED;
}

static const char *name_of(Result r)
{
    static const char *names[] = {"INVALID", "INITIAL", "UNCHANGED", "FORWARD", "REVERSE", "SKIPPED"};
    return names[r];
}

int main(void)
{
    struct Case { int old_value, new_value; Result expected; };
    static const struct Case cases[] = {
        {-1, 6, INITIAL}, {6, 4, FORWARD}, {2, 6, FORWARD},
        {6, 2, REVERSE}, {4, 6, REVERSE}, {6, 6, UNCHANGED},
        {6, 5, SKIPPED}, {6, 0, INVALID}, {6, 7, INVALID},
        {-1, 7, INVALID}, {0, 6, INVALID}, {6, 8, INVALID}
    };
    size_t i;
    unsigned failures = 0;
    for (i = 0; i < sizeof cases / sizeof cases[0]; ++i) {
        Result actual = classify(cases[i].old_value, cases[i].new_value);
        int ok = actual == cases[i].expected;
        if (!ok) ++failures;
        printf("%02u  %s  got=%s expected=%s\n", (unsigned)(i + 1),
               ok ? "PASS" : "FAIL", name_of(actual), name_of(cases[i].expected));
    }
    printf("%u/%u passed\n", (unsigned)(sizeof cases / sizeof cases[0]) - failures,
           (unsigned)(sizeof cases / sizeof cases[0]));
    return failures ? 1 : 0;
}
