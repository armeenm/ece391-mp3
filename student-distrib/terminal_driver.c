#ifndef TERMINAL_DRIVER_H
#define TERMINAL_DRIVER_H

#include "terminal_driver.h"
#include "keyboard.h"

int terminal_read_flag = 0;

/* terminal_read
 * Description: Read input from the terminal
 * Inputs: buf - buffer to write line buffer to
 *         nbytes - number of bytes to read
 * Outputs: none
 * Return Value: number of bytes read
 * Function: To read from the line buffer
 */
int32_t terminal_read(int32_t UNUSED(fd), void* buf, int32_t nbytes)
{
    /* Get line buffer and return size of bytes read */
    return get_line_buffer((char *)buf, nbytes);
}


/* terminal_write
 * Description: Write input to the terminal
 * Inputs: buf - Buffer of chars to write to line buffer
 *         nbytes - number of bytes to write
 * Outputs: none
 * Return Value: number of bytes written
 * Function: To write to the line buffer
 */
int32_t terminal_write(int32_t UNUSED(fd), const void* buf, int32_t nbytes)
{
    /* Typecast buf to a char * */
    char * buffer = (char *)buf;

    /* If params are invalid return -1 */
    if(nbytes <= 0)
        return -1;

    /* Write all bytes to the screen via putc */
    int32_t index, bytes_written = 0;
    for(index = 0; index < nbytes; index++)
    {
        /* Write to screen */
        putc(buffer[index]);
        bytes_written++;
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
int32_t terminal_open(int32_t UNUSED(fd))
{
    return 0;
}

/* terminal_close
 * Description: Close the terminal driver
 * Inputs: none
 * Outputs: none
 * Return Value: 0
 * Function: Closes the terminal driver and deallocates any memory it needs to.
 */
int32_t terminal_close(int32_t UNUSED(fd))
{
    return 0;
}


#endif
