#ifndef TERMINAL_DRIVER_H
#define TERMINAL_DRIVER_H
#include "keyboard.h"

/* Define Function Calls */
int32_t terminal_read(int32_t UNUSED(fd), void* buf, int32_t nbytes);
int32_t terminal_write(int32_t UNUSED(fd), const void* buf, int32_t nbytes);
int32_t terminal_open(int32_t UNUSED(fd));
int32_t terminal_close(int32_t UNUSED(fd));

#define TERMINAL_TEXT "thanOS> "
#define TERMINAL_TEXT_SIZE 8


extern int terminal_read_flag;
#endif
