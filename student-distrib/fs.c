#include "fs.h"
#include "debug.h"
#include "paging.h"
#include "util.h"
#include "x86_desc.h"

static Bootblk* bootblk = NULL;

/* open_fs
 * Description: Opens filesystem
 * Inputs: start -- The beginning
 *         UNUSED(end) -- NOT USED
 * Outputs: none
 * Return Value: 0 on success, -1 on failure
 * Function: Opens the filesystem and sets the page directory to present
 */
int32_t open_fs(uint32_t const start, uint32_t const UNUSED(end)) {
  bootblk = (Bootblk*)start;
  pgdir[start >> PG_4M_ADDR_OFFSET] |= PG_PRESENT;

  /* TODO: Sanity checks */
  if (bootblk->fs_stats.direntry_cnt >= FS_MAX_DIR_ENTRIES) {
    pgdir[start >> PG_4M_ADDR_OFFSET] &= ~1;
    return -1;
  }

  return 0;
}

/* file_open
 * Description: Opens file
 * Inputs: none
 * Outputs: none
 * Return Value: 0
 * Function: none currently
 */
int32_t file_open() { return 0; }

/* file_close
 * Description: Closes file
 * Inputs: none
 * Outputs: none
 * Return Value: 0
 * Function: none currently
 */
int32_t file_close() { return 0; }

/* file_read
 * Description: Reads file
 * Inputs: fname -- Name of file to read
 *         buf --
 *         length -- Length of the file
 *         offset --
 * Outputs:
 * Return Value:
 * Function:
 */
int32_t file_read(int8_t const* const fname, uint32_t* const fsize, uint8_t* const buf, int32_t const length,
                  uint32_t const offset) {
  DirEntry dentry;

  return (!fname) ? -1
                  : read_dentry_by_name((uint8_t const*)fname, &dentry)
                       ?: read_data(dentry.inode_idx, offset, fsize, buf, length);
}

/* file_write
 * Description: Writes file
 * Inputs: none
 * Outputs: none
 * Return Value: -1
 * Function: none currently
 */
int32_t file_write() { return -1; }

/* dir_open
 * Description: Opens directory
 * Inputs: none
 * Outputs: none
 * Return Value: 0
 * Function: none currently
 */
int32_t dir_open() { return 0; }

/* dir_close
 * Description: Closes directory
 * Inputs: none
 * Outputs: none
 * Return Value: 0
 * Function: none currently
 */
int32_t dir_close() { return 0; }

/* dir_read
 * Description: Reads diretory
 * Inputs: buf -- Buffer to read from directory
 * Outputs: none
 * Return Value: Length of the buffer
 * Function: Reads the directory and ...            ####################
 */
int32_t dir_read(uint32_t const idx, DirEntry* const buf) { return read_dentry_by_index(idx, buf); }

/* dir_write
 * Description: Writes directory
 * Inputs: none
 * Outputs: none
 * Return Value: -1
 * Function: none currently
 */
int32_t dir_write() { return -1; }

/* read_dentry_by_name
 * Description: Reads directory entry by name
 * Inputs: ufname -- name of the entry
 *         dentry -- Directory entry struct
 * Outputs: none
 * Return Value: returns -1 if failed, 0 if succeeds
 * Function: Reads the directory entry by name and places it in the directory
 *           entry memory.
 */
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

/* read_dentry_by_index
 * Description: Reads directory entry by index
 * Inputs: index -- index of the entry
 *         dentry -- Directory entry struct
 * Outputs: none
 * Return Value: returns -1 if failed, 0 if succeeds
 * Function: Reads the directory entry by index and places it in the directory
 *           entry memory.
 */
int32_t read_dentry_by_index(uint32_t const idx, DirEntry* const dentry) {
  if (idx >= bootblk->fs_stats.direntry_cnt || !dentry)
    return -1;

  memcpy(dentry, &bootblk->direntries[idx], sizeof(DirEntry));

  return 0;
}

/* read_data
 * Description:
 * Inputs: inode --
 *         offset --
 *         buf --
 *         length --
 * Outputs: none
 * Return Value:
 * Function:
 */
int32_t read_data(uint32_t const inode, uint32_t const offset, uint32_t* const fsize, uint8_t* const buf,
                  uint32_t const length) {

  INode const* const inodes = (INode*)&bootblk[1];
  Datablk const* const datablks = (Datablk*)&inodes[bootblk->fs_stats.inode_cnt];

  uint32_t datablk_idx = offset / FS_BLK_SIZE;
  uint32_t datablk_offset = offset % FS_BLK_SIZE;

  if (inode >= bootblk->fs_stats.inode_cnt)
    return -1;

  if (offset >= inodes[inode].size)
    return -1;

  if (inodes[inode].data[datablk_idx] >= bootblk->fs_stats.datablk_cnt)
    return -1;

  if (fsize)
    *fsize = inodes[inode].size;

  if (!buf) {
    if (!fsize)
      return -1;
    else
      return 0;
  }

  {
    uint32_t reads = 0;

    while (reads < length) {
      uint32_t const datablk = inodes[inode].data[datablk_idx];
      uint8_t const* read_addr = &datablks[datablk].data[datablk_offset];

      if (reads + offset >= inodes[inode].size)
        return reads;

      buf[reads++] = *read_addr;

      if (++datablk_offset >= FS_BLK_SIZE) {
        if (inodes[inode].data[++datablk_idx] >= bootblk->fs_stats.datablk_cnt)
          return -1;

        datablk_offset = 0;
      }
    }

    return reads;
  }
}
