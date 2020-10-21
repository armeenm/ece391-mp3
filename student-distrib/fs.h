#ifndef FS_H
#define FS_H

#include "lib.h"
#include "util.h"

enum { FS_MAX_DIR_ENTRIES = 63 };
typedef enum FileType { FT_RTC, FT_DIR, FT_REG } FileType;
typedef struct DirEntry {
  int8_t filename[32];
  FileType filetype;
  uint32_t inode;
  uint8_t reserved[24];
} DirEntry;

int32_t read_dentry_by_name(uint8_t const* fname, DirEntry* dentry);
int32_t read_dentry_by_index(uint32_t index, DirEntry* dentry);
int32_t read_data(uint32_t inode, uint32_t offset, uint8_t* buf, uint32_t length);

#endif
