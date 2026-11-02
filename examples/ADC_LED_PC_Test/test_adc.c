#include <stdint.h>
#include <stdio.h>
#define GROUP_SIZE 8u
#define LED_ON_THRESHOLD 2200u
#define LED_OFF_THRESHOLD 1900u
#define ADC_OK 1u
#define ADC_CONVERSION_TIMEOUT 6u
static uint32_t sum, count, g_average_count, g_average_valid, g_led_on, g_status, failed;
static uint16_t g_adc_raw, g_average_raw;
/* PC replacement only: records the requested LED state, no GPIO. */
static void led_set(uint32_t on) { g_led_on = on; }
static void reset_model(void) {
    sum=0; count=0; g_average_count=0; g_average_valid=0;
    g_led_on=0; g_status=0; failed=0; g_adc_raw=0; g_average_raw=0;
}
static void feed(uint16_t raw) {
    if (failed) return;
    g_adc_raw = raw;
        sum += g_adc_raw;
        ++count;
        if (count == GROUP_SIZE) {
            g_average_valid = 0u;
            g_average_raw = (uint16_t)(sum / GROUP_SIZE);
            ++g_average_count;
            if (g_average_raw >= LED_ON_THRESHOLD) led_set(1u);
            else if (g_average_raw <= LED_OFF_THRESHOLD) led_set(0u);
            g_average_valid = 1u;
            sum = 0u;
            count = 0u;
        }
        g_status = ADC_OK;
}
static void inject_timeout(void) {
    if (failed) return;
    g_average_valid = 0u; led_set(0u); g_status = ADC_CONVERSION_TIMEOUT;
    failed=1;
}
static void group(uint16_t raw) {
    unsigned i; for (i=0; i<8; ++i) feed(raw);
}
static int check(int condition, const char *name) {
    printf("%s: %s\n", condition ? "PASS" : "FAIL", name);
    return condition ? 0 : 1;
}
int main(void) {
    unsigned i; int errors=0;
    reset_model();
    for (i=0; i<7; ++i) feed(2200);
    errors+=check(!g_average_valid && !g_led_on && g_average_count==0, "first seven samples");
    feed(2200);
    errors+=check(g_average_valid && g_average_raw==2200 && g_led_on && g_average_count==1, "upper boundary ON");
    group(2199);
    errors+=check(g_led_on && g_average_raw==2199, "hold ON in middle");
    group(1900);
    errors+=check(!g_led_on && g_average_raw==1900, "lower boundary OFF");
    group(1901);
    errors+=check(!g_led_on && g_average_raw==1901, "hold OFF in middle");
    group(4095);
    errors+=check(g_led_on && g_average_raw==4095, "maximum input");
    group(0);
    errors+=check(!g_led_on && g_average_raw==0 && g_average_valid, "zero is valid");
    reset_model(); group(2200); feed(4095); inject_timeout();
    errors+=check(!g_average_valid && !g_led_on && g_status==ADC_CONVERSION_TIMEOUT, "timeout invalidates and turns OFF");
    group(2200);
    errors+=check(!g_average_valid && !g_led_on, "late samples ignored after failure");
    printf("Result: %d failure(s)\n", errors);
    return errors ? 1 : 0;
}
