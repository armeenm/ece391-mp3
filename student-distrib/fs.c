#include "fs.h"
#include "debug.h"

static Bootblk* bootblk = NULL;

void fs_init(Bootblk* const bootblk_) { bootblk = bootblk_; }

int32_t read_dentry_by_name(uint8_t const* const ufname, DirEntry* const dentry) {
  int8_t const* const fname = (int8_t const*)ufname;
  uint32_t i;

  ASSERT(dentry);

  for (i = 0; i < FS_MAX_DIR_ENTRIES; ++i)
    if (!strncmp(fname, bootblk->direntries[i].filename, FS_FNAME_LEN)) {
      memcpy(dentry, &bootblk->direntries[i], sizeof(DirEntry));
      return 0;
    }

  return -1;
}

int32_t read_dentry_by_index(uint32_t const index, DirEntry* const dentry) {
  if (index >= FS_MAX_DIR_ENTRIES)
    return -1;

  ASSERT(dentry);

  memcpy(dentry, &bootblk->direntries[index], sizeof(DirEntry));

  return 0;
}

int32_t read_data(uint32_t inode, uint32_t offset, uint8_t* buf, uint32_t length) {
  INode const* const inodes = (INode*)(bootblk + FS_BLK_SIZE);
  uint32_t data_blk = offset / FS_INODE_DATA_LEN;

  if (inode >= bootblk->fs_stats.inode_cnt)
    return -1;

  if (offset >= inodes[inode].size)
    return -1;

  if (offset + length >= FS_BLK_SIZE * bootblk->fs_stats.direntry_cnt)
    return -1;

  if (inodes[inode].data[data_blk] >= bootblk->fs_stats.datablk_cnt)
    return -1;

  {
    uint32_t data_blk_offset = offset % FS_BLK_SIZE;
    uint8_t const* read_addr = (uint8_t*)((data_blk + 1) * FS_BLK_SIZE + data_blk_offset);
    uint32_t reads = 0;

    while (reads < length) {
      buf[reads++] = *(read_addr++);

      if (reads + offset >= inodes[inode].size)
        return reads;

      if (++data_blk_offset >= FS_BLK_SIZE) {
        if (inodes[inode].data[++data_blk] >= bootblk->fs_stats.datablk_cnt)
          return -1;
        else
          data_blk_offset = 0;
      }
    }
    return reads;
  }
}
