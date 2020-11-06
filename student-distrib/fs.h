#ifndef FS_H
#define FS_H

#include "lib.h"
#include "util.h"

enum { FS_MAX_DIR_ENTRIES = 63, FS_FNAME_LEN = 32, FS_INODE_DATA_LEN = 1023, FS_BLK_SIZE = 4096 };

typedef enum FileType { FT_RTC, FT_DIR, FT_REG } FileType;

typedef struct FsStats {
  u32 direntry_cnt;
  u32 inode_cnt;
  u32 datablk_cnt;
  u8 reserved[52];
} FsStats;

typedef struct DirEntry {
  i8 filename[FS_FNAME_LEN];
  FileType filetype;
  u32 inode_idx;
  u8 reserved[24];
} DirEntry;

typedef struct INode {
  u32 size;
  u32 data[FS_INODE_DATA_LEN];
} INode;

typedef struct Bootblk {
  FsStats fs_stats;
  DirEntry direntries[63];
} Bootblk;

typedef struct Datablk {
  u8 data[FS_BLK_SIZE];
} Datablk;

i32 open_fs(u32 start, u32 end);

i32 file_open(const u8* filename);
i32 file_close(i32 fd);
i32 file_read(i8 const* fname, u8* buf, u32 offset, u32 size);
i32 file_write(i32 fd, const void* buf, i32 nbytes);

i32 dir_open(const u8* filename);
i32 dir_close(u32 fd);
i32 dir_read(i32 fd, void* buf, i32 nbytes);
i32 dir_write(i32 fd, const void* buf, i32 nbytes);

i32 read_dentry_by_name(u8 const* fname, DirEntry* dentry);
i32 read_dentry_by_index(u32 index, DirEntry* dentry);
i32 read_data(u32 inode, u32 offset, u8* buf, u32 length);

#endif
