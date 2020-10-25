#ifndef PAGING_H
#define PAGING_H

#define PGDIR_LEN_MCR 1024
#define PGTBL_LEN_MCR 1024
#define PTE_SIZE_MCR 4096

#ifndef ASM

#include "video.h"

enum {
  PGDIR_LEN = PGDIR_LEN_MCR,
  PGTBL_LEN = PGTBL_LEN_MCR,
  PTE_SIZE = PTE_SIZE_MCR,
  PG_VIDMEM_START = VIDMEM_START >> 12,
  PG_PRESENT = 1,
  PG_RW = 1 << 1,
  PG_USPACE = 1 << 2,
  PG_SIZE = 1 << 7,
  PG_4M_START = 1 << 22
};

/* Enable paging and setup page directory and page table */
void init_paging();

#endif
#endif
