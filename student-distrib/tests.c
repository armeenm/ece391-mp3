#include "tests.h"
#include "fs.h"
#include "idt.h"
#include "keyboard.h"
#include "lib.h"
#include "options.h"
#include "paging.h"
#include "rtc.h"
#include "syscall.h"
#include "terminal_driver.h"
#include "util.h"
#include "x86_desc.h"

enum { FAIL, PASS };

#define TEST_FAIL_MSG(str, ...)                                                                    \
  do {                                                                                             \
    clear();                                                                                       \
    printf("[TEST %s] FAIL: %s:%d; " str "\n", __FUNCTION__, __FILE__, __LINE__, ##__VA_ARGS__);   \
    asm volatile("int $15");                                                                       \
  } while (0)

#define TEST_FAIL TEST_FAIL_MSG("")

#define TEST_PASS printf("[TEST %s] PASS\n", __FUNCTION__)

#define TEST(name)                                                                                 \
  static void TEST_##name(void);                                                                   \
  static void TEST_##name(void) {                                                                  \
    if (!ENABLE_TEST_##name)                                                                       \
      return;                                                                                      \
    clear();                                                                                       \
    printf("[TEST %s] Running at %s:%d\n", __FUNCTION__, __FILE__, __LINE__);

#define TEST_END                                                                                   \
  TEST_PASS;                                                                                       \
  }

#define TEST_END_FAIL                                                                              \
  TEST_FAIL;                                                                                       \
  }

enum {
  PDE_USED = PG_PRESENT | PG_RW | PG_USPACE | PG_SIZE,
  PDE_USED_4K = PDE_USED | 0xFFFFF000,
  PDE_USED_4M = PDE_USED | 0xFFC00000
};

/***** CHECKPOINT 1 {{{ *****/

/* Helpers */
static int deref(u32 test);
static i32 NODISCARD PURE idt_test_helper(u32 i, GateType gate_type, Dpl dpl);

/* Tests */

static int deref(u32 test) { return *(int*)test; }

static i32 idt_test_helper(u32 const i, GateType const gate_type, Dpl const dpl) {
  GateTypeU const gate_type_u = {.val = gate_type};

  return (!idt[i].offset_15_00 && !idt[i].offset_31_16) || (idt[i].seg_selector != KERNEL_CS) ||
         idt[i].reserved4 || (idt[i].reserved3 != gate_type_u.reserved3) || !idt[i].reserved2 ||
         !idt[i].reserved1 || !idt[i].size || idt[i].reserved0 || (idt[i].dpl != dpl) ||
         !idt[i].present;
}

/* IDT Test
 *
 * Asserts that IDT exception entries are correct
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: None
 * Coverage: Load IDT, IDT definition
 * Files: x86_desc.h/S
 */

TEST(IDT) {
  u32 i;

  /* Check the first 21 entires */
  for (i = 0; i < 21; ++i)
    if (idt_test_helper(i, INT, DPL0))
      TEST_FAIL;

  /* Only ones left are 30 (#SX), PIT, keyboard, RTC, and syscall entries */
  if (idt_test_helper(30, INT, DPL0) || idt_test_helper(IDT_PIT, INT, DPL0) ||
      idt_test_helper(IDT_KEYBOARD, INT, DPL0) || idt_test_helper(IDT_RTC, INT, DPL0) ||
      idt_test_helper(IDT_SYSCALL, TRAP, DPL3))
    TEST_FAIL;

  TEST_END;
}

TEST(NULLPTR) {
  deref(0);
  TEST_END_FAIL;
}

TEST(USPACE_PTR) {
  deref(0x10000000);
  TEST_END_FAIL;
}

TEST(VIDMEM_EDGE) {
  deref(0xB7FFF);
  TEST_END_FAIL;
}

/* Paging Test
 * int page_test()
 * Asserts that paging is set up correctly
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: None
 * Coverage: Paging, null dereference, negative pointer deref, kernel space, userspace,
 */
