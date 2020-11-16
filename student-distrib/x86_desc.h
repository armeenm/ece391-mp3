/* x86_desc.h - Defines for various x86 descriptors, descriptor tables,
 * and selectors
 * vim:ts=4 noexpandtab
 */

#ifndef _X86_DESC_H
#define _X86_DESC_H

#include "types.h"

/* Segment selector values */
#define KERNEL_CS 0x0010
#define KERNEL_DS 0x0018
#define USER_CS 0x0023
#define USER_DS 0x002B
#define KERNEL_TSS 0x0030
#define KERNEL_LDT 0x0038

/* Size of the task state segment (TSS) */
#define TSS_SIZE 104

/* Number of vectors in the interrupt descriptor table (IDT) */
#define NUM_VEC 256

#ifndef ASM

#include "paging.h"

/* This structure is used to load descriptor base registers
 * like the GDTR and IDTR */
typedef struct x86_desc {
  u16 padding;
  u16 size;
  u32 addr;
} x86_desc_t;

/* This is a segment descriptor.  It goes in the GDT. */
typedef struct seg_desc {
  union {
    u32 val[2];
    struct {
      u16 seg_lim_15_00;
      u16 base_15_00;
      u8 base_23_16;
      u32 type : 4;
      u32 sys : 1;
      u32 dpl : 2;
      u32 present : 1;
      u32 seg_lim_19_16 : 4;
      u32 avail : 1;
      u32 reserved : 1;
      u32 opsize : 1;
      u32 granularity : 1;
      u8 base_31_24;
    } __attribute__((packed));
  };
} seg_desc_t;

/* TSS structure */
typedef struct __attribute__((packed)) tss_t {
  u16 prev_task_link;
  u16 prev_task_link_pad;

  u32 esp0;
  u16 ss0;
  u16 ss0_pad;

  u32 esp1;
  u16 ss1;
  u16 ss1_pad;

  u32 esp2;
  u16 ss2;
  u16 ss2_pad;

  u32 cr3;

  u32 eip;
  u32 eflags;

  u32 eax;
  u32 ecx;
  u32 edx;
  u32 ebx;
  u32 esp;
  u32 ebp;
  u32 esi;
  u32 edi;

  u16 es;
  u16 es_pad;

  u16 cs;
  u16 cs_pad;

  u16 ss;
  u16 ss_pad;

  u16 ds;
  u16 ds_pad;

  u16 fs;
  u16 fs_pad;

  u16 gs;
  u16 gs_pad;

  u16 ldt_segment_selector;
  u16 ldt_pad;

  u16 debug_trap : 1;
  u16 io_pad : 15;
  u16 io_base_addr;
} tss_t;

/* Some external descriptors declared in .S files */
extern x86_desc_t gdt_desc;

extern u16 ldt_desc;
extern u32 ldt_size;
extern seg_desc_t ldt_desc_ptr;
extern seg_desc_t gdt_ptr;
extern u32 ldt;

extern u32 tss_size;
extern seg_desc_t tss_desc_ptr;
extern tss_t tss;

extern u32 pgdir[8][PGDIR_LEN];
extern u32 pgtbl[PGTBL_LEN];
extern u32 pgtbl_proc[PGTBL_LEN];

/* Sets runtime-settable parameters in the GDT entry for the LDT */
#define SET_LDT_PARAMS(str, addr, lim)                                                             \
  do {                                                                                             \
    str.base_31_24 = (u8)(((u32)(addr)&0xFF000000) >> 24);                                         \
    str.base_23_16 = (u8)(((u32)(addr)&0x00FF0000) >> 16);                                         \
    str.base_15_00 = (u16)((u32)(addr)&0x0000FFFF);                                                \
    str.seg_lim_19_16 = ((lim)&0x000F0000) >> 16;                                                  \
    str.seg_lim_15_00 = (lim)&0x0000FFFF;                                                          \
  } while (0)

#define PG_4M (PG_4M_START | PG_USPACE)
/* Sets runtime parameters for the TSS */
#define SET_TSS_PARAMS(str, addr, lim)                                                             \
  do {                                                                                             \
    str.base_31_24 = (u8)(((u32)(addr)&0xFF000000) >> 24);                                         \
    str.base_23_16 = (u8)(((u32)(addr)&0x00FF0000) >> 16);                                         \
    str.base_15_00 = (u16)((u32)(addr)&0x0000FFFF);                                                \
    str.seg_lim_19_16 = ((lim)&0x000F0000) >> 16;                                                  \
    str.seg_lim_15_00 = (lim)&0x0000FFFF;                                                          \
  } while (0)

/* An interrupt descriptor entry (goes into the IDT) */
typedef union idt_desc_t {
  u32 val[2];
  struct {
    u16 offset_15_00;
    u16 seg_selector;
    u8 reserved4;
    u32 reserved3 : 1;
    u32 reserved2 : 1;
    u32 reserved1 : 1;
    u32 size : 1;
    u32 reserved0 : 1;
    u32 dpl : 2;
    u32 present : 1;
    u16 offset_31_16;
  } __attribute__((packed));
} idt_desc_t;

/* The IDT itself (declared in x86_desc.S) */
extern idt_desc_t idt[NUM_VEC];
/* The descriptor used to load the IDTR */
extern x86_desc_t idt_desc_ptr;

/* Sets runtime parameters for an IDT entry */
#define SET_IDT_ENTRY(str, handler)                                                                \
  do {                                                                                             \
    str.offset_31_16 = (u16)(((u32)(handler)&0xFFFF0000) >> 16);                                   \
    str.offset_15_00 = (u16)((u32)(handler)&0xFFFF);                                               \
  } while (0)

/* Load task register.  This macro takes a 16-bit index into the GDT,
 * which points to the TSS entry.  x86 then reads the GDT's TSS
 * descriptor and loads the base address specified in that descriptor
 * into the task register */
#define ltr(desc)                                                                                  \
  do {                                                                                             \
    asm volatile("ltr %w0" ::"r"(desc) : "memory");                                                \
  } while (0)

/* Load the interrupt descriptor table (IDT).  This macro takes a 32-bit
 * address which points to a 6-byte structure.  The 6-byte structure
 * (defined as "struct x86_desc" above) contains a 2-byte size field
 * specifying the size of the IDT, and a 4-byte address field specifying
 * the base address of the IDT. */
#define lidt(desc)                                                                                 \
  do {                                                                                             \
    asm volatile("lidt %0" ::"g"(desc) : "memory");                                                \
  } while (0)

/* Load the local descriptor table (LDT) register.  This macro takes a
 * 16-bit index into the GDT, which points to the LDT entry.  x86 then
 * reads the GDT's LDT descriptor and loads the base address specified
 * in that descriptor into the LDT register */
#define lldt(desc)                                                                                 \
  do {                                                                                             \
    asm volatile("lldt %%ax" : : "a"(desc) : "memory");                                            \
  } while (0)

#endif /* ASM */

#endif /* _x86_DESC_H */
