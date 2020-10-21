#include "rtc.h"
#include "i8259.h"
#include "lib.h"
#include "options.h"

/* init_rtc
 * Description: Initializes the real-time clock.
 * Inputs: None
 * Return Value: None
 * Side Effects: Writes to RTC ports, enables RTC IRQ.
 */
void init_rtc() {
  uint8_t prev;

  enable_irq(RTC_IRQ);

  /* Turn on periodic interupts */

  /* Select register B */
  outb(RTC_REG_B | RTC_DIS_NMI, RTC_SEL_PORT);
  /* Read its old value (we may be able to drop this reliably since we own init) */
  prev = inb(RTC_DATA_PORT);

  /* Reselect register B to avoid resetting index */
  outb(RTC_REG_B | RTC_DIS_NMI, RTC_SEL_PORT);
  outb(prev | PIE_MASK, RTC_DATA_PORT);

  /* Set to max frequency since we virtualize the interrupts */
  set_freq_rtc(HZ2);

  /* TODO: We may want to renable NMI's here, see OSDev */
}

/* irqh_rtc
 * Description: Handles an interrupt from the RTC.
 * Inputs: None
 * Outputs: None
 * Return Value: None
 * Side Effects: Writes to RTC ports, sends EOI.
 */
void irqh_rtc() {
  ack_rtc_int();

#if RTC_RANDOM_TEXT
  test_interrupts();
#endif

  send_eoi(RTC_IRQ);
}

/* ack_rtc_int
 * Description: ACKs an RTC interrupt.
 * Inputs: None
 * Outputs: None
 * Return Value: None
 * Side Effects: RTC port I/O.
 */
uint8_t ack_rtc_int() {
  /* Empty the buffer by reading details from reg C */
  outb(RTC_REG_C, RTC_SEL_PORT);
  return inb(RTC_DATA_PORT);
}

/* set_freq_rtc
 * Description: Sets the frequency of the RTC's interrupts.
 * Inputs: freq -- Desired frequency
 * Outputs: None
 * Return Value: None
 * Side Effects: Writes to RTC ports.
 */
void set_freq_rtc(RTCFreq const freq) {
  uint8_t prev;

  outb(RTC_REG_A | RTC_DIS_NMI, RTC_SEL_PORT);
  prev = inb(RTC_DATA_PORT);

  outb(RTC_REG_A | RTC_DIS_NMI, RTC_SEL_PORT);
  /* Set rate via the bottom 4 bits */
  outb((prev & 0xF0) | (uint8_t)freq, RTC_DATA_PORT);
}
