/* lib.c - Some basic library functions (printf, strlen, etc.) */

#include "lib.h"
#include "paging.h"
#include "syscall.h"
#include "terminal_driver.h"

static u16 screen_x;
static u16 screen_y;
static i8* video_mem = (i8*)VIDEO;

/* void clear(void);
 * Inputs: void
 * Return Value: none
 * Function: Clears video memory */
void clear(void) {
  u32 i;
  terminal* term = get_running_terminal();
  /* If there is a terminal running and it is not the current map video->videos */
  u8 remap_vid_mem = 0;
  if (term && term->id != current_terminal) {
    cli();
    remap_vid_mem = 1;
    map_vid_mem(get_current_pcb()->pid, (u32)VIDEO, (u32)VIDEO);
  }
  /* For all the pixels clear their values */
  for (i = 0; i < NUM_ROWS * NUM_COLS; ++i) {
    video_mem[i << 1] = ' ';
    video_mem[(i << 1) + 1] = ATTRIB;
  }
  /* If video memory was remapped map it back  */
  if (remap_vid_mem) {
    map_vid_mem(get_current_pcb()->pid, (u32)VIDEO, (u32)term->vid_mem_buf);
    set_terminal_screen_xy(current_terminal, 0, 0);
    sti();
  }
  /* Set screen pos to 0,0 */
  set_screen_xy(0, 0);
}
/* void scroll_up
 * Inputs: none
 * Return Value: none
 * Function: Scrolls up the video memory
 */
void scroll_up(void) {
  /* Copy the rows except the first one into the start of video memory */
  video_mem = (char*)memmove(video_mem, video_mem + ((NUM_COLS * 1) << 1),
                             (NUM_COLS * (NUM_ROWS - 1) << 1));
  u32 i;

  /* For the last row set it all to spaces to clear it */
  for (i = 0; i < NUM_COLS; ++i) {
    video_mem[(NUM_COLS * (NUM_ROWS - 1) + i) << 1] = ' ';
    video_mem[((NUM_COLS * (NUM_ROWS - 1) + i) << 1) + 1] = ATTRIB;
  }
}

/* void clear_screen_xy();
 * Inputs: none
 * Return Value: none
 * Function: clears the screen position at the current location
 */
void clear_screen_xy(void) {
  /* Sets char to ' ' at the current screen location */
  video_mem[(NUM_COLS * screen_y + screen_x) << 1] = ' ';
  /* sets attribute to ATTRIB at the current location */
  video_mem[((NUM_COLS * screen_y + screen_x) << 1) + 1] = ATTRIB;
}

/* void clear_terminal_screen_xy();
 * Inputs: u8 num_term -- number of terminal to clear at current x,y
 * Return Value: none
 * Function: clears the screen position at the current location
 * WARNING - assumes VIDEO_MEM is appropriately mapped
 */
void clear_terminal_screen_xy(u8 num_term) {
  /* Ensure valid num_term */
  if (num_term >= TERMINAL_NUM)
    return;
  terminal* term = &terminals[num_term];
  /* Sets char to ' ' at the current screen location */
  video_mem[(NUM_COLS * term->screen_y + term->screen_x) << 1] = ' ';
  /* sets attribute to ATTRIB at the current location */
  video_mem[((NUM_COLS * term->screen_y + term->screen_x) << 1) + 1] = ATTRIB;
}

/* void set_cursor_location(int x, int y)
 * Inputs:  x - x position to set cursor to
 *          y - y position to set cursor to
 * Return Value: none
 * Function: sets the VGA cursor location
 */
void set_cursor_location(u16 const x, u16 const y) {
  u16 const vga_position = (u16)(y * NUM_COLS + x);

  outb(VGA_CURSOR_HIGH_REGISTER, 0x3D4);
  outb(vga_position >> 8, VGA_DATA_REGISTER);

  outb(VGA_CURSOR_LOW_REGISTER, VGA_ADDRESS_REGISTER);
  outb(vga_position, VGA_DATA_REGISTER);
}

/* void setscreen_x(int x);
 * Inputs: x - position to set screen_x to
 * Return Value: none
 * Function: set value of screen_x to x
 */
void set_screen_x(u16 const x) {
  if (x < NUM_COLS) {
    /* If valid position set x position to x and set the cursor */
    screen_x = x;
    set_cursor_location(x, screen_y);
  }
}

