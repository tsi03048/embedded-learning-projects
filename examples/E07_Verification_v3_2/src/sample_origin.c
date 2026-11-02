#include <stdio.h>

/* Deterministic teaching model: one outstanding request, no hardware ADC. */
typedef enum { U, V, W } Phase;
typedef struct { unsigned generation; Phase phase; } Context;
typedef struct { Context origin; int pending; } Request;
typedef struct { Context origin; int value; } Reading;

static void request_start(Request *request, Context origin)
{
    request->origin = origin;
    request->pending = 1;
}

static int complete(Request *request, int value, Reading *result)
{
    if (!request->pending) return 0;
    result->origin = request->origin; /* preserve the modeled request origin */
    result->value = value;
    request->pending = 0;
    return 1;
}

static int matches(Reading result, Context current)
{
    return result.origin.generation == current.generation && result.origin.phase == current.phase;
}

static unsigned total, failures;
static void check(const char *label, int ok)
{
    ++total;
    if (!ok) ++failures;
    printf("%s: %s\n", label, ok ? "PASS" : "FAIL");
}

int main(void)
{
    Context step1 = {1, W}, step2 = {2, V}, later_w = {3, W};
    Request request = {{0, U}, 0};
    Reading correct, wrong;

    request_start(&request, step1);
    check("request created", request.pending);
    check("completion succeeds", complete(&request, -3, &correct));
    check("no duplicate completion", !complete(&request, 3, &wrong));
    wrong.origin = step2; wrong.value = correct.value; /* incorrect labeling at arrival */
    check("wrong label matches current", matches(wrong, step2));
    check("wrong label hides origin", wrong.origin.generation != correct.origin.generation);
    check("preserved origin rejects old result", !matches(correct, step2));
    check("same phase later still rejected", !matches(correct, later_w));
    request_start(&request, step2);
    check("new result accepted", complete(&request, 3, &correct) && matches(correct, step2));
    printf("%u/%u checks passed\n", total - failures, total);
    return failures ? 1 : 0;
}
