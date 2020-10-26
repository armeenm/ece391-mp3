#include "keyboard.h"
#include "i8259.h"
#include "lib.h"
#include "terminal_driver.h"

/* Declare variables for keyboard */
uint8_t key_state[SCS1_PRESSED_F12];
int caps_lock_repeat = 0;
int multi_byte = 0;
char line_buffer[LINE_BUFFER_SIZE];
int line_buffer_index = 0;

/* contains_newline
 * Description: To determine if a newline is present in a buffer
 * Inputs: buf - buffer to write line buffer to
 *         nbytes - number of bytes to read
 * Outputs: none
 * Return Value:  0 - the buffer does not contain a newline
 *                1 - if the buffer contains a newline
 * Function: To determine if \n or \r is present in a buffer
 */
int contains_newline(char * buf, int32_t size)
{
  /* Loop through the entire buffer */
  int index = 0;
  for(index = 0; index < size; index++)
  {
    /* If the buffer contains a newline return 1 */
    if(buf[index] == '\n' || buf[index] == '\r')
    {
      return 1;
    }
  }
  /* No newline is present - return 0 */
  return 0;
}

/* get_line_buffer
 * Description: Get the line buffer and copy it into a buffer
 * Inputs: buffer - buffer that line_buffer writes to
 *         nbytes - number of bytes to write
 * Outputs: buffer is filled with the line_buffer
 * Return Value: size of the buffer
 * Function: Gets a linebuffer, returns the size of it, and clears
 * the current line buffer
 */
int32_t get_line_buffer(char * buffer, int32_t nbytes)
{
  if(nbytes <= 0)
    return -1;
  terminal_read_flag = 1;
  /* Wait while the line_buffer does not contain a \n */
  while(contains_newline(line_buffer, LINE_BUFFER_SIZE) != 1);

  /* Loop through the buffer until there is a newline */
  int index = 0, continue_flag = 1;
  while(index < nbytes && index < LINE_BUFFER_SIZE && continue_flag == 1)
  {
    /* set the buffer to the line_buffer */
    buffer[index] = line_buffer[index];
    /* if there is a newline then do not continue */
    if(line_buffer[index] == '\n' || line_buffer[index] == '\r')
    {
      continue_flag = 0;
    }
    index++;
  }

  /* set the size of the string to the current index */
  int sizeofstring = index;

  /* for the remaining characters set them to empty */
  while(index < LINE_BUFFER_SIZE && index < nbytes)
  {
    buffer[index] = 0;
    index++;
  }

  /* make sure that the last character is a newline */
  if(nbytes < LINE_BUFFER_SIZE)
  {
    buffer[nbytes - 1] = '\n';
  }

  /* If the size is greater than line_buffer_size then set the last char to \n */
  if(nbytes > LINE_BUFFER_SIZE)
  {
    buffer[LINE_BUFFER_SIZE - 1] = '\n';
  }

  /* clear the line buffer and return the size of the buffer written to */
  clear_line_buffer();

  terminal_read_flag = 0;
  
  return sizeofstring;
}



/* init_keyboard
 * Description: Initializes the keyboard
 * Inputs: None
 * Return Value: None
 * Side Effects: Enables the keyboard IRQ.
 */