/* void setscreen_y(int y);
 * Inputs: y - position to set screen_y to
 * Return Value: none
 * Function: set value of screen_y to y
 */
void set_screen_y(u16 const y) {
  if (y < NUM_ROWS) {
    /* If valid position set y position to y and set the cursor */
    screen_y = y;
    set_cursor_location(screen_x, y);
  }
}

/* void set_screen_xy(int x, int y);
 * Inputs:  y - position to set screen_y to
 *          x - position to set screen_x to
 * Return Value: none
 * Function: set value of screen_y to y, screen_x to x
 */
void set_screen_xy(u16 const x, u16 const y) {
  if (y < NUM_ROWS && x < NUM_COLS) {
    /* If valid position set y position to y and set the cursor */
    screen_y = y;
    screen_x = x;
    set_cursor_location(x, y);
  }
}

/* void set_terminal_screen_xy(u8 num_term, u16 x, u16 y);
 * Inputs:  y - position to set screen_y to
 *          x - position to set screen_x to
 *          num_term - terminal to set screen_x and y to
 * Return Value: none
 * Function: set value of screen_y to y, screen_x to x
 */
void set_terminal_screen_xy(u8 num_term, u16 x, u16 y) {
  /* validate x, y and num_term */
  if (num_term >= TERMINAL_NUM || (y >= NUM_ROWS && x >= NUM_COLS))
    return;
  /* Set terminal screen x and y */
  terminal* term = &terminals[num_term];
  term->screen_x = x;
  term->screen_y = y;
}

/* void set_terminal_screen_x(u8 num_term, u16 x);
 * Inputs:  x - position to set screen_x to
 *          num_term - terminal to set screen_x
 * Return Value: none
 * Function: set value of screen_x to x
 */
void set_terminal_screen_x(u8 num_term, u16 x) {
  /* Validate num_term and x */
  if (num_term >= TERMINAL_NUM || x >= NUM_COLS)
    return;
  /* set terminal screen x to x */
  terminal* term = &terminals[num_term];
  term->screen_x = x;
}

/* void set_terminal_screen_y(u8 num_term, u16 y);
 * Inputs:  y - position to set screen_y to
 *          num_term - terminal to set screen_y
 * Return Value: none
 * Function: set value of screen_y to y
 */
void set_terminal_screen_y(u8 num_term, u16 y) {
  /* Validate num_term and y */
  if (num_term >= TERMINAL_NUM || y >= NUM_ROWS)
    return;
  /* set terminal screen y to y */
  terminal* term = &terminals[num_term];
  term->screen_y = y;
}

/* int get_screen_x();
 * Inputs: void
 * Return Value: int
 * Function: get value of screen_x */
u16 get_screen_x(void) { return screen_x; }

/* int get_screen_y();
 * Inputs: void
 * Return Value: int
 * Function: get value of screen_y */
u16 get_screen_y(void) { return screen_y; }

/* Standard printf().
 * Only supports the following format strings:
 * %%  - print a literal '%' character
 * %x  - print a number in hexadecimal
 * %u  - print a number as an unsigned integer
 * %d  - print a number as a signed integer
 * %c  - print a character
 * %s  - print a string
 * %#x - print a number in 32-bit aligned hexadecimal, i.e.
 *       print 8 hexadecimal digits, zero-padded on the left.
 *       For example, the hex number "E" would be printed as
 *       "0000000E".
 *       Note: This is slightly different than the libc specification
 *       for the "#" modifier (this implementation doesn't add a "0x" at
 *       the beginning), but I think it's more flexible this way.
 *       Also note: %x is the only conversion specifier that can use
 *       the "#" modifier to alter output. */
