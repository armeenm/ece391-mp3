/* idt.S - common assembly code between IDT entries */
#ifndef IDT_ASM_S
#define IDT_ASM_S

#ifndef IDT_C
#define ASM 1
#define ASM_EXC(name)         \
  .global asm_##name;         \
  asm_##name: call name; iret

.align 4
#else
#define ASM_EXC(name) \
  void asm_##name(void);
#endif

ASM_EXC(exc_de)
ASM_EXC(exc_db)
ASM_EXC(exc_nmi)
ASM_EXC(exc_bp)
ASM_EXC(exc_of)
ASM_EXC(exc_br)
ASM_EXC(exc_ud)
ASM_EXC(exc_nm)
ASM_EXC(exc_df)
ASM_EXC(exc_cso)
ASM_EXC(exc_ts)
ASM_EXC(exc_np)
ASM_EXC(exc_ss)
ASM_EXC(exc_gp)
ASM_EXC(exc_pf)
ASM_EXC(exc_af)
ASM_EXC(exc_mf)
ASM_EXC(exc_ac)
ASM_EXC(exc_mc)
ASM_EXC(exc_xf)
ASM_EXC(exc_ve)
ASM_EXC(exc_sx)
ASM_EXC(irqh_pit)
ASM_EXC(irqh_keyboard)
ASM_EXC(irqh_rtc)
ASM_EXC(irqh_syscall)

#endif
