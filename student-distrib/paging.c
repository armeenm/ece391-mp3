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


/* make_task_pgdir
 * Description: Sets up a page table for a process.
 * Inputs: proc -- process id to make page table for
 * Outputs: None
 * Return Value: -1 on failure, 0 on success
 * Function: Sets up a page table for a process and flushes the tlb
 */
i32 make_task_pgdir(u8 const proc) {
  u32 i;

  /* If there are more than 8 processes, fail */
  /* It says >= because 0-7 are our 8 processes */
  if (proc >= NUM_PROC)
    return -1;

  /* Initialize page table for process */
  pgtbl_proc[0] = PG_USPACE | PG_RW;

  for (i = 1; i < PGTBL_LEN; ++i)
    pgtbl_proc[i] = (i * PTE_SIZE) | PG_USPACE | PG_RW | PG_PRESENT;

  /* Initialize page directory 4KB pages */
  pgdir[proc][0] = (u32)pgtbl_proc | PG_USPACE | PG_RW | PG_PRESENT;

  /* Initialize page directory kernel */
  pgdir[proc][1] = PG_4M | PG_RW | PG_SIZE | PG_PRESENT;

  /* Initialize page directory 4MB pages */
  pgdir[proc][ELF_LOAD_PG] = ((proc + 2) * PG_4M_START) | PG_SIZE | PG_USPACE | PG_RW | PG_PRESENT;

  /* Sets up page directory for process and flushes TLB */
  asm volatile("mov %0, %%cr3;" ::"g"(pgdir[proc]));

  return 0;
}

/* remove_task_pgdir
 * Description: Removes a processes page table
 * Inputs: proc -- process id to remove page table
 * Outputs: None
 * Return Value: 0 on success
 * Function: Removes a page table for a process and flushes the tlb
 */
i32 remove_task_pgdir(u8 const proc) {
  /* Mark page as not present */
  pgdir[proc][ELF_LOAD_PG] &= ~PG_PRESENT;

  /* Sets up page directory for process and flushes TLB */
  asm volatile("mov %0, %%cr3;" ::"g"(pgdir[proc]));

  return 0;
}

/* map_vid_mem
 * Description: Maps video memory to a page table entry
 * Inputs:    proc -- The process to map the page to
 *            virtual_address -- The virtual address to map to.
 *            physical_address -- The physical address to map to
 * Outputs: None
 * Return Value: -1 on failure, 0 on success
 * Function: Remaps a virtual address into a physical address and flushes the tlb.
 */
i32 map_vid_mem(u8 const proc, u32 virtual_address, u32 physical_address)
{
  /* If process id is valid and physical address is not in kernel space */
  if(proc >= NUM_PROC)
    return -1;
  /* Map page table to page directory */
  pgdir[proc][virtual_address/PG_4M_START] = (u32)pgtbl_proc | PG_USPACE | PG_RW | PG_PRESENT;
  /* Map page table entry to page table. Sets virtual address */
  pgtbl_proc[virtual_address % PG_4M_START] = physical_address | PG_USPACE | PG_RW | PG_PRESENT;

   /* Sets up page directory for process and flushes TLB */
  asm volatile("mov %0, %%cr3;" ::"g"(pgdir[proc]));
  return 0;
}