TEST(PAGING) {
  u32 i;

  /* Access pointer in kernel space (5MiB) */
  deref(0x4C4B40);

  /* Access video memory */
  deref(0xB8000);

  /* Check paging structures */
  /* Check 4KB page directory entry for valid address + permission bits (because the CPU can set
   * other bits) */

  if ((pgdir[0][0] & PDE_USED_4K) != (PG_PRESENT | PG_RW | (u32)pgtbl))
    TEST_FAIL;

  /* Check kernel entry for valid address + permission bits */
  if ((pgdir[0][1] & PDE_USED_4M) != (PG_PRESENT | PG_RW | PG_SIZE | PG_4M_START))
    TEST_FAIL;

  /* Check nullptr region */
  if ((pgtbl[0] & PDE_USED_4K) != PG_RW)
    TEST_FAIL;

  /* Check the page table entry including video memory */
  for (i = 1; i < PGTBL_LEN; ++i)
    if (i == PG_VIDMEM_START) {
      if ((pgtbl[i] & PDE_USED) != (PG_PRESENT | PG_RW))
        TEST_FAIL_MSG("i: %u", i);
    }

    /* We can only assume this because the CPU should not touch non-present locations */
    else if ((pgtbl[i] & PDE_USED_4K) != ((i * PTE_SIZE) | PG_RW | PG_PRESENT))
      TEST_FAIL_MSG("i: %u", i);

  /* Check that the 4MB to 4GB range has the correct bits set (4MB entries, not present), no
   * address yet */
  for (i = 2; i < PGDIR_LEN; ++i)
    if ((pgdir[0][i] & PDE_USED_4M & ~1U) != ((i * PG_4M_START) | PG_RW | PG_USPACE | PG_SIZE))
      TEST_FAIL_MSG("i: %u", i);

  /* Memory sanity check */
  int b = 391;
  int* a = &b;

  if (b != *a)
    TEST_FAIL;

  TEST_END;
}

/* int div_zero_except_test()
 *  force div by zero exception to occur, if not handled, test will fail
 * Inputs: None
 * Outputs: Exception handled or FAIL
 * Side Effects: compiler warning
 * Coverage: vector 0x0
 */
TEST(DIV_ZERO) {
  /* Fool the compiler warnings */
  int y = 0;
  // NOLINTNEXTLINE("div zero")
  int const x = 391 / y;
  (void)x;

  TEST_END_FAIL;
}

/* int invalid_opcode()
 *  force invalid op-code exception to occur, if not handled, test will fail
 * Inputs: None
 * Outputs: Exception handled or FAIL
 * Side Effects: Causes a CPU exception to be thrown
 * Coverage: Vector 0x6
 */
TEST(UD) {
  asm volatile("ud2");

  TEST_END_FAIL;
}

/***** }}} CHECKPOINT 1 *****/

/***** CHECKPOINT 2 {{{ *****/

/* Handle Keypress
 * int handle_keypress_test()
 * Prints all valid scancode characters to the screen asserts the correct final location on the
 * assumption we started from top-left
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: Prints all valid keyboard characters
 * Coverage: Tests large negative to large postiive scancode inputs
 */
