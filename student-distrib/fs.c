#include "fs.h"
#include "debug.h"

static Bootblk* bootblk = NULL;
static uint32_t dir_reads = 0;

void open_fs(uint32_t const start, uint32_t const end) { bootblk = (Bootblk*)start; }

int32_t file_open() { return 0; }

int32_t file_close() { return 0; }

int32_t file_read(uint8_t const* const fname, uint8_t* const buf, int32_t const length,
                  uint32_t const offset) {
  DirEntry dentry;

  return (!fname || !buf) ? -1
                          : read_dentry_by_name(fname, &dentry)
                                ?: read_data(dentry.inode_idx, offset, buf, length);
}

int32_t file_write() { return -1; }

int32_t dir_open() { return 0; }

int32_t dir_close() { return 0; }

int32_t dir_read(int8_t* const buf) {
  memcpy(buf, bootblk->direntries[dir_reads].filename, FS_FNAME_LEN);

  if (++dir_reads >= bootblk->fs_stats.direntry_cnt)
    dir_reads = 0;

  return strlen(buf);
}

int32_t dir_write() { return -1; }

int32_t read_dentry_by_name(uint8_t const* const ufname, DirEntry* const dentry) {
  int8_t const* const fname = (int8_t const*)ufname;
  uint32_t i;

  if (dentry)
    for (i = 0; i < FS_MAX_DIR_ENTRIES; ++i)
      if (!strncmp(fname, bootblk->direntries[i].filename, FS_FNAME_LEN)) {
        memcpy(dentry, &bootblk->direntries[i], sizeof(DirEntry));
        return 0;
      }

  return -1;
}

int32_t read_dentry_by_index(uint32_t const index, DirEntry* const dentry) {
  if (index >= FS_MAX_DIR_ENTRIES || !dentry)
    return -1;

  memcpy(dentry, &bootblk->direntries[index], sizeof(DirEntry));

  return 0;
}

int32_t read_data(uint32_t const inode, uint32_t const offset, uint8_t* const buf,
                  uint32_t const length) {

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
