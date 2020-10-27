#include "fs.h"
#include "debug.h"
#include "paging.h"
#include "util.h"
#include "x86_desc.h"

static Bootblk* bootblk = NULL;

// How many things we've read
static uint32_t dir_read_count = 0;

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
  // Enable the filesystem 4mb page to be marked as present
  pgdir[start >> PG_4M_ADDR_OFFSET] |= PG_PRESENT;

  /* TODO: Sanity checks */
  if (bootblk->fs_stats.direntry_cnt >= FS_MAX_DIR_ENTRIES) {
    // Reset state on page location
    pgdir[start >> PG_4M_ADDR_OFFSET] &= ~(1U);
    return -1;
  }

  return 0;
}

/* file_open
 * Description: Opens file
 * Inputs: filename, the char * holding the name (UNUSED)
 * Outputs: none
 * Return Value: 0
 * Function: none currently
 */
int32_t file_open(const uint8_t* UNUSED(filename)) { return 0; }

/* file_close
 * Description: Closes file
 * Inputs: fd -- pointer to file descriptor (UNUSED)
 * Outputs: none
 * Return Value: 0
 * Function: none currently
 */
int32_t file_close(int32_t UNUSED(fd)) { return 0; }

/* file_read
 * Description: Reads file (not to sys call spec for this part of the MP no fds setup)
 * Inputs: fname -- Name of file to read (fd in fututre)
 *         buf -- User supplied buffer to place file data in
 *         length -- Length of the file
 *         offset -- forward offset (will be in FD in the future)
 * Outputs: none
 * Return Value: -1 on failure, otherwise the number of bytes written to buf
 * Function: Used for the cat test to populate a buffer with file contents
 */
int32_t file_read(int8_t const* const fname, uint8_t* const buf, uint32_t const offset) {
  DirEntry dentry;

  // We validate fname is a ptr and grab the dentry and then do a read on the cooresponding inode to
  // buf
  return (!fname)
             ? -1
             : read_dentry_by_name((uint8_t const*)fname, &dentry) // We get the size of this inode
                   ?: read_data(dentry.inode_idx, offset, buf,
                                ((INode*)&bootblk[1])[dentry.inode_idx].size);
}

/* file_write
 * Description: Writes file
 * Inputs: fd, buf, nbytes (UNUSED)
 * Outputs: none
 * Return Value: -1 (failure because FS is readonly)
 * Function: none currently
 */
int32_t file_write(int32_t UNUSED(fd), const void* UNUSED(buf), int32_t UNUSED(nbytes)) {
  return -1;
}

/* dir_open
 * Description: Opens directory (and resets the read count for this directory)
 * Inputs: filename (UNUSED)
 * Outputs: none
 * Return Value: 0
 * Function: none currently
 */
int32_t dir_open(const uint8_t* UNUSED(filename)) {
  dir_read_count = 0;
  return 0;
}

/* dir_close
 * Description: Closes directory
 * Inputs: fd (UNUSED)
 * Outputs: none
 * Return Value: 0
 * Function: none currently
 */
int32_t dir_close(uint32_t UNUSED(fd)) { return 0; }

/* dir_read
 * Description: Reads diretory
 * Inputs:
 *  fd (UNUSED)
 *  buf -- user supplied buffer for filename
 *  nbytes -- the number of bytes to copy from filename
 * Outputs: none
 * Return Value: -1, otherwise the number of bytes copied
 * Function: Read's the current file name  for this directory read
 */
int32_t dir_read(uint32_t UNUSED(fd), void* buf, int32_t nbytes) {
  DirEntry d;
  int32_t const i = read_dentry_by_index(dir_read_count++, &d);
  int32_t const bytes = MIN(MIN(nbytes, 32), (int32_t)strlen(d.filename));

  if (bytes < 0)
    return -1;

  memcpy(buf, d.filename, (uint32_t)bytes);

  return i ? 0 : bytes;
}

/* dir_write
 * Description: Writes directory (nothing currently)
 * Inputs: fd, buf, nbytes (UNUSED)
 * Outputs: none
 * Return Value: -1
 * Function: none currently
 */
int32_t dir_write(int32_t UNUSED(fd), const void* UNUSED(buf), int32_t UNUSED(nbytes)) {
  return -1;
}

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
      // iterate through each directory entry, compare the files names for a match
      if (!strncmp(fname, bootblk->direntries[i].filename, FS_FNAME_LEN)) {
        // when they match, grab the dir entry
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
 * Description: Reads data from a file into a supplied buffer using offset, length
 * Inputs: inode -- target file inode
 *         offset -- number of bytes forward from start of file
 *         buf -- destination buffer for file data
 *         length -- number of bytes to copy past offset
 * Outputs: none
 * Return Value: -1 on failure, otherwise how many bytes were read
 * Function: used in cat to grab the data from a given file
 */
int32_t read_data(uint32_t const inode, uint32_t const offset, uint8_t* const buf,
                  uint32_t const ulength) {

  INode const* const inodes = (INode*)&bootblk[1];
  Datablk const* const datablks = (Datablk const*)&inodes[bootblk->fs_stats.inode_cnt];
  int32_t const length = (int32_t)ulength;

  // We use the datablk_idx to represent the location we need to pull from the inode
  uint32_t datablk_idx = offset / FS_BLK_SIZE;
  // This is the for determining the start address in actual data block
  uint32_t datablk_offset = offset % FS_BLK_SIZE;

  if (!buf)
    return 0;

  // Ensure we're in bounds for inode
  if (inode >= bootblk->fs_stats.inode_cnt)
    return -1;

  // Make sure our offset is less than filesize
  if (offset >= inodes[inode].size)
    return -1;

  // Check that the furthest byte we want is even in the span of data
  if (offset + ulength >= bootblk->fs_stats.direntry_cnt * FS_BLK_SIZE)
    return -1;

  // Check that the first array position is actually in the datablock size range
  if (inodes[inode].data[datablk_idx] >= bootblk->fs_stats.datablk_cnt)
    return -1;

  {
    // How many bytes we've read
    int32_t reads = 0;

    while (reads < length) {
      uint32_t const datablk = inodes[inode].data[datablk_idx];
      uint8_t const* read_addr = &datablks[datablk].data[datablk_offset];

      // if we hit the total size of the file
      if ((uint32_t)reads + offset >= inodes[inode].size)
        return reads;

      // move forward in buf and grab the byte from the data block
      buf[reads++] = *read_addr;

      // this moves read address to the next byte offset and resets to 0 when at 4096
      if (++datablk_offset >= FS_BLK_SIZE) {
        // this moves the position we're grabbing the inode datablack # from and validates it
        if (inodes[inode].data[++datablk_idx] >= bootblk->fs_stats.datablk_cnt)
          return -1;

        datablk_offset = 0;
      }
    }

    return reads;
  }
}