TEST(KEYPRESS) {
  {
    int NUM_COLS = 80;
    char* video_mem = (char*)(0xB8000);

    putc(' ');

    int start_x = get_screen_x();
    int start_y = get_screen_y();

    handle_keypress(-1);

    if (start_x != get_screen_x())
      TEST_FAIL;

    if (*(u8*)(video_mem + ((NUM_COLS * start_y + start_x) << 1)) != ' ')
      TEST_FAIL;

    terminal_read_flag = 1;

    handle_keypress(SCS1_PRESSED_A);
    handle_keypress(SCS1_RELEASED_A);

    if (start_x + 1 != get_screen_x()) {
      TEST_FAIL;
    }

    if (*(u8*)(video_mem + ((NUM_COLS * start_y + start_x) << 1)) != 'a') {
      TEST_FAIL;
    }

    handle_keypress(SCS1_PRESSED_LEFTSHIFT);

    start_x = get_screen_x();
    start_y = get_screen_y();

    handle_keypress(SCS1_PRESSED_A);
    handle_keypress(SCS1_RELEASED_A);

    if (*(u8*)(video_mem + ((NUM_COLS * start_y + start_x) << 1)) != 'A')
      TEST_FAIL;

    handle_keypress(SCS1_RELEASED_LEFTSHIFT);

    handle_keypress(SCS1_PRESSED_LEFTSHIFT);

    start_x = get_screen_x();
    start_y = get_screen_y();

    handle_keypress(SCS1_PRESSED_1);
    handle_keypress(SCS1_RELEASED_1);

    if (*(u8*)(video_mem + ((NUM_COLS * start_y + start_x) << 1)) != '!')
      TEST_FAIL;

    handle_keypress(SCS1_RELEASED_LEFTSHIFT);

    handle_keypress(SCS1_PRESSED_CAPSLOCK);
    handle_keypress(SCS1_RELEASED_CAPSLOCK);

    handle_keypress(SCS1_PRESSED_CAPSLOCK);
    handle_keypress(SCS1_RELEASED_CAPSLOCK);

    handle_keypress(SCS1_PRESSED_CAPSLOCK);
    handle_keypress(SCS1_RELEASED_CAPSLOCK);

    start_x = get_screen_x();
    start_y = get_screen_y();

    handle_keypress(SCS1_PRESSED_C);
    handle_keypress(SCS1_RELEASED_C);

    if (*(u8*)(video_mem + ((NUM_COLS * start_y + start_x) << 1)) != 'C')
      TEST_FAIL;

    handle_keypress(SCS1_PRESSED_CAPSLOCK);
    handle_keypress(SCS1_RELEASED_CAPSLOCK);

    clear_line_buf();

    terminal_read_flag = 0;
  }

  putc('\n');

  TEST_END;
}

TEST(TERMINAL) {
  char buf[128] = {0};

  terminal_open(0);

  if (terminal_read(0, buf, -1) != -1)
    TEST_FAIL;
  if (terminal_write(0, "TEST", -1) != -1)
    TEST_FAIL;

  terminal_close(0);

  TEST_END;
}

TEST(SHELL) {
  i32 size;
  i8 buf[128] = {0};
  i8 const* const s = "Input: ";

  terminal_open(0);
  for (;;) {
    terminal_write(0, SHELL_PS1, sizeof(SHELL_PS1));
    size = terminal_read(0, buf, 128);
    terminal_write(0, s, (i32)strlen(s));
    terminal_write(0, buf, size);
  }

  terminal_close(0);

  TEST_END;
}

TEST(LS) {
  char fname_buf[33] = {0};

  dir_open(0);

  while (dir_read(0, fname_buf, 32) > 0) {
    printf("file_name: %s\n", fname_buf);
    memset(fname_buf, 0, 32);
  }

  TEST_END;
}

TEST(CAT_FRAME0) {
  char buf[200] = {0};

  int fd = open((u8*)"frame0.txt");
  read(fd, (u8*)buf, 0);
  printf("File: frame0.txt\n%s\n", buf);

  TEST_END;
}

TEST(CAT_VLTWLN) {
  char buf[30000] = {0};

  int fd = open((u8*)"verylargetextwithverylongname.txt");
  read(fd, (u8*)buf, 0);
  printf("File: verylargetextwithverylongname.txt\n%s\n", buf);

  TEST_END;
}

TEST(CAT_HELLO) {
  char buf[30000] = {0};

  int fd = open((u8*)"hello");
  read(fd, (u8*)buf, 0);
  printf("File: hello\n");
  buf[4] = '\0';
  printf("First 4 bytes: %s\n", buf);
  printf("Last 36 bytes: %s\n", &buf[5349 - 37]);

  TEST_END;
}

