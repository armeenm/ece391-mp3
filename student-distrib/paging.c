#include "paging.h"
#include "x86_desc.h"

enum { ELF_LOAD_PG = 0x20 };

/*
 * 4MB to 8MB is kernel, 0MB to 4MB is 4KB pages 8MB to 4GB is 4MB
 * Differentiating 4MB and 4KB is bit 7 in PDE (0 = 4KB, 1 = 4MB)
 *
 * 4KB C-Alignment: int some_variable __attribute__((aligned(4096)));
 * Video memory goes from 0xB8000 to 0xC0000.
 */

/* init_paging
 * Description: Initializes paging with PSE.
 * Inputs: None
 * Outputs: None
 * Return Value: None
 * Function: Creates page table & page directories.
 *           Initializes CR0, CR3, and CR4 to enable paging.
 */
void init_paging(void) {
  /* Initialize page tables */
  u32 i;

  /* Page table set to i * 4096. R = 1 */
  pgtbl[0] = PG_RW;

  for (i = 1; i < PGTBL_LEN; ++i)
    pgtbl[i] = (i * PTE_SIZE) | PG_RW | PG_PRESENT;

  /* Set video memory. R = 1, P = 1; We may want userspace access in the future */
  /* pgtbl[PG_VIDMEM_START] |= PG_USPACE; */

  /* Set first pgdir entry to pgtbl */
  pgdir[0][0] = (u32)pgtbl | PG_RW | PG_PRESENT;

  /* Kernel page setup */
  pgdir[0][1] = PG_4M_START | PG_RW | PG_SIZE | PG_PRESENT;

  /* Set up remaining page directories. */
  for (i = 2; i < PGDIR_LEN; ++i)
    pgdir[0][i] = (i * PG_4M_START) | PG_RW | PG_USPACE | PG_SIZE;

  /* Enable paging.
   * CR3     = pgdir
   * CR4.PSE = 1 (Enable 4MiB pages)
   * CR0.PG  = 1 (Enable paging)
   */
  asm volatile("mov %0, %%cr3;"

               "mov %%cr4, %%eax;"
               "or $0x10, %%eax;"
               "mov %%eax, %%cr4;"

               "mov %%cr0, %%eax;"
               "or $0x80000000, %%eax;"
               "mov %%eax, %%cr0;"
               :
               : "g"(pgdir[0])
               : "eax");
}

i32 make_task_pgdir(u8 const proc) {
  u32 i;

  if (proc >= 8)
    return -1;

  pgtbl_proc[0] = PG_USPACE | PG_RW;

  for (i = 1; i < PGTBL_LEN; ++i)
    pgtbl_proc[i] = (i * PTE_SIZE) | PG_USPACE | PG_RW | PG_PRESENT;

  pgdir[proc][0] = (u32)pgtbl_proc | PG_USPACE | PG_RW | PG_PRESENT;

  pgdir[proc][1] = PG_4M_START | PG_RW | PG_SIZE | PG_PRESENT;

  pgdir[proc][ELF_LOAD_PG] = ((proc + 2) * PG_4M_START) | PG_SIZE | PG_USPACE | PG_RW | PG_PRESENT;

  asm volatile("mov %0, %%cr3;" ::"g"(pgdir[proc]));

  return 0;
}

i32 remove_task_pgdir(u8 const proc) {
  pgdir[proc][ELF_LOAD_PG] = ((proc + 2) * PG_4M_START) | PG_SIZE | PG_USPACE | PG_RW;
  asm volatile("mov %0, %%cr3;" ::"g"(pgdir[proc]));
  return 0;
}
