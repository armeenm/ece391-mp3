#ifndef RTC_H
#define RTC_H

#include "lib.h"
#include "terminal_driver.h"

#define RTC_SEL_PORT 0x70
#define RTC_DATA_PORT 0x71
#define RTC_IRQ 0x8
#define RTC_BASE_FREQ 32768

#define RTC_REG_A 0xA
#define RTC_REG_B 0xB
#define RTC_REG_C 0xC

/* http://www.walshcomptech.com/ohlandl/config/cmos_bank_0.html#Hex_000 */

#define PIE_MASK (1 << 6)
#define RTC_DIS_NMI (1 << 7)
#define TOP_BYTE_NIBBLE 0xF0

#define PID_COUNT 8

typedef enum RTCRate { HZ1024 = 0x6, HZ512, HZ256, HZ128, HZ64, HZ32, HZ16, HZ8, HZ4, HZ2 } RTCRate;


void init_rtc(void);
void irqh_rtc(void);
void set_real_freq_rtc(RTCRate const rate);
int set_virtual_freq_rtc(u32 freq);
u32 ack_rtc_int(void);

// System calls
i32 rtc_read(i32 fd, void* buf, i32 nbytes);
i32 rtc_write(i32 fd, const void* buf, i32 nbytes);
i32 rtc_open(const u8* filename);
i32 rtc_close(i32 fd);
#endif
