#ifndef FS_H
#define FS_H

#include "lib.h"
#include "util.h"

enum { FS_MAX_DIR_ENTRIES = 63, FS_FNAME_LEN = 32, FS_INODE_DATA_LEN = 1023, FS_BLK_SIZE = 4096 };

typedef enum FileType { FT_RTC, FT_DIR, FT_REG } FileType;

typedef struct FsStats {
  uint32_t direntry_cnt;
  uint32_t inode_cnt;
  uint32_t datablk_cnt;
  uint8_t reserved[52];
} FsStats;

typedef struct DirEntry {
  int8_t filename[FS_FNAME_LEN];
  FileType filetype;
  uint32_t inode_idx;
  uint8_t reserved[24];
} DirEntry;

typedef struct INode {
  uint32_t size;
  uint32_t data[FS_INODE_DATA_LEN];
} INode;

typedef struct Bootblk {
  FsStats fs_stats;
  DirEntry direntries[63];
} Bootblk;

typedef struct Datablk {
  uint8_t data[FS_BLK_SIZE];
} Datablk;

int32_t open_fs(uint32_t start, uint32_t end);

int32_t file_open(void);
int32_t file_close(void);
int32_t file_read(int8_t const* fname, uint32_t* fsize, uint8_t* buf, int32_t length, uint32_t offset);
int32_t file_write(void);

int32_t dir_open(void);
int32_t dir_close(void);
int32_t dir_read(uint32_t idx, DirEntry* dentry);
int32_t dir_write(void);

int32_t read_dentry_by_name(uint8_t const* fname, DirEntry* dentry);
int32_t read_dentry_by_index(uint32_t index, DirEntry* dentry);
int32_t read_data(uint32_t inode, uint32_t offset, uint32_t* fsize, uint8_t* buf, uint32_t length);

#endif
