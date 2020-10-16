#include "rtc.h"
#include "i8259.h"
#include "lib.h"

void init_rtc() {
  uint8_t prev;

  enable_irq(RTC_IRQ);

  /* Save old values */
  outb(0x8B, RTC_SEL_PORT);
  prev = inb(RTC_DATA_PORT);

  outb(0x8B, RTC_SEL_PORT);
  outb(prev | 0x40, RTC_DATA_PORT);

  set_freq_rtc(HZ1024);
}

void irqh_rtc() {
  send_eoi(RTC_IRQ);
  sti();

  printf("RTC Event\n");
}

void set_freq_rtc(RTCFreq const freq) {
  uint8_t prev;

  outb(0x8A, RTC_SEL_PORT);
  prev = inb(RTC_DATA_PORT);

  outb(0x8A, RTC_SEL_PORT);
  outb((prev & 0xF0) | (uint8_t)freq, RTC_DATA_PORT);
}
