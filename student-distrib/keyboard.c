#include "keyboard.h"
#include "i8259.h"
#include "lib.h"
#include "terminal_driver.h"
#include "syscall.h"
/* Declare variables for keyboard */

u8 key_state[SCS1_PRESSED_F12];
u32 caps_lock_repeat = 0;
u32 multi_byte = 0;

/* contains_newline
 * Description: To determine if a newline is present in a buf
 * Inputs: buf - buf to write line buf to
 *         nbytes - number of bytes to read
 * Outputs: none
 * Return Value:  0 - the buf does not contain a newline
 *                1 - if the buf contains a newline
 * Function: To determine if \n or \r is present in a buf
 */
int contains_newline(i8 const* const buf, i32 const size) {
  i32 i;

  /* If the buf contains a newline, return its index */
  for (i = 0; i < size; ++i) {
    if (buf[i] == '\n')
      return i;
  }

  /* No newline is present - return -1 */
  return -1;
}

/* get_line_buf
 * Description: Get the line buf and copy it into a buf
 * Inputs: buf - buf that line_buf writes to
 *         nbytes - number of bytes to write
 * Outputs: buf is filled with the line_buf
 * Return Value: size of the buf
 * Function: Gets a linebuf, returns the size of it, and clears
 * the current line buf
 */
i32 get_line_buf(char* const buf, i32 const nbytes) {
  i32 lenstr;
  i32 nl_idx;
  terminal* term;
  if (nbytes <= 0 || !buf)
    return -1;

  term = get_current_terminal();
  if(!term)
    return -1;
  
  term->read_flag = 1;

  /* Wait while the line_buf does not contain a \n */
  while ((nl_idx = contains_newline(term->line_buf, LINE_BUFFER_SIZE)) == -1)
    ;

  {
    i32 const limit = MIN(nl_idx + 1, nbytes);

    /* Copy from the line buf to the buf */
    memcpy(buf, term->line_buf, (u32)limit);

    /* Set the size of the string */
    lenstr = limit;
  }

  /* Make sure that the last character is a newline */
  buf[MIN(nbytes, LINE_BUFFER_SIZE)] = '\n';

  /* clear the line buf and return the size of the buf written to */
  clear_line_buf();

  term->read_flag = 0;

  return lenstr;
}

/* init_keyboard
 * Description: Initializes the keyboard
 * Inputs: None
 * Return Value: None
 * Side Effects: Enables the keyboard IRQ.
 */
void init_keyboard(void) {
  /* Enable IRQ, clear multi-byte and caps-lock vars */
  enable_irq(KEYBOARD_IRQ);
  multi_byte = 0;
  caps_lock_repeat = 0;

  /* Clear the line buf */
  clear_line_buf();
}

/* irqh_keyboard
 * Description: Takes care of typed keys.
 * Inputs: None
 * Return Value: None
 * Side Effects: Safely disables keyboard to avoid multi-input and prints
 *               the typed key to the virtual machine window.
 */
void irqh_keyboard(void) {
  /* If a keypress is ready then handle it */
  if (inb(KEYBOARD_STATUS_PORT) & KEYBOARD_OUTBUF_FULL)
    handle_keypress(inb(KEYBOARD_DATA_PORT));

  /* Send EOI */
  send_eoi(KEYBOARD_IRQ);
}

/* handle_keypress
 * Description: Puts a scancode on the screen
 * Inputs: scancode -- Scancode to use
 * Outputs: None
 * Return Value: None
 * Side Effects: Writes to the video buf
 */
void handle_keypress(SCSet1 const scancode) {
  u32 i;
  terminal* term = get_current_terminal();

  if(!term)
    return;

  /* If scancode is multibyte set multibyte_flag to 1 */
  if (scancode == SCS1_MULTIBYTE)
    multi_byte = 1;

  else if (multi_byte) {
    /* Handle multi_byte keycodes here */
    multi_byte = 0;

  } else if (scancode > 0 && scancode < SCS1_PRESSED_F12) {
    /* Get the printable keycode if any */
    i8 const disp = handle_disp(keycodes[scancode]);

    /* If capslock is pressed and is not being held down NOT the key_state  */
    if (scancode == SCS1_PRESSED_CAPSLOCK && !caps_lock_repeat) {
      key_state[scancode] ^= 1;
      caps_lock_repeat = 1;
    } else {
      /* Otherwise the keystate is equal to 1 */
      key_state[scancode] = 1;
    }

    /* If the command ctrl + l is pressed clear the screen */
    if (key_state[SCS1_PRESSED_LEFTCTRL] && scancode == SCS1_PRESSED_L) {

      /* Clear screen and reset terminal */
      clear();
      set_screen_xy(0, 0);

      // Todo: let's not have the keyboard setup the screen again? maybe call out to shell?
      if (term->read_flag) {
        /* Write to terminal to put "thanOS> " in */
        terminal_write(0, SHELL_PS1, sizeof(SHELL_PS1));

        /* Write what's in the input buf */
        for (i = 0; i < term->line_buf_index; ++i)
          putc(term->line_buf[i]);
      }

    } else if (!term->read_flag) {
      return;
    } else if(is_func_key()) {
      if(scancode == SCS1_PRESSED_F1) {
        send_eoi(KEYBOARD_IRQ);
        switch_terminal(0);
      }
      else if(scancode == SCS1_PRESSED_F2) {
        send_eoi(KEYBOARD_IRQ);
        switch_terminal(1);
      }
      else if(scancode == SCS1_PRESSED_F3) {
        send_eoi(KEYBOARD_IRQ);
        switch_terminal(2);
      }
    } else if (key_state[SCS1_PRESSED_BACKSPACE] == 1) {
      /* If there is data in the line buf and backspace is pressed
       * decrement the line buf and handle the backspace keypress
       */
      if (term->line_buf_index > 0) {
        term->line_buf_index--;
        term->line_buf[term->line_buf_index] = 0;
        putc('\b');
      }
    }

    /* if there is a key to display */
    else if (disp) {

      /* If the linebuf is not full (127 characters,  LINE_BUFFER_SIZE - 1) handle keypress */
      if (term->line_buf_index < LINE_BUFFER_SIZE - 1) {
        /* print the key and put it in the buf */
        putc(disp);
        term->line_buf[term->line_buf_index] = disp;
        term->line_buf_index++;

      } else if (disp == '\n') {
        /* Handle newline, put it in the end of the buf, since line_buf_index >=
         * LINE_BUFFER_SIZE - 1 */
        putc(disp);
        term->line_buf[term->line_buf_index] = disp;
        term->line_buf_index++;
      }
    }

  }

  /* This section handles key releases */
  else if ((u32)scancode > SCS1_KEYPRESS_RELEASE_OFFSET && (u32)scancode <= SCS1_RELEASED_F12) {

    /* If capslock is released reset repeat to 0 so we can toggle it again */
    if (scancode == SCS1_RELEASED_CAPSLOCK) {
      caps_lock_repeat = 0;

    } else {
      /* The release scancode is SCS1_KEYPRESS_RELEASE_OFFSET greater its
       * pressed scancode. Set it to 0 to indicate it was released
       */
      key_state[scancode - SCS1_KEYPRESS_RELEASE_OFFSET] = 0;
    }
  }
}

