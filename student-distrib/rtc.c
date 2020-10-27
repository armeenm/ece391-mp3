#include "rtc.h"
#include "i8259.h"
#include "lib.h"
#include "options.h"

// Global to hold RTC state for virtualization
static virtual_rtc virtual_rtc_instance;

/* init_rtc
 * Description: Initializes the real-time clock.
 * Inputs: None
 * Return Value: None
 * Side Effects: Writes to RTC ports, enables RTC IRQ.
 */
void init_rtc(void) {
  u32 prev;

  enable_irq(RTC_IRQ);

  /* Turn on periodic interupts */

  /* Select register B */
  outb(RTC_REG_B | RTC_DIS_NMI, RTC_SEL_PORT);
  /* Read its old value (we may be able to drop this reliably since we own init) */
  prev = inb(RTC_DATA_PORT);

  /* Reselect register B to avoid resetting index */
  outb(RTC_REG_B | RTC_DIS_NMI, RTC_SEL_PORT);
  outb(prev | PIE_MASK, RTC_DATA_PORT);

  /* Set to "max" frequency since we virtualize the interrupts */

  set_real_freq_rtc(HZ1024);
  set_virtual_freq_rtc(RTC_DEFAULT_VIRT_FREQ);
  virtual_rtc_instance.int_count = 0;
  virtual_rtc_instance.flag = 0;
  /* TODO: We may want to renable NMI's here, see OSDev */
}

/* irqh_rtc
 * Description: Handles an interrupt from the RTC, update virtualized state
 * Inputs: None
 * Outputs: Raised flag based on whether virtual freq has been matched by interrupt count
 * Return Value: None
 * Side Effects: Reads from RTC port C to ack, sends EOI.
 */
void irqh_rtc(void) {
  ack_rtc_int();
  virtual_rtc_instance.int_count++;
  // We only set the flag to indicate a virtualized interrupt occured (eg 1024HZ <= 2HZ * 512 ints)
  virtual_rtc_instance.flag = (virtual_rtc_instance.real_freq <=
                               virtual_rtc_instance.virt_freq * virtual_rtc_instance.int_count);
  if (virtual_rtc_instance.flag) {
    virtual_rtc_instance.int_count = 0;
  }
#if RTC_RANDOM_TEXT_DEMO
  test_interrupts();
#endif

  send_eoi(RTC_IRQ);
}

/* rtc_read
 * Description: Spin while waiting for the IRQH to fire at virtual freq. Then reset that flag.
 * Inputs: i32 fd, void* buf, i32 nbytes (all ignored)
 * Outputs: resets the virtual flag to 0
 * Return Value: i32 (always 0)
 * Side Effects: Blocks exectution of process while waiting for int to occur
 */
i32 rtc_read(i32 UNUSED(fd), void* UNUSED(buf), i32 UNUSED(nbytes)) {
  while (!virtual_rtc_instance.flag) {
    // spin while we wait for flag to be set by IRQH (avoid gcc warnings with this comment)
  }
  // reset the flag so future reads require a future IRQH set
  virtual_rtc_instance.flag = 0;
  return 0;
}

/* rtc_write
 * Description: Set the virtual frequency from buffer if it is a vliad poweer of 2 freq.
 * Inputs: i32 fd, void* buf (the pointer to an int holding frequency requested), i32 nbytes
 * (can only be 4 bytes) Outputs: none Return Value: i32, -1 on invalid freq, sizeof(int) when
 * working Side Effects: none
 */
i32 rtc_write(i32 UNUSED(fd), void const* buf, i32 const nbytes) {
  // TODO: VALIDATE buf location in memory to prevent ring 0 memory access
  // try to set the freq, if it's not valid, this returns -1 and it's failed

  if (!buf || nbytes != sizeof(u32))
    return -1;

  return (set_virtual_freq_rtc(*(u32 const*)buf)) ? -1 : (i32)sizeof(u32);
}

/* rtc_open
 * Description: reset the virtual frequency to 2HZ (default)
 * Inputs: iconst u8* filename (ignored)
 * Outputs: none
 * Return Value: i32, 0 always
 * Side Effects: none
 */
i32 rtc_open(const u8* UNUSED(filename)) {
  set_virtual_freq_rtc(RTC_DEFAULT_VIRT_FREQ);
  return 0;
}

/* rtc_close
 * Description: handles system call case, but does nothing
 * Inputs: none
 * Outputs: none
 * Return Value: 0
 * Side Effects: none
 */
i32 rtc_close(i32 UNUSED(fd)) { return 0; }

/* ack_rtc_int
 * Description: ACKs an RTC interrupt.
 * Inputs: None
 * Outputs: None
 * Return Value: None
 * Side Effects: RTC port I/O.
 */
u32 ack_rtc_int(void) {
  /* Empty the buffer by reading details from reg C */
  outb(RTC_REG_C, RTC_SEL_PORT);
  return inb(RTC_DATA_PORT);
}

/* set_real_freq_rtc
 * Description: Sets the frequency of the RTC's interrupts.
 * Inputs: rate -- Desired frequency in terms of rate
 * Outputs: None
 * Return Value: None
 * Side Effects: Writes to RTC ports.
 */
void set_real_freq_rtc(RTCRate const rate) {
  u32 prev;

  outb(RTC_REG_A | RTC_DIS_NMI, RTC_SEL_PORT);
  prev = inb(RTC_DATA_PORT);

  outb(RTC_REG_A | RTC_DIS_NMI, RTC_SEL_PORT);
  /* Set rate via the bottom 4 bits */
  outb((prev & TOP_BYTE_NIBBLE) | (u8)rate, RTC_DATA_PORT);

  // This is formula to derivce frequency from rate
  virtual_rtc_instance.real_freq = RTC_BASE_FREQ >> (rate - 1);
}

/* set_virtual_freq_rtc
 * Description: Sets the frequency of the RTC's interrupts (virtually)
 * Inputs: u32 freq: a power of 2 frequency that's less than the real freq of the RTC
 * Outputs: Modifies virtual_rtc_instance.
 * Return Value: int: -1 = invalid frequency, 0 = success
 * Side Effects: none
 */
int set_virtual_freq_rtc(u32 freq) {
  // if freq = 0 or > RTC freq or it's not a power of 2, this in an invalid frequency
  if (freq == 0 || freq > virtual_rtc_instance.real_freq || (freq & (freq - 1)) != 0) {
    return -1;
  }
  // We reset interrupt count to start our time-to-first-read constant
  virtual_rtc_instance.int_count = 0;
  virtual_rtc_instance.virt_freq = freq;
  return 0;
}
