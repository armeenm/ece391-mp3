/* idt.S - common assembly code between IDT entries */
#ifndef IDT_ASM_S
#define IDT_ASM_S

#ifndef IDT_C
#define ASM 1

#define CLOBEAX_true addl $4, %esp
#define CLOBEAX_false popl %eax

/* Save/restore registers and call the C code */
#define ASM_EXC(name) I_ASM_EXC(name, false)
#define ASM_EXC_KEEPEAX(name) I_ASM_EXC(name, true)

#define I_ASM_EXC(name, clobeax)                                                                   \
  .global asm_##name;                                                                              \
  asm_##name:                                                                                      \
  pushl %eax;                                                                                      \
  pushl %ecx;                                                                                      \
  pushl %edx;                                                                                      \
  pushf;                                                                                           \
  call name;                                                                                       \
  popf;                                                                                            \
  popl %edx;                                                                                       \
  popl %ecx;                                                                                       \
  CLOBEAX_##clobeax;                                                                               \
  iret;

/* These all use the above macro when we're compiling this file standalone */
#define EXC_DFL(name, str) ASM_EXC(name)
#define EXC_DFL_ERRC EXC_DFL
#define EXC_DFL_NOCLR EXC_DFL
#define EXC_DFL_ERRC_NOCLR EXC_DFL

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
ASM_EXC_KEEPEAX(irqh_syscall)

#undef ASM
#endif
