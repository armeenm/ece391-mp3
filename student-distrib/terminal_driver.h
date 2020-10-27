#ifndef TERMINAL_DRIVER_H
#define TERMINAL_DRIVER_H

#include "keyboard.h"

static const char SHELL_PS1[] = "thanOS> ";

extern int terminal_read_flag;

/* Define Function Calls */
int32_t terminal_read(int32_t fd, void* buf, int32_t nbytes);
int32_t terminal_write(int32_t fd, void const* buf, int32_t nbytes);
int32_t terminal_open(int32_t fd);
int32_t terminal_close(int32_t fd);

#endif
