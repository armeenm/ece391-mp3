#include "tests.h"
#include "fs.h"
#include "idt.h"
#include "keyboard.h"
#include "lib.h"
#include "options.h"
#include "paging.h"
#include "rtc.h"
#include "terminal_driver.h"
#include "util.h"
#include "x86_desc.h"

enum { FAIL, PASS };

/* format these macros as you see fit */
#define TEST_HEADER                                                                                \
  printf("[TEST %s] Running %s at %s:%d\n", __FUNCTION__, __FUNCTION__, __FILE__, __LINE__)

#define TEST_PASS printf("[TEST %s] PASS\n", __FUNCTION__);

#define TEST_FAIL_MSG(str, ...)                                                                    \
  do {                                                                                             \
    clear();                                                                                       \
    printf("[TEST %s] FAIL: %s:%d; " str "\n", __FUNCTION__, __FILE__, __LINE__, ##__VA_ARGS__);   \
    asm volatile("int $15");                                                                       \
  } while (0)
#define TEST_FAIL TEST_FAIL_MSG("")

/* Checkpoint 1 tests */

enum {
  PDE_USED = PG_PRESENT | PG_RW | PG_USPACE | PG_SIZE,
  PDE_USED_4K = PDE_USED | 0xFFFFF000,
  PDE_USED_4M = PDE_USED | 0xFFC00000
};

int idt_test_helper(int const i, GateType const gate_type, Dpl const dpl) {
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

void idt_test() {
  int i;

  TEST_HEADER;

  /* Check the first 21 entires */
  for (i = 0; i < 21; ++i)
    if (idt_test_helper(i, INT, DPL0))
      TEST_FAIL;

  /* Only ones left are 30 (#SX), PIT, keyboard, RTC, and syscall entries */
  if (idt_test_helper(30, INT, DPL0) || idt_test_helper(IDT_PIT, INT, DPL0) ||
      idt_test_helper(IDT_KEYBOARD, INT, DPL0) || idt_test_helper(IDT_RTC, INT, DPL0) ||
      idt_test_helper(IDT_SYSCALL, INT, DPL3))
    TEST_FAIL;

  TEST_PASS;
}

/* int deref(int * test)
 * Description: Dereference an integer pointer
 * Inputs: test - integer pointer
 * Return Value: integer value at that point
 * Side Effects: none
 */
int deref(uint32_t test) { return *(int*)test; }

/* Paging Test
 * int page_test()
 * Asserts that paging is set up correctly
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: None
 * Coverage: Paging, null dereference, negative pointer deref, kernel space, userspace,
 */
void page_test() {
  uint32_t i;

  TEST_HEADER;

  /* Access NULL pointer */
#if NULLPTR_TEST
  deref(0);
#endif

  /* Access pointer in 8MiB - 4GiB range (userspace) */
#if USPACE_PTR_TEST
  deref(0x10000000);
#endif

#if VIDMEM_EDGE_TEST
  /* Access video memory */
  deref(0xB7FFF);
#endif

  /* Access pointer in kernel space (5MiB) */
  deref(0x4C4B40);

  /* Access video memory */
  deref(0xB8000);

  /* Check paging structures */
  /* Check 4KB page directory entry for valid address + permission bits (because the CPU can set
   * other bits) */

  if ((pgdir[0] & PDE_USED_4K) != (PG_PRESENT | PG_RW | (uint32_t)pgtbl))
    TEST_FAIL;

  /* Check kernel entry for valid address + permission bits */
  if ((pgdir[1] & PDE_USED_4M) != (PG_PRESENT | PG_RW | PG_SIZE | PG_4M_START))
    TEST_FAIL;

  /* Check the page table entry including video memory */
  for (i = 0; i < PGTBL_LEN; ++i)
    if (i == PG_VIDMEM_START) {
      if ((pgtbl[i] & PDE_USED) != (PG_PRESENT | PG_RW))
        TEST_FAIL_MSG("i: %u", i);
    }

    /* We can only assume this because the CPU should not touch non-present locations */
    else if (pgtbl[i] != ((i * PTE_SIZE) | PG_RW))
      TEST_FAIL_MSG("i: %u", i);

  /* Check that the 4MB to 4GB range has the correct bits set (4MB entries, not present), no
   * address yet */
  for (i = 2; i < PGDIR_LEN; ++i)
    if (pgdir[i] != (PG_RW | PG_USPACE | PG_SIZE))
      TEST_FAIL_MSG("i: %u", i);

  /* Memory sanity check */
  int b = 391;
  int* a = &b;

  if (b != *a)
    TEST_FAIL;

  TEST_PASS;
}

/* Handle Keypress
 * int handle_keypress_test()
 * Prints all valid scancode characters to the screen asserts the correct final location on the
 * assumption we started from top-left
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: Prints all valid keyboard characters
 * Coverage: Tests large negative to large postiive scancode inputs
 */
void handle_keypress_test() {
  int i;

  TEST_HEADER;

  {
    int const start_x = get_screen_x();
    int const start_y = get_screen_y();

    putc(' ');

    for (i = -391; i < 391; i++)
      handle_keypress(i);

    /* Last character positions from start postions, down 3 rows, 38 across corner */
    if (get_screen_x() - start_x != last_x_pos || get_screen_y() - start_y != last_y_pos)
      TEST_FAIL;
  }

  putc('\n');

  TEST_PASS;
}

int terminal_test() {
  terminal_open();
  char buf[128];
  while (1 == 1) {
    terminal_write("ece391> ", 8);
    int size = terminal_read(buf, 128);
    terminal_write("Input was : ", 12);
    terminal_write(buf, size);
  }

  terminal_close();
}

/* int div_zero_except_test()
 *  force div by zero exception to occur, if not handled, test will fail
 * Inputs: None
 * Outputs: Exception handled or FAIL
 * Side Effects: compiler warning
 * Coverage: vector 0x0
 */
void div_zero_test() {
  TEST_HEADER;

  {
    /* Fool the compiler warnings */
    int y = 0;
    int const x = 391 / y;
    (void)x;
  }

  TEST_FAIL;
}

/* int invalid_opcode()
 *  force invalid op-code exception to occur, if not handled, test will fail
 * Inputs: None
 * Outputs: Exception handled or FAIL
 * Side Effects: Causes a CPU exception to be thrown
 * Coverage: Vector 0x6
 */
void invalid_opcode_test() {
  TEST_HEADER;

  asm volatile("ud2");

  TEST_FAIL;
}

/* Checkpoint 2 tests */

void ls_test() {
  int8_t buf[32];

  TEST_HEADER;

  dir_read(buf);
  printf("%s\n", buf);

  TEST_PASS;
}

void rtc_test() {
  int i;
  int j;
  int freq;
  for (j = 1; j <= 10; j++) {
    freq = 1 << j;
    // print 8 chars for 2hz, print 16 for 4hz (4 seconds per RTC)
    clear();
    set_screen_xy(0, 0);
    rtc_write(0, &freq, sizeof(int));
    for (i = 0; i < 1 << (2 + j); i++) {
      printf("%d ", 1);
      rtc_read(0, 0, 0);
    }
  }
}
/* Checkpoint 3 tests */
/* Checkpoint 4 tests */
/* Checkpoint 5 tests */

/* Test suite entry point */
void launch_tests() {
  /* CP1 */
  /*
    idt_test();
    page_test();
    handle_keypress_test();

  #if DIV_ZERO_TEST
    div_zero_test();
  #endif

  #if INVALID_OPCODE_TEST
    invalid_opcode_test();
  #endif
  */

  /* CP2 */
  ls_test();
  idt_test();
  // page_test();
  // handle_keypress_test();
  // rtc_test();
#if DIV_ZERO_TEST
  div_zero_test();
#endif

#if INVALID_OPCODE_TEST
  invalid_opcode_test();
#endif
}
