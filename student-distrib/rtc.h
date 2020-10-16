#ifndef RTC_H
#define RTC_H

#define RTC_SEL_PORT 0x70
#define RTC_DATA_PORT 0x71
#define RTC_IRQ 0x8

typedef enum RTCFreq { HZ1024 = 0x6, HZ512, HZ256, HZ128, HZ64, HZ32, HZ16, HZ8, HZ4, HZ2 } RTCFreq;

void init_rtc(void);
void irqh_rtc(void);
void set_freq_rtc(RTCFreq freq);

#endif
