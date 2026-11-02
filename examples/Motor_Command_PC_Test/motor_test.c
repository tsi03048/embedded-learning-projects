#include <stdio.h>

typedef enum { STOP, FORWARD, REVERSE } Direction;
typedef struct {
    Direction direction;
    unsigned duty_percent;
} MotorRequest;

/* Logical request only. This function does not drive a real motor. */
static int make_request(int direction, int duty, MotorRequest *out)
{
    out->direction = STOP;
    out->duty_percent = 0u;
    if (direction != STOP && direction != FORWARD && direction != REVERSE)
        return 0;
    if (duty < 0 || duty > 100)
        return 0;
    if (direction != STOP && duty != 0) {
        out->direction = (Direction)direction;
        out->duty_percent = (unsigned)duty;
    }
    return 1;
}
static int check(int ok, const char *name)
{
    printf("%s: %s\n", ok ? "PASS" : "FAIL", name);
    return ok ? 0 : 1;
}
int main(void)
{
    MotorRequest out;
    int ok, errors = 0;
    ok = make_request(FORWARD, 50, &out);
    errors += check(ok && out.direction == FORWARD && out.duty_percent == 50u, "forward 50");
    ok = make_request(REVERSE, 25, &out);
    errors += check(ok && out.direction == REVERSE && out.duty_percent == 25u, "reverse 25");
    ok = make_request(STOP, 80, &out);
    errors += check(ok && out.direction == STOP && out.duty_percent == 0u, "stop ignores duty");
    ok = make_request(FORWARD, 0, &out);
    errors += check(ok && out.direction == STOP && out.duty_percent == 0u, "zero duty becomes stop");
    ok = make_request(REVERSE, 100, &out);
    errors += check(ok && out.direction == REVERSE && out.duty_percent == 100u, "maximum duty accepted");
    ok = make_request(FORWARD, 101, &out);
    errors += check(!ok && out.direction == STOP && out.duty_percent == 0u, "duty above 100 rejected");
    ok = make_request(REVERSE, -1, &out);
    errors += check(!ok && out.direction == STOP && out.duty_percent == 0u, "negative duty rejected");
    ok = make_request(99, 50, &out);
    errors += check(!ok && out.direction == STOP && out.duty_percent == 0u, "unknown direction rejected");
    printf("Result: %d failure(s)\n", errors);
    return errors ? 1 : 0;
}
