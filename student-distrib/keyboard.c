#include "keyboard.h"
#include "i8259.h"
#include "lib.h"

/* lshift, rshift, lcontrol, rcontrol, capslock, backspace*/
 uint8_t key_state[SCS1_PRESSED_F12];


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

    key_state[scancode] = 1;

    char disp = handle_disp(keycodes[scancode]);

    if((key_state[SCS1_PRESSED_LEFTCTRL] == 1 && scancode == SCS1_PRESSED_L) || (key_state[SCS1_PRESSED_L] == 1 && scancode == SCS1_PRESSED_LEFTCTRL))
    {
      clear();
      set_screen_x(0);
      set_screen_y(0);
    }
      
    else if (disp)
      putc(disp);
  }
  else if (scancode > 0x81 && scancode <= SCS1_RELEASED_F12) {
    key_state[scancode - 0x80] = 0;
  }
}

char handle_disp(char disp)
{
  if(key_state[SCS1_PRESSED_LEFTSHIFT] == 1 || key_state[SCS1_PRESSED_RIGHTSHIFT] == 1)
  {
    if(disp >= 'a' && disp <= 'z')
    {
      disp -= 0x20;
    }
    else
    {
      switch(disp)
      {
        case '1':
          disp = '!';
        break;
        case '2':
          disp = '@';
        break;
        case '3':
          disp = '#';
        break;
        case '4':
          disp = '$';
        break;
        case '5':
          disp = '%';
        break;
        case '6':
          disp = '^';
        break;
        case '7':
          disp = '&';
        break;
        case '8':
          disp = '*';
        break;
        case '9':
          disp = '(';
        break;
        case '0':
          disp = ')';
        break;
        case '`':
          disp = '~';
        break;
        case '-':
          disp = '_';
        break;
        case '=':
          disp = '+';
        break;
        case ';':
          disp = ':';
        break;
        case '\'':
          disp = '\"';
        break;
        case ',':
          disp = '<';
        break;
        case '.':
          disp = '>';
        break;
        case '/':
          disp = '?';
        break;
        case '[':
          disp = '{';
        break;
        case ']':
          disp = '}';
        break;
        case '\\':
          disp = '|';
        break;
        default:
        break;
      }
    }
      
  }
      
  

  return disp;
}
