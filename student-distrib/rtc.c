#include "rtc.h"
#include "i8259.h"
#include "lib.h"

void init_rtc() { enable_irq(RTC_IRQ); }

void irqh_rtc() {
  cli();
  disable_irq(RTC_IRQ);
  send_eoi(RTC_IRQ);
  sti();

  printf("RTC Event\n");
  test_interrupts();

  enable_irq(RTC_IRQ);
}
