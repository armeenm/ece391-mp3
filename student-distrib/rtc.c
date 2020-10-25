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
  set_real_freq_rtc(HZ1024);
  set_virtual_freq_rtc(RTC_DEFAULT_VIRT_FREQ);
  virtual_rtc_instance.int_count = 0;
  virtual_rtc_instance.flag = 0;
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
  virtual_rtc_instance.int_count++;
  // We only set the flag to indicate a virtualized interrupt occured
  virtual_rtc_instance.flag = (virtual_rtc_instance.real_freq/virtual_rtc_instance.int_count == virtual_rtc_instance.virt_freq);
  if (virtual_rtc_instance.flag) {
    virtual_rtc_instance.int_count = 0;
  }
#if RTC_RANDOM_TEXT
  test_interrupts();
#endif

  send_eoi(RTC_IRQ);
}

int32_t rtc_read(void* buf, int32_t nbytes) {
  while (!virtual_rtc_instance.flag) {
    //spin while we wait for flag to be set by IRQh
  }
  // reset the flag so future reads require a future IRQH set
  virtual_rtc_instance.flag = 0;
  return 0;
}


int32_t rtc_write(const void* buf, int32_t nbytes) {
  // try to set the freq, if it's not valid, this returns -1 and it's failed
  if (set_virtual_freq_rtc(*(uint32_t*)buf)) {
    return -1;
  }
  return sizeof(uint32_t);
}

int32_t rtc_open(const uint8_t* filename) {
  set_virtual_freq_rtc(RTC_DEFAULT_VIRT_FREQ);
  return 0;
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
 * Inputs: rate -- Desired frequency in terms of rate
 * Outputs: None
 * Return Value: None
 * Side Effects: Writes to RTC ports.
 */
void set_real_freq_rtc(RTCRate const rate) {
  uint8_t prev;

  outb(RTC_REG_A | RTC_DIS_NMI, RTC_SEL_PORT);
  prev = inb(RTC_DATA_PORT);

  outb(RTC_REG_A | RTC_DIS_NMI, RTC_SEL_PORT);
  /* Set rate via the bottom 4 bits */
  outb((prev & 0xF0) | (uint8_t)rate, RTC_DATA_PORT);

  // This is formula to derivce frequency from rate
  virtual_rtc_instance.real_freq = RTC_BASE_FREQ >> (rate - 1);
}


int set_virtual_freq_rtc(uint32_t freq) {
  // if freq = 0 or > RTC freq or it's not a power of 2, this in an invalid frequency
  if (freq == 0 || freq > virtual_rtc_instance.real_freq || (freq & (freq - 1)) != 0) {
    return -1;
  }
  virtual_rtc_instance.int_count = 0;
  virtual_rtc_instance.virt_freq = freq;
  return 0;
}