i32 printf(i8* format, ...) {

  /* Pointer to the format string */
  i8* buf = format;

  /* Stack pointer for the other parameters */
  i32* esp = (void*)&format;
  esp++;

  while (*buf != '\0') {
    switch (*buf) {
    case '%': {
      i32 alternate = 0;
      buf++;

    format_char_switch:
      /* Conversion specifiers */
      switch (*buf) {
      /* Print a literal '%' character */
      case '%':
        putc('%');
        break;

      /* Use alternate formatting */
      case '#':
        alternate = 1;
        buf++;
        /* Yes, I know gotos are bad.  This is the
         * most elegant and general way to do this,
         * IMHO. */
        goto format_char_switch;

      /* Print a number in hexadecimal form */
      case 'x': {
        i8 conv_buf[64];
        if (alternate == 0) {
          itoa(*((u32*)esp), conv_buf, 16);
          puts(conv_buf);
        } else {
          i32 starting_index;
          i32 i;
          itoa(*((u32*)esp), &conv_buf[8], 16);
          i = starting_index = (i32)strlen(&conv_buf[8]);
          while (i < 8) {
            conv_buf[i] = '0';
            i++;
          }
          puts(&conv_buf[starting_index]);
        }
        esp++;
      } break;

      /* Print a number in unsigned int form */
      case 'u': {
        i8 conv_buf[36];
        itoa(*((u32*)esp), conv_buf, 10);
        puts(conv_buf);
        esp++;
      } break;

      /* Print a number in signed int form */
      case 'd': {
        i8 conv_buf[36];
        i32 value = *((i32*)esp);
        if (value < 0) {
          conv_buf[0] = '-';
          itoa((u32)-value, &conv_buf[1], 10);
        } else {
          itoa((u32)value, conv_buf, 10);
        }
        puts(conv_buf);
        esp++;
      } break;

      /* Print a single character */
      case 'c':
        putc((i8) * ((i32*)esp));
        esp++;
        break;

      /* Print a NULL-terminated string */
      case 's':
        puts(*((i8**)esp));
        esp++;
        break;

      default:
        break;
      }

    } break;

    default:
      putc(*buf);
      break;
    }
    buf++;
  }
  return (buf - format);
}

/* i32 puts(i8* s);
 *   Inputs: int_8* s = pointer to a string of characters
 *   Return Value: Number of bytes written
 *    Function: Output a string to the console */
i32 puts(i8* s) {
  register i32 index = 0;
  while (s[index] != '\0') {
    putc(s[index]);
    index++;
  }
  return index;
}
/* void terminal_putc(u8 num_term, u8 c);
 * Inputs: int_8* c = character to print
 *         uint_8 num_term -- termrinal to print to
 * Return Value: void
 *  Function: Output a character to the console */
void terminal_putc(u8 num_term, i8 c) {
  if (c == 0 || num_term >= TERMINAL_NUM)
    return;
  terminal* term = &terminals[num_term];

  /* Get x and y pos  */
  u16 x = get_screen_x();
  u16 y = get_screen_y();
  /* If not the currrent terminal screen_pos is term->screen_ */
  if (term->id != current_terminal) {
    x = term->screen_x;
    y = term->screen_y;
  }

  if (c == '\n') {
    /* If there is still screenspace left then go to next line */
    if (y < NUM_ROWS - 1) {
      y++;
    } else {
      /* If there is no screenspace left scroll up */
      scroll_up();
    }
    /* Reset to the start of the line */
    x = 0;

  } else if (c == '\b') {
    /* if x is zero go to previous line */
    if (x == 0 && y > 0) {
      x = NUM_COLS - 1;
      y--;
    } else {
      /* Otherwise go back one */
      x--;
    }

    /* If not current terminal use terminal libc functions */
    if (term->id == current_terminal) {
      /* clear the character at the current location */
      set_screen_xy(x, y);
      clear_screen_xy();
    } else {
      /* clear the character at the current location */
      set_terminal_screen_xy(term->id, x, y);
      clear_terminal_screen_xy(term->id);
    }

    return;

  } else if (c == '\t') {
    /* if tab use a space */
    terminal_putc(num_term, ' ');
    return;

  } else {
    video_mem[(NUM_COLS * y + x) << 1] = c;
    video_mem[((NUM_COLS * y + x) << 1) + 1] = ATTRIB;

    ++x;

    /* if it is a newline then go to the next line */
    if ((x / NUM_COLS) > 0) {

      /* At the top of the screen scroll up */
      if (y == NUM_ROWS - 1)
        scroll_up();
      else {
        /* Otherwise go to the next row */
        y = (u16)((y + (x / NUM_COLS)) % NUM_ROWS);
      }
    }
    /* Make sure x is bounded by the columns */
    x %= NUM_COLS;
  }

  if (term->id == current_terminal) {
    /* Set location of the cursor based on new x and y */
    set_screen_xy(x, y);
  } else {
    set_terminal_screen_xy(term->id, x, y);
  }
}

