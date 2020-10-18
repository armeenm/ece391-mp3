#include "tests.h"
#include "idt.h"
#include "lib.h"
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

  assertion_failure();

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

// add more tests here

/* Checkpoint 2 tests */
/* Checkpoint 3 tests */
/* Checkpoint 4 tests */
/* Checkpoint 5 tests */

/* Test suite entry point */
void launch_tests() {
  TEST_OUTPUT("idt_test", idt_test());
  // launch your tests here
}
