/* idt.S - common assembly code between IDT entries */
#ifndef IDT_ASM_S
#define IDT_ASM_S

#ifndef IDT_C

#define ASM 1

#define ASM_EXC(name)                                                                              \
  .global asm_##name;                                                                              \
  asm_##name: call name; iret

#define EXC_DFL(name, str) ASM_EXC(name)
#define EXC_DFL_ERRC(name, str) ASM_EXC(name)

.align 4

#else

#include "lib.h"

#define ASM_EXC(name) void asm_##name(void);

#define EXC_DFL(name, str)                                                                         \
  ASM_EXC(name)                                                                                    \
  void name(int eip, int UNUSED(cs), int eflags) {                                                 \
    cli();                                                                                         \
    clear();                                                                                       \
    printf("EXC: " str ": eip: 0x%x, eflags: 0x%x\n", eip, eflags);                                \
    for (;;)                                                                                       \
      ;                                                                                            \
  }

#define EXC_DFL_ERRC(name, str)                                                                    \
  ASM_EXC(name)                                                                                    \
  void name(int errc, int eip, int UNUSED(cs), int eflags) {                                       \
    cli();                                                                                         \
    clear();                                                                                       \
    printf("EXC: " str ": errc: 0x%x, eip: 0x%x, eflags: 0x%x\n", errc, eip, eflags);              \
    for (;;)                                                                                       \
      ;                                                                                            \
  }

#endif

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

EXC_DFL(irqh_pit, "PIT event!")

ASM_EXC(irqh_keyboard)
ASM_EXC(irqh_rtc)
ASM_EXC(irqh_syscall)

#endif
