#ifndef RTC_H
#define RTC_H

#include "lib.h"

#define RTC_SEL_PORT 0x70
#define RTC_DATA_PORT 0x71
#define RTC_IRQ 0x8

#define RTC_REG_A 0xA
#define RTC_REG_B 0xB
#define RTC_REG_C 0xC

/* http://www.walshcomptech.com/ohlandl/config/cmos_bank_0.html#Hex_000 */

#define PIE_MASK (1 << 6)
#define RTC_DIS_NMI (1 << 7)

typedef enum RTCFreq { HZ1024 = 0x6, HZ512, HZ256, HZ128, HZ64, HZ32, HZ16, HZ8, HZ4, HZ2 } RTCFreq;

void init_rtc(void);
void irqh_rtc(void);
void set_freq_rtc(RTCFreq freq);
uint8_t ack_rtc_int(void);

#endif
