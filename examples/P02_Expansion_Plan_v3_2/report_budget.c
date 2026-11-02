#include <stdio.h>

int main(void)
{
    const char p01[] = "raw=2600,state=W\r\n";
    const char e05[] = "D1,raw=2600,state=W\r\n";
    const char proposed[] = "D1,seq=104,tick=12400,raw=2600,state=W\r\n";
    const char proposed_max[] = "D1,seq=4294967295,tick=4294967295,raw=4095,state=W\r\n";
    unsigned bytes;
    double wire_ms;
    double link_percent;

    /* ASCII, 9600 baud, 8N1 = 10 transmitted bits per byte.
       A 100 ms report interval is an assumed planning input.
       This program does not send UART data or execute MCU firmware. */
    bytes = (unsigned)(sizeof p01 - 1U);
    wire_ms = (double)bytes * 10.0 * 1000.0 / 9600.0;
    link_percent = wire_ms / 100.0 * 100.0;
    printf("P01: bytes=%u, wire_ms=%.3f, link_percent=%.3f\n",
           bytes, wire_ms, link_percent);

    bytes = (unsigned)(sizeof e05 - 1U);
    wire_ms = (double)bytes * 10.0 * 1000.0 / 9600.0;
    link_percent = wire_ms / 100.0 * 100.0;
    printf("E05: bytes=%u, wire_ms=%.3f, link_percent=%.3f\n",
           bytes, wire_ms, link_percent);

    bytes = (unsigned)(sizeof proposed - 1U);
    wire_ms = (double)bytes * 10.0 * 1000.0 / 9600.0;
    link_percent = wire_ms / 100.0 * 100.0;
    printf("PROPOSED: bytes=%u, wire_ms=%.3f, link_percent=%.3f\n",
           bytes, wire_ms, link_percent);

    bytes = (unsigned)(sizeof proposed_max - 1U);
    wire_ms = (double)bytes * 10.0 * 1000.0 / 9600.0;
    link_percent = wire_ms / 100.0 * 100.0;
    printf("PROPOSED_MAX: bytes=%u, wire_ms=%.3f, link_percent=%.3f\n",
           bytes, wire_ms, link_percent);
    printf("PROPOSED_MAX C string storage: %u bytes including NUL\n",
           (unsigned)sizeof proposed_max);
    return 0;
}
