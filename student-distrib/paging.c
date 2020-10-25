#include "paging.h"
#include "x86_desc.h"

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
void init_paging() {
  /* Make sure to flush TLB in the future when this function gets reused */

  /* Initialize page tables */
  uint32_t i;

  /* Page table set to i * 4096. R = 1 */
  pgtbl[0] = PG_RW;

  for (i = 1; i < PGTBL_LEN; ++i)
    pgtbl[i] = (i * PTE_SIZE) | PG_RW | PG_PRESENT;

  /* Set video memory. R = 1, P = 1; We may want userspace access in the future */
  pgtbl[PG_VIDMEM_START] = (PG_VIDMEM_START * PTE_SIZE) | PG_RW | PG_PRESENT;

  /* Set first pgdir to pgtbl */
  pgdir[0] = (uint32_t)pgtbl | PG_RW | PG_PRESENT;

  /* Kernel page setup.
   * Address = 1
   * S       = 1 (4MiB pages)
   * R       = 1 (R/W permissions)
   * P       = 1 (Present)
   */
  pgdir[1] = PG_4M_START | PG_PRESENT | PG_RW | PG_SIZE;

  /* Set up remaining page directories. */
  for (i = 2; i < PGDIR_LEN; ++i)
    pgdir[i] = PG_RW | PG_USPACE | PG_SIZE;

  pgdir[0x20] |= PG_PRESENT;

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
               : "r"(pgdir)
               : "eax");
}
