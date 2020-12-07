/* lib.h - Defines for useful library functions
 */

#ifndef LIB_H
#define LIB_H

#include "types.h"
#include "util.h"

#define VGA_DATA_REGISTER 0x3D5
#define VGA_ADDRESS_REGISTER 0x3D4
#define VGA_CURSOR_HIGH_REGISTER 0x0E
#define VGA_CURSOR_LOW_REGISTER 0x0F

enum { ATTRIB = 2, NUM_ROWS = 25, NUM_COLS = 80, VIDEO = 0xB8000 };

i32 printf(i8* format, ...);
void putc(i8 c);
i32 puts(i8* s);
i8* itoa(u32 value, i8* buf, i32 radix);
i8* strrev(i8* s);
u32 strlen(const i8* s);
u32 strnonspace(i8 const* s);
void clear(void);

void* memset(void* s, i32 c, u32 n);
void* memset_word(void* s, i32 c, u32 n);
void* memset_dword(void* s, i32 c, u32 n);
void* memcpy(void* dest, const void* src, u32 n);
void* memmove(void* dest, const void* src, u32 n);
i32 strncmp(const i8* s1, const i8* s2, u32 n);
i8* strcpy(i8* dest, const i8* src);
i8* strncpy(i8* dest, const i8* src, u32 n);

/* Userspace address-check functions */
i32 bad_userspace_addr(const void* addr, i32 len);
i32 safe_strncpy(i8* dest, const i8* src, i32 n);

void test_interrupts(void);
u16 get_screen_x(void);
u16 get_screen_y(void);
void set_screen_x(u16 x);
void set_screen_y(u16 y);
void scroll_up(void);
void clear_screen_xy(void);
void set_cursor_location(u16 x, u16 y);
void set_screen_xy(u16 x, u16 y);

void set_terminal_screen_xy(u8 num_term, u16 x, u16 y);
void set_terminal_screen_x(u8 num_term, u16 x);
void set_terminal_screen_y(u8 num_term, u16 y);
void clear_terminal_screen_xy(u8 num_term);
void terminal_putc(u8 num_term, i8 c);

/* Port read functions */
/* Inb reads a byte and returns its value as a zero-extended 32-bit
 * unsigned int */
static inline u32 inb(int port) {
  u32 val;
  asm volatile("             \n\
            xorl %0, %0         \n\
            inb  (%w1), %b0     \n\
            "
               : "=a"(val)
               : "d"(port)
               : "memory");
  return val;
}

/* Reads two bytes from two consecutive ports, starting at "port",
 * concatenates them little-endian style, and returns them zero-extended
 * */
static inline u32 inw(int port) {
  u32 val;
  asm volatile("             \n\
            xorl %0, %0         \n\
            inw  (%w1), %w0     \n\
            "
               : "=a"(val)
               : "d"(port)
               : "memory");
  return val;
}

/* Reads four bytes from four consecutive ports, starting at "port",
 * concatenates them little-endian style, and returns them */
static inline u32 inl(int port) {
  u32 val;
  asm volatile("inl (%w1), %0" : "=a"(val) : "d"(port) : "memory");
  return val;
}

/* Writes a byte to a port */
#define outb(data, port)                                                                           \
  do {                                                                                             \
    asm volatile("outb %b1, (%w0)" : : "d"(port), "a"(data) : "memory", "cc");                     \
  } while (0)

/* Writes two bytes to two consecutive ports */
#define outw(data, port)                                                                           \
  do {                                                                                             \
    asm volatile("outw %w1, (%w0)" : : "d"(port), "a"(data) : "memory", "cc");                     \
  } while (0)

/* Writes four bytes to four consecutive ports */
#define outl(data, port)                                                                           \
  do {                                                                                             \
    asm volatile("outl %l1, (%w0)" : : "d"(port), "a"(data) : "memory", "cc");                     \
  } while (0)

/* Clear interrupt flag - disables interrupts on this processor */
#define cli()                                                                                      \
  do {                                                                                             \
    asm volatile("cli" : : : "memory", "cc");                                                      \
  } while (0)

/* Save flags and then clear interrupt flag
 * Saves the EFLAGS register into the variable "flags", and then
 * disables interrupts on this processor */
#define cli_and_save(flags)                                                                        \
  do {                                                                                             \
    asm volatile("                   \n\
            pushfl                    \n\
            popl %0                   \n\
            cli                       \n\
            "                                                           \
                 : "=r"(flags)                                                                     \
                 :                                                                                 \
                 : "memory", "cc");                                                                \
  } while (0)

/* Set interrupt flag - enable interrupts on this processor */
#define sti()                                                                                      \
  do {                                                                                             \
    asm volatile("sti" : : : "memory", "cc");                                                      \
  } while (0)

/* Restore flags
 * Puts the value in "flags" into the EFLAGS register.  Most often used
 * after a cli_and_save_flags(flags) */
#define restore_flags(flags)                                                                       \
  do {                                                                                             \
    asm volatile("                   \n\
            pushl %0                  \n\
            popfl                     \n\
            "                                                           \
                 :                                                                                 \
                 : "r"(flags)                                                                      \
                 : "memory", "cc");                                                                \
  } while (0)

#endif /* LIB_H */
