/* idt.S - common assembly code between IDT entries */
#ifndef IDT_ASM_S
#define IDT_ASM_S

/* Macros to use when included in C code */
#ifdef IDT_C

#include "lib.h"

#define ASM_EXC(name) void asm_##name(void);

/* Some macro magic to choose whether we clear or not */
#define CLR_true clear();
#define CLR_false

#define I_EXC_DFL(name, str, clear)                                                                \
  ASM_EXC(name)                                                                                    \
  void name(void) {                                                                                \
    cli();                                                                                         \
    CLR_##clear                                                                                    \
    printf("EXC: " str ": eip: 0x%x, eflags: 0x%x\n", 0, 0);                                       \
    HLTLOOP;                                                                                       \
  }

#define I_EXC_DFL_ERRC(name, str, clear)                                                           \
  ASM_EXC(name)                                                                                    \
  void name(void) {                                                                                \
    cli();                                                                                         \
    CLR_##clear                                                                                    \
    printf("EXC: " str ": errc: 0x%x, eip: 0x%x, eflags: 0x%x\n", 0, 0, 0);                        \
    HLTLOOP;                                                                                       \
  }

/* Separate versions for whether we should clear the screen or not */
#define EXC_DFL(name, str) I_EXC_DFL(name, str, true)
#define EXC_DFL_NOCLR(name, str) I_EXC_DFL(name, str, false)

/* Certain versions for accepting an error code at the top of the stack -- mainly for GPF */
#define EXC_DFL_ERRC(name, str) I_EXC_DFL_ERRC(name, str, true)
#define EXC_DFL_ERRC_NOCLR(name, str) I_EXC_DFL_ERRC(name, str, false)

/* Macros to use when compiled standalone */
#else

#define ASM 1

/* Save/restore registers and call the C code */
#define ASM_EXC(name)                                                                              \
  .global asm_##name;                                                                              \
  asm_##name:                                                                                      \
  pushl %eax;                                                                                      \
  pushl %ecx;                                                                                      \
  pushl %edx;                                                                                      \
  call name;                                                                                       \
  popl %edx;                                                                                       \
  popl %ecx;                                                                                       \
  popl %eax;                                                                                       \
  iret;

/* These all use the above macro when we're compiling this file standalone */
#define EXC_DFL(name, str) ASM_EXC(name)
#define EXC_DFL_ERRC EXC_DFL
#define EXC_DFL_NOCLR EXC_DFL
#define EXC_DFL_ERRC_NOCLR EXC_DFL

.align 4

#endif

/* Define processor exceptions */
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
EXC_DFL_NOCLR(exc_af, "(Debug) Assertion Failure")
EXC_DFL(exc_mf, "x87 Floating-Point Exception")
EXC_DFL(exc_ac, "Alignment Check")
EXC_DFL(exc_mc, "Machine Check")
EXC_DFL(exc_xf, "SIMD Floating-Point Exception")
EXC_DFL(exc_ve, "Virtualization Exception")
EXC_DFL(exc_sx, "Security Exception")

/* PIT - unused */
EXC_DFL(irqh_pit, "PIT event!")

/* Define normal interrupts */
ASM_EXC(irqh_keyboard)
ASM_EXC(irqh_rtc)
ASM_EXC(irqh_syscall)

#endif
