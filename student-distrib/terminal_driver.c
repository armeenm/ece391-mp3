#ifndef TERMINAL_DRIVER_H
#define TERMINAL_DRIVER_H

#include "terminal_driver.h"
#include "keyboard.h"

int32_t terminal_read(void* buf, int32_t nbytes)
{
    return get_line_buffer((char *)buf, nbytes);
}
int32_t terminal_write(const void* buf, int32_t nbytes)
{
    char * buffer = (char *)buf;

    if(nbytes > strlen(buffer) || nbytes <= 0)
        return -1;

    int32_t index, bytes_written = 0;
    for(index = 0; index < nbytes; index++)
    {
        putc(buffer[index]);
        bytes_written++;
    }
    return bytes_written;
}
int32_t terminal_open()
{
    return 0;
}
int32_t terminal_close()
{
    return 0;
}


#endif