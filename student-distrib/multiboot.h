/* multiboot.h - Defines used in working with Multiboot-compliant
 * bootloaders (such as GRUB)
 * vim:ts=4 noexpandtab
 */

#ifndef _MULTIBOOT_H
#define _MULTIBOOT_H

#define MULTIBOOT_HEADER_FLAGS 0x00000003
#define MULTIBOOT_HEADER_MAGIC 0x1BADB002
#define MULTIBOOT_BOOTLOADER_MAGIC 0x2BADB002

#ifndef ASM

/* Types */
#include "types.h"

/* The Multiboot header. */
typedef struct multiboot_header {
  u32 magic;
  u32 flags;
  u32 checksum;
  u32 header_addr;
  u32 load_addr;
  u32 load_end_addr;
  u32 bss_end_addr;
  u32 entry_addr;
} multiboot_header_t;

/* The section header table for ELF. */
typedef struct elf_section_header_table {
  u32 num;
  u32 size;
  u32 addr;
  u32 shndx;
} elf_section_header_table_t;

/* The Multiboot information. */
typedef struct multiboot_info {
  u32 flags;
  u32 mem_lower;
  u32 mem_upper;
  u32 boot_device;
  u32 cmdline;
  u32 mods_count;
  u32 mods_addr;
  elf_section_header_table_t elf_sec;
  u32 mmap_length;
  u32 mmap_addr;
} multiboot_info_t;

typedef struct module {
  u32 mod_start;
  u32 mod_end;
  u32 string;
  u32 reserved;
} module_t;

/* The memory map. Be careful that the offset 0 is base_addr_low
   but no size. */
typedef struct memory_map {
  u32 size;
  u32 base_addr_low;
  u32 base_addr_high;
  u32 length_low;
  u32 length_high;
  u32 type;
} memory_map_t;

#endif /* ASM */

#endif /* _MULTIBOOT_H */
