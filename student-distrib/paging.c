#include "paging.h"

/* Define constants for Paging aligned to 4096 */
uint32_t page_directory[PAGE_DIRECTORY_SIZE] __attribute__((aligned(PTE_SIZE)));
uint32_t page_table[PAGE_TABLE_SIZE] __attribute__((aligned(PTE_SIZE)));

/*
 * 4MB to 8MB is kernel, 0MB to 4MB is 4KB pages 8MB to 4GB is 4MB
 * Differentiating 4MB and 4KB is bit 7 in PDE (0 = 4KB, 1 = 4MB)
 *
 * 4KB C-Alignment: int some_variable __attribute__((aligned(4096)));
 * Video memory goes from 0xB8000 to 0xC0000.
 */

/* void init_paging();
 * Description: Initializes paging with PSE.
 * Inputs: None
 * Return Value: None
 * Function: Creates page table & page directories.
 *           Initializes CR0, CR3, and CR4 to enable paging.
 */
void init_paging() {
  /* Initialize page tables */
  uint32_t i;

  /* Page table set to i * 4096. R = 1 */
  for (i = 0; i < PAGE_TABLE_SIZE; ++i)
    page_table[i] = (i * PTE_SIZE) | 0x2;

  /* Set video memory. R = 1, P = 1 */
  page_table[VIDEO_MEMORY_START] = (VIDEO_MEMORY_START * PTE_SIZE) | 0x3;

  /* Set first page_directory to page_table. R = 1, P = 1 */
  page_directory[0] = ((uint32_t)page_table) | 0x3;

  /* Kernel page setup.
   * Address = 1
   * S       = 1 (4MiB pages)
   * R       = 1 (R/W permissions)
   * P       = 1 (Present)
   */
  page_directory[1] = 0x3 | (1 << 7) | (1 << 22);

  /* Setup remaining page directories.
   * S = 1 (4MiB pages)
   * U = 1 (Userspace permissions)
   * R = 1 (R/W permissions)
   */
  for (i = 2; i < PAGE_DIRECTORY_SIZE; ++i)
    page_directory[i] = 0x6 | (1 << 7);

  /* Enable paging.
   * CR3     = page_directory
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
               : "r"(page_directory)
               : "eax");
}
