#ifndef PAGING_H
#define PAGING_H

#include "lib.h"

#define PAGE_DIRECTORY_SIZE 1024
#define PAGE_TABLE_SIZE 1024
#define VIDEO_MEMORY_START 0xB8
#define PTE_SIZE 4096

extern uint32_t page_directory[PAGE_DIRECTORY_SIZE] __attribute__((aligned(PTE_SIZE)));
extern uint32_t page_table[PAGE_TABLE_SIZE] __attribute__((aligned(PTE_SIZE)));

/* Enable paging and setup page directory and page table */
void init_paging();

#endif
