#include "paging.h"

/* define constants for Paging aligned to 4096 */
unsigned int page_directory[PAGE_DIRECTORY_SIZE] __attribute__ ((aligned(4096))) = {0};
unsigned int page_table[PAGE_TABLE_SIZE] __attribute__ ((aligned(4096))) = {0};

/* 
   4MB to 8MB is kernel, 0MB to 4MB is 4KB pages 8MB to 4GB is 4MB
   Differentiating 4MB and 4KB is bit 7 in PDE (0 = 4KB, 1 = 4MB)
   
   4KB C-Alignment: int some_variable __attribute__((aligned(4096)));
   Video memory goes from 0xB8000 to 0xC0000.
*/

/* void init_paging();
 * Description: Initializes Paging
 * Inputs: none
 * Return Value: none
 * Function: Creates page table & page directories.
 *           Initializes cr0 and cr4 to enable paging.
*/
void init_paging() 
{
    /* Initialize page tables */
    int i;
    for (i = 0; i < PAGE_TABLE_SIZE; ++i) {
        /* Page table set to i * 4096 and 0b11 is orded to set to
         * R/W mode and present
         */
        page_table[i] = (i * PAGE_TABLE_SIZE) | 0x3;
    }

    /* Set first page_directory to page_table, set R/W, Present */
    page_directory[0] = ((unsigned int)page_table) | 0x3;
    /* Set up kernel page, address equals 1 (22nd bit), R/W Mode (0x3), 
     * 4mb page (1 << 7)
     */
    page_directory[1] = (0x1 << 22) | 0x3 | (1 << 7);

    /* Setup remaining page directories. Usermode, R/W, Not present (0x6)
     * 4mb mode (1 << 7)
     */
    for (i = 2; i < PAGE_DIRECTORY_SIZE; ++i) {
        page_directory[i] = 0x6 | (1 << 7);
    }
    
    /* Enable paging. Set cr3 to page_directory address (cr3), enable PSE to enable
     * 4kb and 4mb pages (cr4 bit 4), enable paging (cr0 bit 31)
     */
    asm volatile("mov %0, %%cr3\n\t"
                 "mov %%cr4, %%eax\n\t"
                 "or $0x10, %%eax\n\t"
                 "mov %%eax, %%cr4\n\t"
                 "mov %%cr0, %%eax\n\t"
                 "or $0x80000000, %%eax\n\t"
                 "mov %%eax, %%cr0\n\t"
                 :: "r"(page_directory) : "eax");
}
