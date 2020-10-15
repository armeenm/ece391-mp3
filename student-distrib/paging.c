#include "paging.h"

/* 
   4MB to 8MB is kernel, 8 MB to 12MB is 4KB pages 12MB to 4GB is 4MB
   Differentiating 4MB and 4KB is bit 7 in PDE (0 = 4KB, 1 = 4MB)
   
   4KB C-Alignment: int some_variable __attribute__((aligned(4096)));
   Video memory goes from 0xB8000 to 0xC0000.


*/

void init_paging(void) 
{
    int i;
    for(i = 0; i < PAGE_DIRECTORY_SIZE; i++)
    {
        page_directory[i] = 0;
    }




    /* Get value of cr0 */
    unsigned int cr0;
    asm volatile("mov %%cr0, %0" : "=b"(cr0));

    /* Set 31st bit to 1 to enable PG*/
    cr0 |= 0x1<<31;
    asm volatile("mov %0, %%cr0", "b"(cr0));

    /* Get value of cr4 */
    unsigned int cr4;
    asm volatile("mov %%cr4, %0" : "=b"(cr4));

    /* Set 4th bit to 1 to enable PSE */ 
    cr4 |= 0x1<<4;
    asm volatile("mov %0, %%cr4", "b"(cr4));
    
}