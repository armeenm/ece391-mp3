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

  for (i = 0; i < 31; ++i) {
    if ((i > 20 && i < 30) || i == 15)
      continue;

    if ((!idt[i].offset_15_00 && !idt[i].offset_31_16) || idt[i].seg_selector != KERNEL_CS ||
        idt[i].reserved4 || !idt[i].reserved3 || !idt[i].reserved2 || !idt[i].reserved1 ||
        !idt[i].size || idt[i].reserved0 || idt[i].dpl || !idt[i].present) {

      result = FAIL;
      assertion_failure();
    }
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