void init_keyboard(void)
{
  /* Enable irq, set multi_byte and caps lock to zero */
  enable_irq(KEYBOARD_IRQ);
  multi_byte = 0;
  caps_lock_repeat = 0;
  /* Clear the line buffer */
  clear_line_buffer();
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
  /* Send eoi */
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
  /* If scancode is multibyte set multibyte_flag to 1 */
  if(scancode == SCS1_MULTIBYTE)
  {
    multi_byte = 1;
  }
  else if(multi_byte == 1)
  {
    /* Handle multi_byte keycodes here */
    multi_byte = 0;
  }
  else if (scancode > 0 && scancode < SCS1_PRESSED_F12)
  {
    /* If capslock is pressed and is not being held down NOT the key_state  */
    if(scancode == SCS1_PRESSED_CAPSLOCK && caps_lock_repeat == 0)
    {
      key_state[scancode] ^= 1;
      caps_lock_repeat = 1;
    }
    else
    {
      /* Otherwise the keystate is equal to 1 */
      key_state[scancode] = 1;
    }
    
    /* Get the printable keycode if any */
    char disp = handle_disp(keycodes[scancode]);

    /* If the command ctrl + l is pressed clear the screen */
    if((key_state[SCS1_PRESSED_LEFTCTRL] == 1 && scancode == SCS1_PRESSED_L) || (key_state[SCS1_PRESSED_L] == 1 && scancode == SCS1_PRESSED_LEFTCTRL))
    {
      /* Clear screen and reset terminal */
      clear();
      set_screen_xy(0, 0);
      if(terminal_read_flag == 1)
      { 
        /* Write to terminal to put "thanOS> " in */
        terminal_write(0, TERMINAL_TEXT, TERMINAL_TEXT_SIZE);
        int i = 0;
        /* Write what's in the input buffer */
        for(i = 0; i < line_buffer_index; i++)
        {
          putc(line_buffer[i]);
        }
      }
      
      
    }
    else if(terminal_read_flag == 0)
    {
      return;
    }
    else if(key_state[SCS1_PRESSED_BACKSPACE] == 1)
    {
      /* If there is data in the line buffer and backspace is pressed
       * decrement the line buffer and handle the backspace keypress
       */
      if(line_buffer_index > 0)
      {
        line_buffer_index--;
        line_buffer[line_buffer_index] = 0;
        putc('\b');
      }
    }
    /* if there is a key to display */
    else if (disp)
    {
      /* If the linebuffer is not full (127 characters,  LINE_BUFFER_SIZE - 1) handle keypress */
      if(line_buffer_index < LINE_BUFFER_SIZE - 1)
      {
        /* print the key and put it in the buffer */
        putc(disp);
        line_buffer[line_buffer_index] = disp;
        line_buffer_index++;
      }
      else if(disp == '\n')
      {
        /* Handle newline, put it in the end of the buffer, since line_buffer_index >= LINE_BUFFER_SIZE - 1 */
        putc(disp);
        line_buffer[line_buffer_index] = disp;
        line_buffer_index++;
      }
    }

      
  }
  /* This section handles key releases */
  else if (scancode > 0x81 && scancode <= SCS1_RELEASED_F12) {
    /* If capslock is released reset repeat to 0 so we can toggle it again */
    if(scancode == SCS1_RELEASED_CAPSLOCK)
    {
      caps_lock_repeat = 0;
    }
    else {
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
char handle_disp(char disp)
{
  /* If shift is pressed or capslock then print its shift/capslock modified value */
  if(shift_pressed() || (capslock_pressed() && disp >= 'a' && disp <= 'z'))
  {
    /* If the character is a-z capitalize it */
    if(disp >= 'a' && disp <= 'z' && !(capslock_pressed() && shift_pressed()))
    {
      /* an uppercase character is SCS1_UPPERCASE_OFFSET away from its lowercase value */
      disp -= SCS1_UPPERCASE_OFFSET;
    }
    else
    {
      /* If the character should be modified by shift find which character to print */
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
  /* Return the character to display */
  return disp;
}

/* clear_line_buffer
 * Description: Clears the line buffer
 * Inputs: none
 * Outputs: None
 * Return Value: none
 * Side Effects: Clears the line_buffer and resets the index to 0
 */
void clear_line_buffer()
{
  /* Iterate through the entire line buffer */
  int i;
  for(i = 0; i < LINE_BUFFER_SIZE; i++)
  {
    /* Set line__buffer at index to 0 */
    line_buffer[i] = 0;
  }
  /* Set index to 0 */
  line_buffer_index = 0;
}

/* shift_pressed
 * Description: Indicates whether shift is pressed or not
 * Inputs: none
 * Outputs: None
 * Return Value: 1 if shift pressed, 0 otherwise
 * Side Effects: none
 */
int shift_pressed()
{
  return (key_state[SCS1_PRESSED_LEFTSHIFT] == 1 || key_state[SCS1_PRESSED_RIGHTSHIFT] == 1);
}

/* capslock_pressed
 * Description: Indicates whether capslock is pressed or not
 * Inputs: none
 * Outputs: None
 * Return Value: 1 if capslock pressed, 0 otherwise
 * Side Effects: none
 */
int capslock_pressed()
{
  return (key_state[SCS1_PRESSED_CAPSLOCK] == 1);
}