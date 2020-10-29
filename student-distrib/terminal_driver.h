#ifndef TERMINAL_DRIVER_H
#define TERMINAL_DRIVER_H

#include "keyboard.h"

static const char SHELL_PS1[] = "thanOS> ";

extern int terminal_read_flag;

/* Define Function Calls */
i32 terminal_read(i32 fd, void* buf, i32 nbytes);
i32 terminal_write(i32 fd, void const* buf, i32 nbytes);
i32 terminal_open(i32 fd);
i32 terminal_close(i32 fd);

#endif
