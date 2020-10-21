#ifndef FS_H
#define FS_H

#include "lib.h"
#include "util.h"

enum { FS_MAX_DIR_ENTRIES = 63, FS_FILENAME_LENGTH = 32, FS_INODE_DATA_LENGTH = 1023 };

typedef enum FileType { FT_RTC, FT_DIR, FT_REG } FileType;

typedef struct FsStats {
  uint32_t direntry_cnt;
  uint32_t inode_cnt;
  uint32_t datablk_cnt;
  uint8_t reserved[52];
} FsStats;

typedef struct DirEntry {
  int8_t filename[FS_FILENAME_LENGTH];
  FileType filetype;
  uint32_t inode_idx;
  uint8_t reserved[24];
} DirEntry;

typedef struct INode {
  uint32_t size;
  uint32_t data[FS_INODE_DATA_LENGTH];
} INode;

int32_t read_dentry_by_name(uint8_t const* fname, DirEntry* dentry);
int32_t read_dentry_by_index(uint32_t index, DirEntry* dentry);
int32_t read_data(uint32_t inode, uint32_t offset, uint8_t* buf, uint32_t length);

#endif
