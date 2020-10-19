#ifndef PAGING_H
#define PAGING_H

#include "lib.h"

#define PAGE_DIRECTORY_SIZE 1024
#define PAGE_TABLE_SIZE 1024
#define VIDEO_MEMORY_START 0xB8
#define PTE_SIZE 4096
// Page directory/table setup bits
#define PRESENT 1<<0 // 0x1
#define READ_WRITE 1<<1 // 0x2
#define USER_ACCESS 1<<2 // 00000100
#define FOUR_MEG_SIZE 1<<7 // Enable 4MB paging
#define FOUR_MEG_ADDRESS_ONE 1<<22 // 4194304 -- the start of kernel space 4MB section (not a ptr to a table entry)


extern uint32_t page_directory[PAGE_DIRECTORY_SIZE] __attribute__((aligned(PTE_SIZE)));
extern uint32_t page_table[PAGE_TABLE_SIZE] __attribute__((aligned(PTE_SIZE)));

/* Enable paging and setup page directory and page table */
void init_paging();

#endif
