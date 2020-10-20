#include "keyboard.h"
#include "i8259.h"
#include "lib.h"

/* init_keyboard
 * Description: Initializes the keyboard
 * Inputs: None
 * Return Value: None
 * Side Effects: Enables the keyboard IRQ.
 */
void init_keyboard(void) { enable_irq(KEYBOARD_IRQ); }

/* irqh_keyboard
 * Description: Takes care of typed keys.
 * Inputs: None
 * Return Value: None
 * Side Effects: Safely disables keyboard to avoid multi-input and prints
 *               the typed key to the virtual machine window.
 */
void irqh_keyboard(void) {
  if (inb(KEYBOARD_STATUS_PORT) & KEYBOARD_OUTBUF_FULL)
    handle_keypress(inb(KEYBOARD_DATA_PORT));

  send_eoi(KEYBOARD_IRQ);
}

/* handle_keypress
 * Description: Puts a scancode on the screen
 * Inputs: scancode -- Scancode to use
 * Outputs: None
 * Return Value: None
 * Side Effects: Writes to the video buffer
 */
void handle_keypress(SCSet1 const scancode) {
  if (scancode > 0 && scancode < SCS1_PRESSED_F12) {
    char const disp = keycodes[scancode];
    if (disp)
      putc(disp);
  }
}
