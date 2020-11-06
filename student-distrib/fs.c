#include "fs.h"
#include "debug.h"
#include "paging.h"
#include "x86_desc.h"
#include "syscall.h"

static Bootblk* bootblk = NULL;

// How many things we've read
static u32 dir_read_count = 0;

/* open_fs
 * Description: Opens filesystem
 * Inputs: start -- The beginning
 *         UNUSED(end) -- NOT USED
 * Outputs: none
 * Return Value: 0 on success, -1 on failure
 * Function: Opens the filesystem and sets the page directory to present
 */
i32 open_fs(u32 const start, u32 const UNUSED(end)) {
  bootblk = (Bootblk*)start;
  // Enable the filesystem 4mb page to be marked as present
  pgdir[0][start >> PG_4M_ADDR_OFFSET] |= PG_PRESENT;

  /* TODO: Sanity checks */
  if (bootblk->fs_stats.direntry_cnt >= FS_MAX_DIR_ENTRIES) {
    // Reset state on page location
    pgdir[0][start >> PG_4M_ADDR_OFFSET] &= ~(1U);
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
i32 file_open(const u8* UNUSED(filename)) { return 0; }

/* file_close
 * Description: Closes file
 * Inputs: fd -- pointer to file descriptor (UNUSED)
 * Outputs: none
 * Return Value: 0
 * Function: none currently
 */
i32 file_close(i32 UNUSED(fd)) { return 0; }

/* file_read
 * Description: Reads file (not to sys call spec for this part of the MP no fds setup)
 * Inputs: fd     -- file descriptor to read from
 *         length -- Length of the file
 *         offset -- forward offset (will be in FD in the future)
 * Outputs: none
 * Return Value: -1 on failure, otherwise the number of bytes written to buf
 * Function: Used for the cat test to populate a buffer with file contents
 */
i32 file_read(i32 fd, u8* const buf, u32 size) {
  DirEntry dentry;

  Pcb* pcb = get_current_pcb();

  if(pcb == NULL || pcb->fds == NULL
    || (pcb->fds[fd].flags & FD_IN_USE) == FD_NOT_IN_USE)
    return -1;

  if (read_dentry_by_index(pcb->fds[fd].inode, &dentry) == -1)
    return -1;

  /* TODO: Is this the right behavior. If the buf is < size it will write into random memory */
  if (!size)
    size = ((INode*)&bootblk[1])[dentry.inode_idx].size;

  i32 bytes_read = read_data(dentry.inode_idx, pcb->fds[fd].file_position, buf, size);
  if(bytes_read >= 0)
    pcb->fds[fd].file_position += bytes_read;

  return bytes_read;
}

/* file_write
 * Description: Writes file
 * Inputs: fd, buf, nbytes (UNUSED)
 * Outputs: none
 * Return Value: -1 (failure because FS is readonly)
 * Function: none currently
 */
i32 file_write(i32 UNUSED(fd), const void* UNUSED(buf), i32 UNUSED(nbytes)) { return -1; }

/* dir_open
 * Description: Opens directory (and resets the read count for this directory)
 * Inputs: filename (UNUSED)
 * Outputs: none
 * Return Value: 0
 * Function: none currently
 */
i32 dir_open(const u8* UNUSED(filename)) {
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
i32 dir_close(u32 UNUSED(fd)) { return 0; }

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
i32 dir_read(i32 UNUSED(fd), void* buf, i32 nbytes) {
  DirEntry d;
  i32 const i = read_dentry_by_index(dir_read_count++, &d);
  i32 const bytes = MIN(MIN(nbytes, 32), (i32)strlen(d.filename));

  if (bytes < 0)
    return -1;

  memcpy(buf, d.filename, (u32)bytes);

  return i ? 0 : bytes;
}

/* dir_write
 * Description: Writes directory (nothing currently)
 * Inputs: fd, buf, nbytes (UNUSED)
 * Outputs: none
 * Return Value: -1
 * Function: none currently
 */
i32 dir_write(i32 UNUSED(fd), const void* UNUSED(buf), i32 UNUSED(nbytes)) { return -1; }

/* read_dentry_by_name
 * Description: Reads directory entry by name
 * Inputs: ufname -- name of the entry
 *         dentry -- Directory entry struct
 * Outputs: none
 * Return Value: returns -1 if failed, 0 if succeeds
 * Function: Reads the directory entry by name and places it in the directory
 *           entry memory.
 */
i32 read_dentry_by_name(u8 const* const ufname, DirEntry* const dentry) {
  i8 const* const fname = (i8 const*)ufname;
  u32 i;

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
i32 read_dentry_by_index(u32 const idx, DirEntry* const dentry) {
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
i32 read_data(u32 const inode, u32 const offset, u8* const buf, u32 const ulength) {

  INode const* const inodes = (INode*)&bootblk[1];
  Datablk const* const datablks = (Datablk const*)&inodes[bootblk->fs_stats.inode_cnt];
  i32 const length = (i32)ulength;

  // We use the datablk_idx to represent the location we need to pull from the inode
  u32 datablk_idx = offset / FS_BLK_SIZE;
  // This is the for determining the start address in actual data block
  u32 datablk_offset = offset % FS_BLK_SIZE;

  if (!buf)
    return -1;

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
    i32 reads = 0;

    while (reads < length) {
      u32 const datablk = inodes[inode].data[datablk_idx];
      u8 const* read_addr = &datablks[datablk].data[datablk_offset];

      // if we hit the total size of the file
      if ((u32)reads + offset >= inodes[inode].size)
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
