#ifndef RTC_H
#define RTC_H

#define RTC_IRQ 0x8

void init_rtc(void);
void irqh_rtc(void);

#endif
