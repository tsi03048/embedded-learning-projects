#include <stdio.h>
#include <stddef.h>
#include "judge.h"

/* This file supplies fake events and checks the decision results on a PC. */
typedef struct { Event event; int value; State expected; const char *label; } Step;

int main(void)
{
    static const Step steps[] = {
        {START, 0, WAIT, "start"},
        {SAMPLE, -3, WAIT, "below threshold"},
        {SAMPLE, 1, WAIT, "near threshold"},
        {SAMPLE, 3, RUN, "crossing"},
        {TIMEOUT, 0, FAULT, "timeout"},
        {RESET, 0, FAULT, "recovery denied"},
        {RESET, 1, IDLE, "recovery allowed"},
        {START, 0, WAIT, "separate restart"}
    };
    Judge j;
    size_t i;
    unsigned failures = 0;
    judge_init(&j);
    for (i = 0; i < sizeof steps / sizeof steps[0]; ++i) {
        State actual = judge_event(&j, steps[i].event, steps[i].value);
        int ok = actual == steps[i].expected;
        if (!ok) ++failures;
        printf("%s: %s (got %d, expected %d)\n", steps[i].label,
               ok ? "PASS" : "FAIL", (int)actual, (int)steps[i].expected);
    }
    printf("%u/%u passed\n", (unsigned)(sizeof steps / sizeof steps[0]) - failures,
           (unsigned)(sizeof steps / sizeof steps[0]));
    return failures ? 1 : 0;
}
