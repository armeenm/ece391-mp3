#include "rtc.h"
#include "i8259.h"
#include "lib.h"

void init_rtc() {
  uint8_t prev;

  enable_irq(RTC_IRQ);

  cli();
  // Turn on periodic interupts
  outb(RTC_REG_B | RTC_DIS_NMI, RTC_SEL_PORT); // Select register B
  prev = inb(RTC_DATA_PORT); // Read it's old value (we may be able to drop this reliably since we own init)

  outb(RTC_REG_B | RTC_DIS_NMI, RTC_SEL_PORT); // Reselect register B to avoid reseting index
  outb(prev | BIT_6_MASK, RTC_DATA_PORT); // turn on bit 6 of reg B for periodic ints enable (PIE)

  set_freq_rtc(HZ2);
  sti(); // we may want to renable NMI's here, see OSDEV
}

void irqh_rtc() {
  send_eoi(RTC_IRQ);
  sti();

  printf("RTC Event \n");
  ack_rtc_int();
}

char ack_rtc_int() {
  outb(RTC_REG_C, RTC_SEL_PORT); // We need to read register C to permit multiple interupts
  return inb(RTC_DATA_PORT); // if we want, this gives us back int details
}

void set_freq_rtc(RTCFreq const freq) {
  uint8_t prev;

  outb(RTC_REG_A | RTC_DIS_NMI, RTC_SEL_PORT);
  prev = inb(RTC_DATA_PORT); // pull the old value of reg A

  outb(RTC_REG_A | RTC_DIS_NMI, RTC_SEL_PORT); // reselect reg A
  outb((prev & 0xF0) | (uint8_t)freq, RTC_DATA_PORT); // set rate in the bottom 4 bits
}
