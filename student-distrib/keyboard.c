#include "keyboard.h"
#include "i8259.h"
#include "keycodes.h"
#include "lib.h"

char keycodes[SCS1_PRESSED_F12] = {[KEY_1] = '1',
                                   '2',
                                   '3',
                                   '4',
                                   '5',
                                   '6',
                                   '7',
                                   '8',
                                   '9',
                                   '0',
                                   '-',
                                   '=',
                                   10,
                                   '\t',
                                   'q',
                                   'w',
                                   'e',
                                   'r',
                                   't',
                                   'y',
                                   'u',
                                   'i',
                                   'o',
                                   'p',
                                   '[',
                                   ']',
                                   '\n',
                                   [KEY_A] = 'a',
                                   's',
                                   'd',
                                   'f',
                                   'g',
                                   'h',
                                   'j',
                                   'k',
                                   'l',
                                   ';',
                                   '\'',
                                   '`',
                                   [KEY_BACKSLASH] = '\\',
                                   'z',
                                   'x',
                                   'c',
                                   'v',
                                   'b',
                                   'n',
                                   'm',
                                   ',',
                                   '.',
                                   '/',
                                   [KEY_KPASTERISK] = '*',
                                   [KEY_SPACE] = ' ',
                                   '7',
                                   '8',
                                   '9',
                                   '-',
                                   '4',
                                   '5',
                                   '6',
                                   '+',
                                   '1',
                                   '2',
                                   '3',
                                   '0',
                                   '.'};

/* void init_keyboard(void);
 * Description: Initializes the keyboard
 * Inputs: none
 * Return Value: none
 * Function: Enables the keyboard irq.
*/
void init_keyboard(void) { enable_irq(KEYBOARD_IRQ); }

/* void irqh_keyboard(void);
 * Description: Takes care of typed keys.
 * Inputs: none
 * Return Value: none
 * Function: Safely disables keyboard to avoid multi-input and prints
 *           the typed key to the virtual machine window.
*/
void irqh_keyboard(void) {
  send_eoi(KEYBOARD_IRQ);
  sti();

  if (inb(KEYBOARD_STATUS_PORT) & KEYBOARD_OUTBUF_FULL)
    handle_keypress(inb(KEYBOARD_DATA_PORT));
}

/* void handle_keypress(SCSet1 const scancode);
 * Description: 
 * Inputs: none
 * Return Value: none
 * Function: 
*/
void handle_keypress(SCSet1 const scancode) {
  if (scancode > 0 && scancode < SCS1_PRESSED_F12) {
    char const disp = keycodes[scancode];
    if (disp)
      putc(disp);
  }
}
