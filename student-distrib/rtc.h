#ifndef RTC_H
#define RTC_H

#include "lib.h"

#define RTC_SEL_PORT 0x70
#define RTC_DATA_PORT 0x71
#define RTC_IRQ 0x8
#define RTC_BASE_FREQ 32768
#define RTC_DEFAULT_VIRT_FREQ 2 


#define RTC_REG_A 0xA
#define RTC_REG_B 0xB
#define RTC_REG_C 0xC

/* http://www.walshcomptech.com/ohlandl/config/cmos_bank_0.html#Hex_000 */

#define PIE_MASK (1 << 6)
#define RTC_DIS_NMI (1 << 7)
#define TOP_BYTE_NIBBLE 0xF0

typedef enum RTCRate { HZ1024 = 0x6, HZ512, HZ256, HZ128, HZ64, HZ32, HZ16, HZ8, HZ4, HZ2 } RTCRate;


typedef struct {
    uint32_t virt_freq; // must be a power of 2 <= 1024
    uint32_t real_freq; // The intial, real freq requested of RTC (1024)
    uint32_t int_count;
    volatile uint8_t flag;
} virtual_rtc;

void init_rtc(void);
void irqh_rtc(void);
void set_real_freq_rtc(RTCRate const rate);
int set_virtual_freq_rtc(uint32_t freq);
uint8_t ack_rtc_int(void);


// System calls
int32_t rtc_read(int32_t fd, void* buf, int32_t nbytes);
int32_t rtc_write(int32_t fd, const void* buf, int32_t nbytes);
int32_t rtc_open(const uint8_t* filename);
int32_t rtc_close(int32_t fd);
#endif
