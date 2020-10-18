#include "tests.h"
#include "idt.h"
#include "lib.h"
#include "keyboard.h"
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
    if (idt_test_helper(i, 1, 0)) {
      result = FAIL;
      assertion_failure();
    }
  }

  /* Only ones left are 30 (#SX), PIT, keyboard, RTC, and syscall entries */
  if (idt_test_helper(30, 1, 0) || idt_test_helper(PIT_IDT, 0, 0) ||
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
int deref(int * test)
{
  return *test;
}

/* Paging Test
 * int page_test()
 * Asserts that paging is set up correctly
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: None
 * Coverage: Paging, null dereference, negative pointer deref, kernel space, userspace, 
 */
int page_test()
{
  int result = PASS;
  /* Access null pointer */
  //deref((int *)0);
  /* Access negative pointer */
  //deref((int *)-100);
  /* Access Video memory*/
  //deref((int *)0xB8000);
  /* Access pointer in kernel space (5mb) */
  //deref((int *)0x4C4B40);
  /* Access pointer in 8mb - 4gb range (userspace) */
  //deref((int *)0x10000000);

  int b = 391;
  int * a = &b;

  if(b != deref(a))
  {
    result = FAIL;
    assertion_failure();
  }

  return result;
}

/* handle Keypress
 * int handle_keypress_test()
 * Prints all valid scancode characters to the screen asserts the correct final location on the assumption
 * we started from top-left
 * Inputs: None
 * Outputs: PASS/FAIl
 * Side Effects: Prints all valid keyboard characters
 * Coverage: Tests large negative to large postiive scancode inputs
 */
int handle_keypress_test() {
  int i;
  int result = PASS;
  int start_x, start_y;

  start_x = get_screen_x();
  start_y = get_screen_y();

  putc(' ');
  for (i = -391; i<391; i++) {
    handle_keypress(i);
  }
  // Last character postions from start postions, down 3 rows, 38 across corner
  if (get_screen_x()-start_x != last_x_pos || get_screen_y()-start_y != last_y_pos) {
    result = FAIL;
  }
  printf("\n");
  return result;
}

/* int div_zero_except_test() 
 *  force div by zero exception to occur, if not handled, test will fail
 * Inputs: None
 * Outputs: Exception handled or FAIL
 * Side Effects: compiler warning
 * Coverage: vector 0x0
 */
int div_zero_except_test() {
  int x = 391/0;
  printf("%d", x);
  return FAIL;
}

/* int invalid_opcode()
 *  force invalid op-code exception to occur, if not handled, test will fail
 * Inputs: None
 * Outputs: Exception handled or FAIL
 * Side Effects: none
 * Coverage: vector 0x6
 */
int invalid_opcode() {
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
  TEST_OUTPUT("idt_test", idt_test());
  TEST_OUTPUT("Paging Test", page_test());
  TEST_OUTPUT("Handle Keypress", handle_keypress_test());
  // TEST_OUTPUT("Divide by zero", div_zero());
  // TEST_OUTPUT("Invalid Ppcode", invalid_opcode());
  // launch your tests here
}
