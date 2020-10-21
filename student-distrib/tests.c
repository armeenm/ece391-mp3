#include "tests.h"
#include "idt.h"
#include "keyboard.h"
#include "lib.h"
#include "options.h"
#include "paging.h"
#include "x86_desc.h"

#define PASS 1
#define FAIL 0

/* format these macros as you see fit */
#define TEST_HEADER                                                                                \
  printf("[TEST %s] Running %s at %s:%d\n", __FUNCTION__, __FUNCTION__, __FILE__, __LINE__)
#define TEST_OUTPUT(name, result)                                                                  \
  printf("[TEST %s] Result = %s\n", name, (result) ? "PASS" : "FAIL");

static inline void assertion_failure() {
  /* Use exception #15 for assertions, otherwise reserved by Intel */
  asm volatile("int $15");
}

/* Checkpoint 1 tests */

int idt_test_helper(int const i, int const reserved3, int const dpl) {
  return (!idt[i].offset_15_00 && !idt[i].offset_31_16) || (idt[i].seg_selector != KERNEL_CS) ||
         idt[i].reserved4 || (idt[i].reserved3 != reserved3) || !idt[i].reserved2 ||
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

int idt_test() {
  int i;
  int result = PASS;

  TEST_HEADER;

  /* Check the first 20 entires, skipping 15 (reserved) */
  for (i = 0; i < 21; ++i) {
    if (idt_test_helper(i, 0, 0)) {
      result = FAIL;
      assertion_failure();
    }
  }

  /* Only ones left are 30 (#SX), PIT, keyboard, RTC, and syscall entries */
  if (idt_test_helper(30, 0, 0) || idt_test_helper(PIT_IDT, 0, 0) ||
      idt_test_helper(KEYBOARD_IDT, 0, 0) || idt_test_helper(RTC_IDT, 0, 0) ||
      idt_test_helper(SYSCALL_IDT, 0, 3)) {

    result = FAIL;
    assertion_failure();
  }

  return result;
}

/* int deref(int * test)
 * Description: Dereference an integer pointer
 * Inputs: test - integer pointer
 * Return Value: integer value at that point
 * Side Effects: none
 */
int deref(int* test) { return *test; }

/* Paging Test
 * int page_test()
 * Asserts that paging is set up correctly
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: None
 * Coverage: Paging, null dereference, negative pointer deref, kernel space, userspace,
 */
int page_test() {
  int result = PASS;

  /* Access NULL pointer */
#if NULL_PTR_TEST
  deref((int*)0);
#endif

  /* Access negative pointer */
#if NEG_PTR_TEST
  deref((int*)-100);
#endif

  /* Access pointer in 8MiB - 4GiB range (userspace) */
#if USERSPACE_PTR_TEST
  deref((int*)0x10000000);
#endif

#if VID_MEM_EDGE_CASE_TEST
  /* Access video memory */
  deref((int*)0xB7FFF);
#endif

  /* Access pointer in kernel space (5MiB) */
  deref((int*)0x4C4B40);

  /* Access video memory */
  deref((int*)0xB8000);

  /* Check paging structures */
  {
    uint32_t i;

    // Check 4KB page directory entry for valid address + permission bis (because the CPU can set
    // other bits)
    if (!(page_directory[0] & PRESENT) || !(page_directory[0] & READ_WRITE) ||
        (page_directory[0] & USER_ACCESS) ||
        ((int)page_table | page_directory[0]) != page_directory[0]) {
      result = FAIL;
      assertion_failure();
    }

    // Check kernel entry for valid address + permission bits
    if (!(page_directory[1] & PRESENT) || !(page_directory[1] & READ_WRITE) ||
        (page_directory[1] & USER_ACCESS) || !(page_directory[1] & FOUR_MEG_SIZE) ||
        !(page_directory[1] & FOUR_MEG_ADDRESS_ONE)) {
      result = FAIL;
      assertion_failure();
    }

    // Check the page table entry including video memory
    for (i = 0; i < PAGE_TABLE_SIZE; ++i) {
      if (i == VIDEO_MEMORY_START) {
        if (!(page_table[i] & PRESENT) || !(page_table[i] & READ_WRITE)) {
          result = FAIL;
          assertion_failure();
        }
        // We can only assume this because the CPU should not touch non-present locations
      } else if (page_table[i] != ((i * PTE_SIZE) | READ_WRITE)) {
        result = FAIL;
        assertion_failure();
      }
    }

    // Check that the 4MB to 4GB range has the correct bits set (4MB entries, not present), no
    // adddress yet
    for (i = 2; i < PAGE_DIRECTORY_SIZE; i++) {
      if (page_directory[i] != (READ_WRITE | USER_ACCESS | FOUR_MEG_SIZE)) {
        result = FAIL;
        assertion_failure();
      }
    }
  }

  /* Memory sanity check */
  int b = 391;
  int* a = &b;

  if (b != deref(a)) {
    result = FAIL;
    assertion_failure();
  }

  return result;
}

/* Handle Keypress
 * int handle_keypress_test()
 * Prints all valid scancode characters to the screen asserts the correct final location on the
 * assumption we started from top-left
 * Inputs: None
 * Outputs: PASS/FAIl
 * Side Effects: Prints all valid keyboard characters
 * Coverage: Tests large negative to large postiive scancode inputs
 */
int handle_keypress_test() {
  int i;
  int result = PASS;
  int const start_x = get_screen_x();
  int const start_y = get_screen_y();

  putc(' ');

  for (i = -391; i < 391; i++)
    handle_keypress(i);

  /* Last character positions from start postions, down 3 rows, 38 across corner */
  if (get_screen_x() - start_x != last_x_pos || get_screen_y() - start_y != last_y_pos)
    result = FAIL;

  putc('\n');

  return result;
}

/* int div_zero_except_test()
 *  force div by zero exception to occur, if not handled, test will fail
 * Inputs: None
 * Outputs: Exception handled or FAIL
 * Side Effects: compiler warning
 * Coverage: vector 0x0
 */
int div_zero_test() {
  /* Fool the compiler warnings */
  int y = 0;
  int const x = 391 / y;
  (void)x;

  return FAIL;
}

/* int invalid_opcode()
 *  force invalid op-code exception to occur, if not handled, test will fail
 * Inputs: None
 * Outputs: Exception handled or FAIL
 * Side Effects: Causes a CPU exception to be thrown
 * Coverage: Vector 0x6
 */
int invalid_opcode_test() {
  asm volatile("ud2");
  return FAIL;
}

// add more tests here

/* Checkpoint 2 tests */
/* Checkpoint 3 tests */
/* Checkpoint 4 tests */
/* Checkpoint 5 tests */

/* Test suite entry point */
void launch_tests() {
#if TESTS_ENABLED
  TEST_OUTPUT("idt_test", idt_test());
  TEST_OUTPUT("page_test", page_test());
  TEST_OUTPUT("handle_keypress_test", handle_keypress_test());
#endif
#if DIV_ZERO_TEST
  TEST_OUTPUT("div_zero_test", div_zero_test());
#endif

#if INVALID_OPCODE_TEST
  TEST_OUTPUT("invalid_opcode_test", invalid_opcode_test());
#endif
}
