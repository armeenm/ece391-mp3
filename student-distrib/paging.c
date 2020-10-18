#include "paging.h"

/* 
   4MB to 8MB is kernel, 8 MB to 12MB is 4KB pages 12MB to 4GB is 4MB
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
    int i;
    for(i = 0; i < PAGE_TABLE_SIZE; i++)
    {
        page_tables[i] = (i * 0x1000 | 0x3);
    }
/* Set up kernel page */
    page_directory[0] = ((unsigned int)page_tables) | 0x3;
    page_directory[1] = 0x3 | 1<<7;    //
    for(i = 2; i < PAGE_DIRECTORY_SIZE; i++)
    {
        page_directory[i] = 0x6 | 1<<7;
        
    }
    
    /* Get value of cr0 */
    unsigned int cr0;
    asm volatile("movl %%cr0, %0" : "=r"(cr0));

    /* Set 31st bit to 1 to enable PG*/
    cr0 |= 0x1<<31;
    asm volatile("movl %0, %%cr0" :: "r"(cr0));

    /* Get value of cr4 */
    unsigned int cr4;
    asm volatile("movl %%cr4, %0" : "=r"(cr4));

    /* Set 4th bit to 1 to enable PSE */ 
    cr4 |= 0x1<<4;
    asm volatile("movl %0, %%cr4" :: "r"(cr4));
    
    unsigned int page_directory_address = ((unsigned int)page_directory) & 0xFFFFF000;
    asm volatile("movl %0, %%cr3" ::"r"(page_directory_address));

}
