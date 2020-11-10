#include "lib.h"
#include "syscall.h"

#define ASM_EXC(name) void asm_##name(void);
#define ASM_EXC_KEEPEAX(name) ASM_EXC(name)
#define I_ASM_EXC(name, unused) ASM_EXC(name)

/* Some macro magic to choose whether we clear or not */
#define CLR_true clear()
#define CLR_false

#define ERRC_true u32 errc
#define ERRC_false

#define INTSTACK_NAME_true IntStackE
#define INTSTACK_NAME_false IntStack

#define ERRC_PRINTF_true(str)                                                                      \
  printf("EXC: " str ": errc: 0x%x, eip: 0x%x, eflags: 0x%x\n", stack.errc, stack.eip, stack.eflags)
#define ERRC_PRINTF_false(str)                                                                     \
  printf("EXC: " str ": eip: 0x%x, eflags: 0x%x\n", stack.eip, stack.eflags)

#define INTSTACK(has_errc)                                                                         \
  typedef struct INTSTACK_NAME_##has_errc {                                                        \
    u32 flags;                                                                                     \
    u32 edx;                                                                                       \
    u32 ecx;                                                                                       \
    u32 eax;                                                                                       \
    ERRC_##has_errc;                                                                               \
    u32 eip;                                                                                       \
    u32 cs;                                                                                        \
    u32 eflags;                                                                                    \
    u32 esp;                                                                                       \
    u32 ss;                                                                                        \
  } PACKED INTSTACK_NAME_##has_errc

INTSTACK(true);
INTSTACK(false);

#define I_EXC_DFL(name, str, clear, errc)                                                          \
  ASM_EXC(name)                                                                                    \
  void name(INTSTACK_NAME_##errc stack);                                                           \
  void name(INTSTACK_NAME_##errc stack) {                                                          \
    CLR_##clear;                                                                                   \
    ERRC_PRINTF_##errc(str);                                                                       \
    halt(1);                                                                                       \
  }

/* Separate versions for whether we should clear the screen or not */
#define EXC_DFL(name, str) I_EXC_DFL(name, str, true, false)
#define EXC_DFL_NOCLR(name, str) I_EXC_DFL(name, str, false, false)

/* Certain versions for accepting an error code at the top of the stack -- mainly for GPF */
#define EXC_DFL_ERRC(name, str) I_EXC_DFL(name, str, true, true)
#define EXC_DFL_ERRC_NOCLR(name, str) I_EXC_DFL(name, str, false, true)

#define IDT_C

#include "idt_asm.S"

#undef ASM_EXC
#undef ASM_EXC_KEEPEAX
#undef I_ASM_EXC
#undef CLR_true
#undef CLR_false
#undef ERRC_true
#undef ERRC_false
#undef INTSTACK
#undef I_EXC_DFL
#undef EXC_DFL
#undef EXC_DFL_NOCLR
#undef EXC_DFL_ERRC
#undef EXC_DFL_ERRC_NOCLR
#undef IDT_C

#include "idt.h"
#include "lib.h"
#include "util.h"
#include "x86_desc.h"

typedef void (*IntHandler)(void);
static const IntHandler exc_handlers[IDT_EXC_CNT] = {
    asm_exc_de, asm_exc_db, asm_exc_nmi, asm_exc_bp,         asm_exc_of, asm_exc_br,
    asm_exc_ud, asm_exc_nm, asm_exc_df,  asm_exc_cso,        asm_exc_ts, asm_exc_np,
    asm_exc_ss, asm_exc_gp, asm_exc_pf,  asm_exc_af,         asm_exc_mf, asm_exc_ac,
    asm_exc_mc, asm_exc_xf, asm_exc_ve,  [0x1E] = asm_exc_sx};

static NODISCARD CONST idt_desc_t make_idt_desc(void const* handler, u16 seg_selector,
                                                GateType gate_type, Dpl dpl) NONNULL(());

/**
 * make_idt_desc
 * Description: Creates an IDT descriptor entry with the specified parameters.
 * Inputs: handler      -- Handler function to call upon interrupt.
 *         seg_selector -- Segment selector.
 *         gate_type     -- Type of IDT entry.
 *         dpl          -- Descriptor privilege level.
 * Outputs: None
 * Return: IDT descriptor.
 * Side Effects: None
 */
static idt_desc_t make_idt_desc(void const* const handler, u16 const seg_selector,
                                GateType const gate_type, Dpl const dpl) {

  GateTypeU const gate_type_u = {.val = gate_type};
  idt_desc_t ret;

  ret.seg_selector = seg_selector;
  ret.reserved4 = 0;                         /* Must always be 0 */
  ret.reserved3 = gate_type_u.reserved3;     /* Setting the gate type */
  ret.reserved2 = gate_type_u.reserved2;     /* ... */
  ret.reserved1 = gate_type_u.reserved1;     /* ... */
  ret.size = 1;                              /* Always 32-bit */
  ret.reserved0 = (gate_type_u.val == TASK); /* Gate type cont. */
  ret.dpl = dpl;                             /* Descriptor Privilege Level */
  ret.present = 1;                           /* Set to present */
  SET_IDT_ENTRY(ret, handler);               /* Set function address */

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
  u16 i;

  /* Configure first 32 IDT entries (exceptions)
   * Defined by Intel
   * Read Appendix D for more information
   */
  for (i = 0; i < IDT_EXC_CNT; ++i)
    idt[i] = make_idt_desc(exc_handlers[i], KERNEL_CS, INT, DPL0);

  /* PIT */
  idt[IDT_PIT] = make_idt_desc(asm_irqh_pit, KERNEL_CS, INT, DPL0);

  /* Keyboard */
  idt[IDT_KEYBOARD] = make_idt_desc(asm_irqh_keyboard, KERNEL_CS, INT, DPL0);

  /* RTC */
  idt[IDT_RTC] = make_idt_desc(asm_irqh_rtc, KERNEL_CS, INT, DPL0);

  /* Syscall; use privilege lvl 3 for this to allow userspace calls */
  idt[IDT_SYSCALL] = make_idt_desc(asm_irqh_syscall, KERNEL_CS, TRAP, DPL3);

  /* Load the IDT */
  lidt(idt_desc_ptr);
}

