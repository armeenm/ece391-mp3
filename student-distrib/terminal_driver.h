#ifndef TERMINAL_DRIVER_H
#define TERMINAL_DRIVER_H
#include "keyboard.h"

int32_t terminal_read(void* buf, int32_t nbytes);
int32_t terminal_write(const void* buf, int32_t nbytes);
int32_t terminal_open();
int32_t terminal_close();


#endif