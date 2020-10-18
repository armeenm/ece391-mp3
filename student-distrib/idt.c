#include "idt.h"
#include "keyboard.h"
#include "lib.h"
#include "rtc.h"
#include "util.h"
#include "x86_desc.h"

/* Default exception handler */
#define EXC_DFL(name, str)                                                                         \
  void name(int eip, int UNUSED(cs), int eflags) {                                                 \
    printf("EXC: " str ": eip: 0x%x, eflags: 0x%x\n", eip, eflags);                                \
    for (;;)                                                                                       \
      ;                                                                                            \
  }

#define EXC_DFL_ERRC(name, str)                                                                    \
  void name(int errc, int eip, int UNUSED(cs), int eflags) {                                       \
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
EXC_DFL(exc_mf, "x87 Floating-Point Exception")
EXC_DFL(exc_ac, "Alignment Check")
EXC_DFL(exc_mc, "Machine Check")
EXC_DFL(exc_xf, "SIMD Floating-Point Exception")
EXC_DFL(exc_ve, "Virtualization Exception")
EXC_DFL(exc_sx, "Security Exception")

/* TODO: These need to be legitimate handlers */
EXC_DFL(pit_handler, "PIT event!")

void syscall_handler(void) {
  printf("Handling syscall...\n");
  for (;;)
    ;
}

typedef void (*IntHandler)(void);
static const IntHandler int_handlers[] = {(IntHandler)exc_de,
                                          (IntHandler)exc_db,
                                          (IntHandler)exc_nmi,
                                          (IntHandler)exc_bp,
                                          (IntHandler)exc_of,
                                          (IntHandler)exc_br,
                                          (IntHandler)exc_ud,
                                          (IntHandler)exc_nm,
                                          (IntHandler)exc_df,
                                          (IntHandler)exc_cso,
                                          (IntHandler)exc_ts,
                                          (IntHandler)exc_np,
                                          (IntHandler)exc_ss,
                                          (IntHandler)exc_gp,
                                          (IntHandler)exc_pf,
                                          [0x10] = (IntHandler)exc_mf,
                                          (IntHandler)exc_ac,
                                          (IntHandler)exc_mc,
                                          (IntHandler)exc_xf,
                                          (IntHandler)exc_ve,
                                          [0x1E] = (IntHandler)exc_sx,
                                          [PIT_IDT] = (IntHandler)pit_handler,
                                          [KEYBOARD_IDT] = irqh_keyboard,
                                          [RTC_IDT] = irqh_rtc,
                                          [SYSCALL_IDT] = syscall_handler};

/* Various types used for make_idt_desc */
typedef enum Dpl { DPL0 = 0, DPL3 = 3 } Dpl;
typedef enum GateType { TASK = 5, INT = 6, EXC = 7 } GateType;
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
                                GateType const int_type, Dpl const dpl) {

  GateTypeU const int_type_u = {.val = int_type};
  idt_desc_t ret;

  ret.seg_selector = seg_selector;
  ret.reserved4 = 0;
  ret.reserved3 = int_type_u.reserved3;
  ret.reserved2 = int_type_u.reserved2;
  ret.reserved1 = int_type_u.reserved1;
  ret.size = 1;
  ret.reserved0 = (int_type_u.val == TASK);
  ret.dpl = dpl;
  ret.present = 1;
  SET_IDT_ENTRY(ret, handler);

  return ret;
}

void init_idt(void) {
  uint16_t i;

  /* Configure first 32 IDT entries (exceptions)
   * Defined by Intel
   * Read Appendix D for more information
   */
  for (i = 0; i < EXCEPTION_CNT; ++i)
    idt[i] = make_idt_desc(int_handlers[i], KERNEL_CS, EXC, DPL0);

  idt[PIT_IDT] = make_idt_desc(int_handlers[PIT_IDT], KERNEL_CS, INT, DPL0);
  idt[KEYBOARD_IDT] = make_idt_desc(int_handlers[KEYBOARD_IDT], KERNEL_CS, INT, DPL0);
  idt[RTC_IDT] = make_idt_desc(int_handlers[RTC_IDT], KERNEL_CS, INT, DPL0);
  idt[SYSCALL_IDT] = make_idt_desc(int_handlers[SYSCALL_IDT], KERNEL_CS, INT, DPL3);

  lidt(idt_desc_ptr);
}
