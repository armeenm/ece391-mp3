#ifndef TERMINAL_DRIVER_H
#define TERMINAL_DRIVER_H

#include "keyboard.h"
#include "lib.h"
#define TERMINAL_NUM 3

static const char SHELL_PS1[] = "391OS> ";

typedef struct terminal {
    char line_buf[LINE_BUFFER_SIZE];
    u32 line_buf_index;
    u16 cursor_x, cursor_y;
    u32 pid;
    u8 read_flag;
    u8 vid_mem_buf[NUM_COLS * NUM_ROWS * 2];
    u8 running;
}terminal;


/* Define Function Calls */
i32 terminal_read(i32 fd, void* buf, i32 nbytes);
i32 terminal_write(i32 fd, void const* buf, i32 nbytes);
i32 terminal_open(const u8* filename);
i32 terminal_close(i32 fd);

void restore_terminal(u8 term_num);
void switch_terminal(u8 term_num);
void init_terminals(void);
terminal* get_terminal_from_pid(u32 pid);
terminal* get_current_terminal(void);
#endif
