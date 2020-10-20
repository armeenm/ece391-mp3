#define IDT_C
#include "idt_asm.S"
#undef IDT_C

#include "idt.h"
#include "keyboard.h"
#include "lib.h"
#include "rtc.h"
#include "util.h"
#include "x86_desc.h"

/* Default exception handler */
#define EXC_DFL(name, str)                                                                         \
  void name(int eip, int UNUSED(cs), int eflags) {                                                 \
    cli();                                                                                         \
    clear();                                                                                       \
    printf("EXC: " str ": eip: 0x%x, eflags: 0x%x\n", eip, eflags);                                \
    for (;;)                                                                                       \
      ;                                                                                            \
  }

#define EXC_DFL_ERRC(name, str)                                                                    \
  void name(int errc, int eip, int UNUSED(cs), int eflags) {                                       \
    cli();                                                                                         \
    clear();                                                                                       \
    printf("EXC: " str ": errc: 0x%x, eip: 0x%x, eflags: 0x%x\n", errc, eip, eflags);              \
    for (;;)                                                                                       \
      ;                                                                                            \
  }

EXC_DFL(exc_de, "Divide-by-zero Error")
EXC_DFL(exc_db, "Debug")
EXC_DFL(exc_nmi, "Non-maskable Interrupt")
EXC_DFL(exc_bp, "Breakpoint")
EXC_DFL(exc_of, "Overflow")
EXC_DFL(exc_br, "Bound Range Exceeded")
EXC_DFL(exc_ud, "Invalid Opcode")
EXC_DFL(exc_nm, "Device Not Available")
EXC_DFL(exc_df, "Double Fault")
EXC_DFL(exc_cso, "Coprocessor Segment Overrun")
EXC_DFL(exc_ts, "Invalid TSS")
EXC_DFL(exc_np, "Segment Not Present")
EXC_DFL(exc_ss, "Stack-Segment Fault")
EXC_DFL_ERRC(exc_gp, "General Protection Fault")
EXC_DFL(exc_pf, "Page Fault")
EXC_DFL(exc_af, "(Debug) Assertion Failure")
EXC_DFL(exc_mf, "x87 Floating-Point Exception")
EXC_DFL(exc_ac, "Alignment Check")
EXC_DFL(exc_mc, "Machine Check")
EXC_DFL(exc_xf, "SIMD Floating-Point Exception")
EXC_DFL(exc_ve, "Virtualization Exception")
EXC_DFL(exc_sx, "Security Exception")

/* TODO: These need to be legitimate handlers */
EXC_DFL(irqh_pit, "PIT event!")

void irqh_syscall(void) {
  printf("Handling syscall...\n");
  for (;;)
    ;
}

typedef void (*IntHandler)(void);
static const IntHandler int_handlers[] = {asm_exc_de,
                                          asm_exc_db,
                                          asm_exc_nmi,
                                          asm_exc_bp,
                                          asm_exc_of,
                                          asm_exc_br,
                                          asm_exc_ud,
                                          asm_exc_nm,
                                          asm_exc_df,
                                          asm_exc_cso,
                                          asm_exc_ts,
                                          asm_exc_np,
                                          asm_exc_ss,
                                          asm_exc_gp,
                                          asm_exc_pf,
                                          asm_exc_af,
                                          asm_exc_mf,
                                          asm_exc_ac,
                                          asm_exc_mc,
                                          asm_exc_xf,
                                          asm_exc_ve,
                                          [0x1E] = asm_exc_sx,
                                          [PIT_IDT] = asm_irqh_pit,
                                          [KEYBOARD_IDT] = asm_irqh_keyboard,
                                          [RTC_IDT] = asm_irqh_rtc,
                                          [SYSCALL_IDT] = asm_irqh_syscall};

/* Various types used for make_idt_desc */
typedef enum Dpl { DPL0 = 0, DPL3 = 3 } Dpl;
typedef enum GateType { TASK = 5, INT = 6, TRAP = 7 } GateType;
typedef union GateTypeU {
  GateType val;

  struct {
    uint32_t reserved3 : 1;
    uint32_t reserved2 : 1;
    uint32_t reserved1 : 1;
  };

} GateTypeU;

static idt_desc_t NODISCARD CONST make_idt_desc(void const* handler, uint16_t seg_selector,
                                                GateType int_type, Dpl dpl) NONNULL(());

/**
 * make_idt_desc
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
                                GateType const int_type, Dpl const dpl) {

  GateTypeU const int_type_u = {.val = int_type};
  idt_desc_t ret;

  ret.seg_selector = seg_selector;
  ret.reserved4 = 0;                        /* Must always be 0 */
  ret.reserved3 = int_type_u.reserved3;     /* Setting the gate type */
  ret.reserved2 = int_type_u.reserved2;     /* ... */
  ret.reserved1 = int_type_u.reserved1;     /* ... */
  ret.size = 1;                             /* Always 32-bit */
  ret.reserved0 = (int_type_u.val == TASK); /* Gate type cont. */
  ret.dpl = dpl;                            /* Descriptor Privilege Level */
  ret.present = 1;                          /* Set to present */
  SET_IDT_ENTRY(ret, handler);              /* Set function address */

  return ret;
}

/**
 * init_idt
 * Description: Initializes and loads the IDT array.
 * Inputs: None
 * Outputs: None
 * Return: None
 * Side Effects: Modified the `idt` array and loads it into the processor with `lidt`.
 */
void init_idt(void) {
  uint16_t i;

  /* Configure first 32 IDT entries (exceptions)
   * Defined by Intel
   * Read Appendix D for more information
   */
  for (i = 0; i < EXCEPTION_CNT; ++i)
    idt[i] = make_idt_desc(int_handlers[i], KERNEL_CS, INT, DPL0);

  idt[PIT_IDT] = make_idt_desc(int_handlers[PIT_IDT], KERNEL_CS, INT, DPL0);
  idt[KEYBOARD_IDT] = make_idt_desc(int_handlers[KEYBOARD_IDT], KERNEL_CS, INT, DPL0);
  idt[RTC_IDT] = make_idt_desc(int_handlers[RTC_IDT], KERNEL_CS, INT, DPL0);
  idt[SYSCALL_IDT] = make_idt_desc(int_handlers[SYSCALL_IDT], KERNEL_CS, INT, DPL3);

  lidt(&idt_desc_ptr);
}