/* void putc(u8 c);
 * Inputs: uint_8* c = character to print
 * Return Value: void
 *  Function: Output a character to the console */
void putc(i8 c) {
  terminal* term = get_running_terminal();
  if (c == 0)
    return;

  /* If not current terminal remap video memory so the screen can be
   * directly written to.
   */
  u8 remap_vid_mem = 0;
  if (term && term->id != current_terminal) {
    remap_vid_mem = 1;
    map_vid_mem(get_current_pcb()->pid, (u32)VIDEO, (u32)VIDEO);
  }

  if (c == '\n') {
    /* If there is still screenspace left then go to next line */
    if (screen_y < NUM_ROWS - 1) {
      screen_y++;
    } else {
      /* If there is no screenspace left scroll up */
      scroll_up();
    }
    /* Reset to the start of the line */
    screen_x = 0;

  } else if (c == '\b') {
    /* Get x and y pos  */
    u16 x = get_screen_x();
    u16 y = get_screen_y();
    /* if x is zero go to previous line */
    if (x == 0 && y > 0) {
      set_screen_xy(NUM_COLS - 1, y - 1);
    } else {
      /* Otherwise go back one */
      set_screen_x(x - 1);
    }
    /* clear the character at the current location */
    clear_screen_xy();

  } else if (c == '\t') {
    /* if tab use a space */
    putc(' ');
    return;

  } else {
    video_mem[(NUM_COLS * screen_y + screen_x) << 1] = c;
    video_mem[((NUM_COLS * screen_y + screen_x) << 1) + 1] = ATTRIB;

    ++screen_x;

    /* if it is a newline then go to the next line */
    if ((screen_x / NUM_COLS) > 0) {

      /* At the top of the screen scroll up */
      if (screen_y == NUM_ROWS - 1)
        scroll_up();
      else {
        /* Otherwise go to the next row */
        screen_y = (u16)((screen_y + (screen_x / NUM_COLS)) % NUM_ROWS);
      }
    }
    /* Make sure x is bounded by the columns */
    screen_x %= NUM_COLS;
  }
  /* If video memory was remap move it back to the buffer location */
  if (remap_vid_mem) {
    map_vid_mem(get_current_pcb()->pid, (u32)VIDEO, (u32)term->vid_mem_buf);
  }
  /* Set location of the cursor based on new screen_x and screen_y */
  set_cursor_location(screen_x, screen_y);
}

/* i8* itoa(u32 value, i8* buf, i32 radix);
 * Inputs: u32 value = number to convert
 *            i8* buf = allocated buffer to place string in
 *          i32 radix = base system. hex, oct, dec, etc.
 * Return Value: number of bytes written
 * Function: Convert a number to its ASCII representation, with base "radix" */