/* handle_disp
 * Description: Handles a character and outputs the character that should print
 * Inputs: disp -- char to change if keystates indicate it
 * Outputs: None
 * Return Value: disp - the changed or unchanged value of the handled disp character
 * Side Effects: none
 */
i8 handle_disp(i8 disp) {
  /* If shift is pressed or capslock then print its shift/capslock modified value */
  if (shift_pressed() || (capslock_pressed() && disp >= 'a' && disp <= 'z')) {

    /* If the character is a-z capitalize it */
    if (disp >= 'a' && disp <= 'z' && !(capslock_pressed() && shift_pressed())) {
      /* an uppercase character is SCS1_UPPERCASE_OFFSET away from its lowercase value */
      disp -= SCS1_UPPERCASE_OFFSET;

    } else
      /* If the character should be modified by shift find which character to print */
      switch (disp) {
      case '1':
        return '!';
      case '2':
        return '@';
      case '3':
        return '#';
      case '4':
        return '$';
      case '5':
        return '%';
      case '6':
        return '^';
      case '7':
        return '&';
      case '8':
        return '*';
      case '9':
        return '(';
      case '0':
        return ')';
      case '`':
        return '~';
      case '-':
        return '_';
      case '=':
        return '+';
      case ';':
        return ':';
      case '\'':
        return '\"';
      case ',':
        return '<';
      case '.':
        return '>';
      case '/':
        return '?';
      case '[':
        return '{';
      case ']':
        return '}';
      case '\\':
        return '|';
      default:
        break;
      }
  }

  return disp;
}

/* clear_line_buf
 * Description: Clears the line buf
 * Inputs: none
 * Outputs: None
 * Return Value: none
 * Side Effects: Clears the line_buf and resets the index to 0
 */
void clear_line_buf(void) {
  /* Iterate through the entire line buf */
  u32 i;
  terminal * term = get_current_terminal();
  if(!term)
    return;
  /* Clear the line buf */
  for (i = 0; i < LINE_BUFFER_SIZE; ++i)
    term->line_buf[i] = 0;

  /* Set index to 0 */
  term->line_buf_index = 0;
}

/* shift_pressed
 * Description: Indicates whether shift is pressed or not
 * Inputs: none
 * Outputs: None
 * Return Value: 1 if shift pressed, 0 otherwise
 * Side Effects: none
 */
i32 shift_pressed(void) {
  return key_state[SCS1_PRESSED_LEFTSHIFT] | key_state[SCS1_PRESSED_RIGHTSHIFT];
}

/* capslock_pressed
 * Description: Indicates whether capslock is pressed or not
 * Inputs: none
 * Outputs: None
 * Return Value: 1 if capslock pressed, 0 otherwise
 * Side Effects: none
 */
i32 capslock_pressed(void) { return key_state[SCS1_PRESSED_CAPSLOCK]; }

i32 is_func_key(void) {
  return (key_state[SCS1_PRESSED_F1] || key_state[SCS1_PRESSED_F2] ||
  key_state[SCS1_PRESSED_F3] || key_state[SCS1_PRESSED_F4] ||
  key_state[SCS1_PRESSED_F5] || key_state[SCS1_PRESSED_F6] ||
  key_state[SCS1_PRESSED_F7] || key_state[SCS1_PRESSED_F8] ||
  key_state[SCS1_PRESSED_F9] || key_state[SCS1_PRESSED_F10] ||
  key_state[SCS1_PRESSED_F11] || key_state[SCS1_PRESSED_F12]);
}

i32 alt_pressed(void) {
  return key_state[SCS1_PRESSED_LEFTALT];
}
