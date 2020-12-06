#ifndef TERMINAL_DRIVER_H
#define TERMINAL_DRIVER_H

#include "keyboard.h"
#include "lib.h"
#define TERMINAL_NUM 3

#define RTC_DEFAULT_REAL_FREQ 1024
#define RTC_DEFAULT_VIRT_FREQ 2

static const char SHELL_PS1[] = "391OS> ";


typedef struct virtual_rtc {
  u32 virt_freq; // must be a power of 2 <= 1024
  u32 real_freq; // The intial, real freq requested of RTC (1024)
  u32 int_count;
  volatile u8 flag;
} virtual_rtc;

typedef struct {
	char line_buf[128];
	u32 line_buf_index;
	u16 screen_x, screen_y;
	u32 pid;
	u8 id;
	u8 read_flag;
	u8* vid_mem_buf;
	u8 running;
	u8 status;
	u8 vidmap;
	virtual_rtc rtc;
} terminal;

u8 current_terminal;
terminal terminals[TERMINAL_NUM];

/* Define Function Calls */
i32 terminal_read(i32 fd, void* buf, i32 nbytes);
i32 terminal_write(i32 fd, void const* buf, i32 nbytes);
i32 terminal_open(const u8* filename);
i32 terminal_close(i32 fd);

void restore_terminal(u8 term_num);
void switch_terminal(u8 term_num);
void init_terminals(void);
terminal* get_terminal_from_pid(u32 pid);
terminal* get_running_terminal(void);
terminal* new_terminal(u8 pid);
#endif
