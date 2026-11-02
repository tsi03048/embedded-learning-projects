#ifndef REPORT_FORMAT_H
#define REPORT_FORMAT_H
#include <stdint.h>
/* Caller supplies a21-byte writable array and raw0..4095, alarm0/1. */
void format_report(uint8_t report[], uint32_t raw, uint32_t alarm);
#endif
