#include "idt.h"
#include "lib.h"
#include "util.h"

/* Default exception handler */
#define EXC_DFL(name, str)                                                                         \
  void name(void) {                                                                                \
    printf("EXCEPTION: " str "\n");                                                                \
    for (;;)                                                                                       \
      ;                                                                                            \
  }

EXC_DFL(exc_de, "Divide-by-zero Error");
EXC_DFL(exc_db, "Debug");
EXC_DFL(exc_nmi, "Non-maskable Interrupt");
EXC_DFL(exc_bp, "Breakpoint");
EXC_DFL(exc_of, "Overflow");
EXC_DFL(exc_br, "Bound Range Exceeded");
EXC_DFL(exc_ud, "Invalid Opcode");
EXC_DFL(exc_nm, "Device Not Available");
EXC_DFL(exc_df, "Double Fault");
EXC_DFL(exc_ts, "Invalid TSS");
EXC_DFL(exc_np, "Segment Not Present");
EXC_DFL(exc_ss, "Stack-Segment Fault");
EXC_DFL(exc_gp, "General Protection Fault");
EXC_DFL(exc_pf, "Page Fault");
EXC_DFL(exc_mf, "x87 Floating-Point Exception");
EXC_DFL(exc_ac, "Alignment Check");
EXC_DFL(exc_mc, "Machine Check");
EXC_DFL(exc_xf, "SIMD Floating-Point Exception");
EXC_DFL(exc_ve, "Virtualization Exception");
EXC_DFL(exc_sx, "Security Exception");

void syscall_handler() {
  printf("Handling syscall...\n");
  for (;;)
    ;
}

typedef void (*IntHandler)(void);
static const IntHandler int_handlers[] = {
    exc_de,         exc_db,          exc_nmi,
    exc_bp,         exc_of,          exc_br,
    exc_ud,         exc_nm,          exc_df,
    [0xA] = exc_ts, exc_np,          exc_ss,
    exc_gp,         exc_pf,          [0x10] = exc_mf,
    exc_ac,         exc_mc,          exc_xf,
    exc_ve,         [0x1E] = exc_sx, [SYSCALL_IDT_IDX] = syscall_handler};

/* Various types used for make_idt_desc */
typedef enum Dpl { DPL0 = 0, DPL3 = 3 } Dpl;
typedef enum IntType { TASK = 2, SYSCALL = 3, EXCEPTION = 5 } IntType;
typedef union IntTypeU {
  IntType val;

  struct {
    uint32_t reserved3 : 1;
    uint32_t reserved2 : 1;
    uint32_t reserved1 : 1;
  };

} IntTypeU;

static idt_desc_t NODISCARD CONST make_idt_desc(void const* handler, uint16_t seg_selector,
                                                IntType int_type, Dpl dpl) NONNULL(());

/**
 * idt_desc_t
 * Description: Creates an IDT descriptor entry with the specified parameters.
 * Inputs: handler      -- Handler function to call upon interrupt.
 *         seg_selector -- Segment selector.
 *         int_type     -- Type of IDT entry.
 *         dpl          -- Descriptor privilege level.
 * Outputs: None
 * Return: IDT descriptor.
 * Side Effects: None
 */
static idt_desc_t make_idt_desc(void const* const handler, uint16_t const seg_selector,
                                IntType const int_type, Dpl const dpl) {
  IntTypeU const int_type_u = {.val = int_type};
  idt_desc_t ret = {.seg_selector = seg_selector,
                    .reserved4 = 0,
                    .reserved3 = int_type_u.reserved3,
                    .reserved2 = int_type_u.reserved2,
                    .reserved1 = int_type_u.reserved1,
                    .size = 1,
                    .reserved0 = (int_type_u.val == TASK),
                    .dpl = dpl,
                    .present = 1};

  SET_IDT_ENTRY(ret, handler);

  return ret;
}

void init_idt(void) {
  uint16_t i;

  /* Configure first 32 IDT entries (exceptions)
   * Defined by Intel
   * Read Appendix D for more information
   */
  for (i = 0; i < EXCEPTION_COUNT; ++i)
    idt[i] = make_idt_desc(int_handlers[i], KERNEL_CS, EXCEPTION, DPL0);

  /* Syscalls must be callable from ring 3 */
  idt[SYSCALL_IDT_IDX] = make_idt_desc(int_handlers[SYSCALL_IDT_IDX], KERNEL_CS, SYSCALL, DPL3);
}
