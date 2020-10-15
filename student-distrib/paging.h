#ifndef PAGING_H
#define PAGING_H

/* Define memory addresses and constants */

#define KERNEL_MEMORY_START 0x400000
#define VIDEO_MEMORY_START  0x0B8000
#define VIDEO_MEMORY_END    0x0C0000
#define PAGE_DIRECTORY_SIZE 1024
#define PAGE_TABLE_SIZE 1024


void init_paging(void);

static unsigned int page_directory[PAGE_DIRECTORY_SIZE] __attribute__ ((aligned(4096)));
static unsigned int page_tables[PAGE_TABLE_SIZE] __attribute__ ((aligned(4096)));

#endif