i8* itoa(u32 value, i8* buf, i32 radix) {
  static i8 lookup[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
  i8* newbuf = buf;
  i32 i;
  u32 newval = value;

  /* Special case for zero */
  if (value == 0) {
    buf[0] = '0';
    buf[1] = '\0';
    return buf;
  }

  /* Go through the number one place value at a time, and add the
   * correct digit to "newbuf".  We actually add characters to the
   * ASCII string from lowest place value to highest, which is the
   * opposite of how the number should be printed.  We'll reverse the
   * characters later. */
  while (newval > 0) {
    i = (i32)newval % radix;
    *newbuf = lookup[i];
    newbuf++;
    newval /= (u32)radix;
  }

  /* Add a terminating NULL */
  *newbuf = '\0';

  /* Reverse the string and return */
  return strrev(buf);
}

/* i8* strrev(i8* s);
 * Inputs: i8* s = string to reverse
 * Return Value: reversed string
 * Function: reverses a string s */
i8* strrev(i8* s) {
  register i8 tmp;
  register i32 beg = 0;
  register i32 end = (i32)strlen(s) - 1;

  while (beg < end) {
    tmp = s[end];
    s[end] = s[beg];
    s[beg] = tmp;
    beg++;
    end--;
  }
  return s;
}

/* u32 strnonspace(i8* s);
 * Inputs:  i8* s
 * Return Value: u32 postition of first non-space character:
 * Function: returns the postition of the first non-scace char */
u32 strnonspace(i8 const* s) {
  u32 i;
  for (i = 0; i < strlen(s); i++) {
    if (s[i] != ' ') {
      return i;
    }
  }
  return i;
}

/* u32 strlen(const i8* s);
 * Inputs: const i8* s = string to take length of
 * Return Value: length of string s
 * Function: return length of string s */
u32 strlen(i8 const* s) {
  register u32 len = 0;
  while (s[len] != '\0')
    len++;
  return len;
}

/* void* memset(void* s, i32 c, u32 n);
 * Inputs:    void* s = pointer to memory
 *          i32 c = value to set memory to
 *         u32 n = number of bytes to set
 * Return Value: new string
 * Function: set n consecutive bytes of pointer s to value c */
void* memset(void* s, i32 c, u32 n) {
  c &= 0xFF;
  asm volatile("                 \n\
            .memset_top:            \n\
            testl   %%ecx, %%ecx    \n\
            jz      .memset_done    \n\
            testl   $0x3, %%edi     \n\
            jz      .memset_aligned \n\
            movb    %%al, (%%edi)   \n\
            addl    $1, %%edi       \n\
            subl    $1, %%ecx       \n\
            jmp     .memset_top     \n\
            .memset_aligned:        \n\
            movw    %%ds, %%dx      \n\
            movw    %%dx, %%es      \n\
            movl    %%ecx, %%edx    \n\
            shrl    $2, %%ecx       \n\
            andl    $0x3, %%edx     \n\
            cld                     \n\
            rep     stosl           \n\
            .memset_bottom:         \n\
            testl   %%edx, %%edx    \n\
            jz      .memset_done    \n\
            movb    %%al, (%%edi)   \n\
            addl    $1, %%edi       \n\
            subl    $1, %%edx       \n\
            jmp     .memset_bottom  \n\
            .memset_done:           \n\
            "
               :
               : "a"(c << 24 | c << 16 | c << 8 | c), "D"(s), "c"(n)
               : "edx", "memory", "cc");
  return s;
}

/* void* memset_word(void* s, i32 c, u32 n);
 * Description: Optimized memset_word
 * Inputs:    void* s = pointer to memory
 *          i32 c = value to set memory to
 *         u32 n = number of bytes to set
 * Return Value: new string
 * Function: set lower 16 bits of n consecutive memory locations of pointer s to value c */
void* memset_word(void* s, i32 c, u32 n) {
  asm volatile("                 \n\
            movw    %%ds, %%dx      \n\
            movw    %%dx, %%es      \n\
            cld                     \n\
            rep     stosw           \n\
            "
               :
               : "a"(c), "D"(s), "c"(n)
               : "edx", "memory", "cc");
  return s;
}

/* void* memset_dword(void* s, i32 c, u32 n);
 * Inputs:    void* s = pointer to memory
 *          i32 c = value to set memory to
 *         u32 n = number of bytes to set
 * Return Value: new string
 * Function: set n consecutive memory locations of pointer s to value c */
void* memset_dword(void* s, i32 c, u32 n) {
  asm volatile("                 \n\
            movw    %%ds, %%dx      \n\
            movw    %%dx, %%es      \n\
            cld                     \n\
            rep     stosl           \n\
            "
               :
               : "a"(c), "D"(s), "c"(n)
               : "edx", "memory", "cc");
  return s;
}

/* void* memcpy(void* dest, const void* src, u32 n);
 * Inputs:      void* dest = destination of copy
 *         const void* src = source of copy
 *              u32 n = number of byets to copy
 * Return Value: pointer to dest
 * Function: copy n bytes of src to dest */
void* memcpy(void* dest, const void* src, u32 n) {
  asm volatile("                 \n\
            .memcpy_top:            \n\
            testl   %%ecx, %%ecx    \n\
            jz      .memcpy_done    \n\
            testl   $0x3, %%edi     \n\
            jz      .memcpy_aligned \n\
            movb    (%%esi), %%al   \n\
            movb    %%al, (%%edi)   \n\
            addl    $1, %%edi       \n\
            addl    $1, %%esi       \n\
            subl    $1, %%ecx       \n\
            jmp     .memcpy_top     \n\
            .memcpy_aligned:        \n\
            movw    %%ds, %%dx      \n\
            movw    %%dx, %%es      \n\
            movl    %%ecx, %%edx    \n\
            shrl    $2, %%ecx       \n\
            andl    $0x3, %%edx     \n\
            cld                     \n\
            rep     movsl           \n\
            .memcpy_bottom:         \n\
            testl   %%edx, %%edx    \n\
            jz      .memcpy_done    \n\
            movb    (%%esi), %%al   \n\
            movb    %%al, (%%edi)   \n\
            addl    $1, %%edi       \n\
            addl    $1, %%esi       \n\
            subl    $1, %%edx       \n\
            jmp     .memcpy_bottom  \n\
            .memcpy_done:           \n\
            "
               :
               : "S"(src), "D"(dest), "c"(n)
               : "eax", "edx", "memory", "cc");
  return dest;
}

/* void* memmove(void* dest, const void* src, u32 n);
 * Description: Optimized memmove (used for overlapping memory areas)
 * Inputs:      void* dest = destination of move
 *         const void* src = source of move
 *              u32 n = number of byets to move
 * Return Value: pointer to dest
 * Function: move n bytes of src to dest */
void* memmove(void* dest, const void* src, u32 n) {
  asm volatile("                             \n\
            movw    %%ds, %%dx                  \n\
            movw    %%dx, %%es                  \n\
            cld                                 \n\
            cmp     %%edi, %%esi                \n\
            jae     .memmove_go                 \n\
            leal    -1(%%esi, %%ecx), %%esi     \n\
            leal    -1(%%edi, %%ecx), %%edi     \n\
            std                                 \n\
            .memmove_go:                        \n\
            rep     movsb                       \n\
            "
               :
               : "D"(dest), "S"(src), "c"(n)
               : "edx", "memory", "cc");
  return dest;
}

/* i32 strncmp(const i8* s1, const i8* s2, u32 n)
 * Inputs: const i8* s1 = first string to compare
 *         const i8* s2 = second string to compare
 *               u32 n = number of bytes to compare
 * Return Value: A zero value indicates that the characters compared
 *               in both strings form the same string.
 *               A value greater than zero indicates that the first
 *               character that does not match has a greater value
 *               in str1 than in str2; And a value less than zero
 *               indicates the opposite.
 * Function: compares string 1 and string 2 for equality */
i32 strncmp(const i8* s1, const i8* s2, u32 n) {
  u32 i;
  for (i = 0; i < n; i++) {
    if ((s1[i] != s2[i]) || (s1[i] == '\0') /* || s2[i] == '\0' */) {

      /* The s2[i] == '\0' is unnecessary because of the short-circuit
       * semantics of 'if' expressions in C.  If the first expression
       * (s1[i] != s2[i]) evaluates to false, that is, if s1[i] ==
       * s2[i], then we only need to test either s1[i] or s2[i] for
       * '\0', since we know they are equal. */
      return s1[i] - s2[i];
    }
  }
  return 0;
}

/* i8* strcpy(i8* dest, const i8* src)
 * Inputs:      i8* dest = destination string of copy
 *         const i8* src = source string of copy
 * Return Value: pointer to dest
 * Function: copy the source string into the destination string */
i8* strcpy(i8* dest, const i8* src) {
  i32 i = 0;
  while (src[i] != '\0') {
    dest[i] = src[i];
    i++;
  }
  dest[i] = '\0';
  return dest;
}

/* i8* strcpy(i8* dest, const i8* src, u32 n)
 * Inputs:      i8* dest = destination string of copy
 *         const i8* src = source string of copy
 *                u32 n = number of bytes to copy
 * Return Value: pointer to dest
 * Function: copy n bytes of the source string into the destination string */
i8* strncpy(i8* dest, const i8* src, u32 n) {
  u32 i = 0;
  while (src[i] != '\0' && i < n) {
    dest[i] = src[i];
    i++;
  }
  while (i < n) {
    dest[i] = '\0';
    i++;
  }
  return dest;
}

/* void test_interrupts(void)
 * Inputs: void
 * Return Value: void
 * Function: increments video memory. To be used to test rtc */
void test_interrupts(void) {
  i32 i;
  for (i = 0; i < NUM_ROWS * NUM_COLS; i++) {
    video_mem[i << 1]++;
  }
}
