#include "keyboard.h"
#include "i8259.h"
#include "lib.h"

/* lshift, rshift, lcontrol, rcontrol, capslock, backspace*/
uint8_t key_state[SCS1_PRESSED_F12];
int caps_lock_repeat = 0;
int multi_byte = 0;
char line_buffer[LINE_BUFFER_SIZE];
int line_buffer_index = 0;


int contains_newline(char * buf, int32_t size)
{
  int index = 0;
  for(index = 0; index < size; index++)
  {
    if(buf[index] == '\n')
    {
      return 1;
    }
  }
  return 0;
}

int32_t get_line_buffer(char * buffer, int32_t nbytes)
{
  int index = 0, continue_flag = 1;
  while(contains_newline(line_buffer, LINE_BUFFER_SIZE) != 1);
  while(index < nbytes && continue_flag == 1)
  {
    buffer[index] = line_buffer[index];
    
    if(line_buffer[index] == '\n' || line_buffer[index] == '\r')
    {
      continue_flag = 0;
    }
    index++;
  }
  int sizeofstring = index;
  while(index < LINE_BUFFER_SIZE)
  {
    buffer[index] = 0;
    index++;
  }

  if(nbytes < LINE_BUFFER_SIZE)
  {
    buffer[nbytes - 1] = '\n';
  }

  clear_line_buffer();
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
  enable_irq(KEYBOARD_IRQ);
  multi_byte = 0;
  caps_lock_repeat = 0;
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
  if(scancode == SCS1_MULTIBYTE)
  {
    multi_byte = 1;
  }
  else if(multi_byte == 1)
  {
    multi_byte = 0;
  }
  else if (scancode > 0 && scancode < SCS1_PRESSED_F12)
  {
    if(scancode == SCS1_PRESSED_CAPSLOCK && caps_lock_repeat == 0)
    {
      key_state[scancode] ^= 1;
      caps_lock_repeat = 1;
    }
    else
    {
      key_state[scancode] = 1;
    }
    
    char disp = handle_disp(keycodes[scancode]);

    if((key_state[SCS1_PRESSED_LEFTCTRL] == 1 && scancode == SCS1_PRESSED_L) || (key_state[SCS1_PRESSED_L] == 1 && scancode == SCS1_PRESSED_LEFTCTRL))
    {
      clear();
      clear_line_buffer();
      set_screen_xy(0, 0);
    }
    else if(key_state[SCS1_PRESSED_BACKSPACE] == 1)
    {
      if(line_buffer_index > 0)
      {
        line_buffer_index--;
        line_buffer[line_buffer_index] = 0;
        putc('\b');
      }
    }
    else if (disp)
    {
      if(line_buffer_index < LINE_BUFFER_SIZE - 1)
      {
        putc(disp);
        line_buffer[line_buffer_index] = disp;
        line_buffer_index++;
      }
      else if(disp == '\n')
      {
        putc(disp);
        line_buffer[line_buffer_index] = disp;
        line_buffer_index++;
      }
    }

      
  }
  else if (scancode > 0x81 && scancode <= SCS1_RELEASED_F12) {
    if(scancode == SCS1_RELEASED_CAPSLOCK)
    {
      caps_lock_repeat = 0;
    }
    else {
      key_state[scancode - 0x80] = 0;
    } 
  }
}

char handle_disp(char disp)
{
  if(key_state[SCS1_PRESSED_LEFTSHIFT] == 1
  || key_state[SCS1_PRESSED_RIGHTSHIFT] == 1
  || (key_state[SCS1_PRESSED_CAPSLOCK] == 1 && disp >= 'a' && disp <= 'z'))
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

void clear_line_buffer()
{
  int i;
  for(i = 0; i < LINE_BUFFER_SIZE; i++)
  {
    line_buffer[i] = 0;
  }
  line_buffer_index = 0;
}