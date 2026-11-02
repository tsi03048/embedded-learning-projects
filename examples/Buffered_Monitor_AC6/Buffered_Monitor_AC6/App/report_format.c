#include "report_format.h"

void format_report(uint8_t report[], uint32_t raw, uint32_t alarm)
{
    report[7]=(uint8_t)('0'+raw/1000u);
    report[8]=(uint8_t)('0'+(raw/100u)%10u);
    report[9]=(uint8_t)('0'+(raw/10u)%10u);
    report[10]=(uint8_t)('0'+raw%10u);
    if (alarm != 0u) { report[18]='W'; }
    else { report[18]='N'; }
}
