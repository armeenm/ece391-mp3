#ifndef TERMINAL_DRIVER_H
#define TERMINAL_DRIVER_H

#include "terminal_driver.h"
#include "keyboard.h"

int terminal_read_flag = 0;

/* terminal_read
 * Description: Read input from the terminal
 * Inputs: buf - buf to write line buf to
 *         nbytes - number of bytes to read
 * Outputs: none
 * Return Value: number of bytes read
 * Function: To read from the line buf
 */
int32_t terminal_read(int32_t UNUSED(fd), void* const buf, int32_t const nbytes) {
  /* Get line buf and return size of bytes read */
  return get_line_buf((char*)buf, nbytes);
}

/* terminal_write
 * Description: Write input to the terminal
 * Inputs: buf - Buffer of chars to write to line buf
 *         nbytes - number of bytes to write
 * Outputs: none
 * Return Value: number of bytes written
 * Function: To write to the line buf
 */
int32_t terminal_write(int32_t UNUSED(fd), void const* const buf, int32_t const nbytes) {
  /* Typecast buf to a char* */
  char const* const cbuf = (char const*)buf;
  int32_t i, bytes_written = 0;

  /* If params are invalid return -1 */
  if (nbytes <= 0 || !buf)
    return -1;

  for (i = 0; i < nbytes; ++i) {
    /* Write to screen */
    putc(cbuf[i]);
    ++bytes_written;
  }

  /* Return bytes written */
  return bytes_written;
}

/* terminal_open
 * Description: Opens the terminal driver
 * Inputs: none
 * Outputs: none
 * Return Value: 0
 * Function: Opens the terminal driver and allocates any memory it needs to
 */
int32_t terminal_open(int32_t UNUSED(fd)) { return 0; }

/* terminal_close
 * Description: Close the terminal driver
 * Inputs: none
 * Outputs: none
 * Return Value: 0
 * Function: Closes the terminal driver and deallocates any memory it needs to.
 */
int32_t terminal_close(int32_t UNUSED(fd)) { return 0; }

#endif
