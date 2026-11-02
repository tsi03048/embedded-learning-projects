#ifndef JUDGE_H
#define JUDGE_H

/* Contract shared by the PC input file and the decision file. */
typedef enum { IDLE, WAIT, RUN, FAULT } State;
typedef enum { START, SAMPLE, TIMEOUT, RESET } Event;
typedef struct { State state; int saw_negative; } Judge;

void judge_init(Judge *j);
State judge_event(Judge *j, Event event, int value);

#endif