TEST(RTC_DEMO) {
  int i, j;
  int freq;

  rtc_open(0);

  for (j = 1; j <= 10; ++j) {
    freq = 1 << j;

    /* Print 8 chars for 2Hz, print 16 for 4Hz (4 seconds per RTC) */
    rtc_write(0, &freq, sizeof(int));

    for (i = 0; i < 1 << (2 + j); ++i) {
      printf("%d", 1);
      rtc_read(0, 0, 4);
    }
  }

  TEST_END;
}

TEST(RTC_WRITE) {
  i32 i;

  rtc_open(0);

  // This also checks against set_virt_freq because it's just a wrapper
  if (rtc_write(0, 0, sizeof(i32)) != -1) // Null pointer rtc_write
    TEST_FAIL;

  i = -128;
  if (rtc_write(0, &i, sizeof(i32)) != -1) // negative freq
    TEST_FAIL;

  i = 2048;
  if (rtc_write(0, &i, sizeof(i32)) != -1) // higher than max
    TEST_FAIL;

  i = 391;
  if (rtc_write(0, &i, sizeof(i32)) != -1) // Valid range, not power of 2
    TEST_FAIL;

  i = 256;
  if (rtc_write(0, &i, sizeof(i32)) != sizeof(i32)) // valid range and power of 2
    TEST_FAIL;

  i = 256;
  if (set_virtual_freq_rtc((u32)i) != 0) // valid range and power of 2
    TEST_FAIL;

  i = 391;
  if (set_virtual_freq_rtc((u32)i) != -1) // valid range and power of 2
    TEST_FAIL;

  rtc_close(0);

  TEST_END;
}

TEST(RTC_READ) {
  rtc_open(0);
  if (rtc_read(0, 0, 0)) // Read always works, 2Hz
    TEST_FAIL;
  rtc_close(0);

  TEST_END;
}

TEST(FS) {
  if (file_open(0))
    TEST_FAIL;

  if (dir_open(0))
    TEST_FAIL;

  if (file_close(0))
    TEST_FAIL;

  if (dir_close(0))
    TEST_FAIL;

  if (file_write(0, 0, 0) != -1)
    TEST_FAIL;

  if (dir_write(0, 0, 0) != -1)
    TEST_FAIL;

  if (file_read(0, 0, 0) != -1)
    TEST_FAIL;

  if (dir_read(0, 0, -1) != -1)
    TEST_FAIL;

  if (read_dentry_by_name(0, 0) != -1)
    TEST_FAIL;

  if (read_dentry_by_index(0, 0) != -1)
    TEST_FAIL;

  if (read_data(0, 0, 0, 0) != -1)
    TEST_FAIL;

  TEST_END;
}

/***** }}} CHECKPOINT 2 *****/

/***** CHECKPOINT 3 {{{ *****/

TEST(EXEC_LS) {
  if (execute((u8*)"ls"))
    TEST_FAIL;

  TEST_END;
}

/***** }}} CHECKPOINT 3 *****/

/* Test suite entry point */
void launch_tests(void) {
#if TESTS_ENABLED
  TEST_DIV_ZERO();
  TEST_UD();
  TEST_NULLPTR();
  TEST_USPACE_PTR();
  TEST_VIDMEM_EDGE();

  TEST_IDT();
  TEST_PAGING();
  TEST_FS();
  TEST_TERMINAL();
  TEST_KEYPRESS();
  TEST_RTC_DEMO();
  TEST_RTC_WRITE();
  TEST_RTC_READ();
  TEST_LS();
  TEST_CAT_FRAME0();
  TEST_CAT_VLTWLN();
  TEST_CAT_HELLO();
  TEST_SHELL();

  TEST_EXEC_LS();
#endif
}
