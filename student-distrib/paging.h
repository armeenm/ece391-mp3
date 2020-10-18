#ifndef PAGING_H
#define PAGING_H

/* Define Page Directory and Page Table sizes */
#define PAGE_DIRECTORY_SIZE 1024
#define PAGE_TABLE_SIZE 1024

/* Enable paging and setup page directory and page table */
void init_paging();

#endif
