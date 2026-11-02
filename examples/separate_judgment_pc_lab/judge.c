#include "judge.h"

/* Teaching rules; input values are synthetic and outputs are only states. */
void judge_init(Judge *j)
{
    j->state = IDLE;
    j->saw_negative = 0;
}

State judge_event(Judge *j, Event event, int value)
{
    if (j->state == FAULT) {
        if (event == RESET && value == 1) judge_init(j);
        return j->state;
    }
    if (j->state == IDLE && event == START) {
        j->state = WAIT;
        j->saw_negative = 0;
    } else if ((j->state == WAIT || j->state == RUN) && event == TIMEOUT) {
        j->state = FAULT;
    } else if (j->state == WAIT && event == SAMPLE) {
        if (value <= -2) j->saw_negative = 1;
        if (j->saw_negative && value >= 2) j->state = RUN;
    }
    return j->state;
}
