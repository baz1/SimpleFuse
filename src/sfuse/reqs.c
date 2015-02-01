/*
 * Copyright (c) 2015, Rémi Bazin <bazin.remi@gmail.com>
 * All rights reserved.
 * See LICENSE for licensing details.
 */

#include "reqs.h"

lString toLString(char *str)
{
	lString result;
	result.str_value = str;
	result.str_len = strlen(str);
	return result;
}

int sGetAttr(const lString &addr, sAttr &attr)
{
	// On success, return 0.
	// Else, -error_number where error_number is:
	//  EFAULT: Bad address.
	//  ENOENT: A component of pathname does not exist, or pathname is an empty string.
	//  ENOTDIR: A component of the path prefix of pathname is not a directory.
	// TODO
	return 0;
}

int sMkFile(const lString &addr, mode_t st_mode)
{
	// Create the file {addr} with the permissions ({mode} & 0x1FF).
	// File if ((mode & 0xFE00) == 0x8000), Directory if ((mode & 0xFE00) == 0x4000), no alternative.
	// On success, return 0.
	// Else, -error_number where error_number is:
	//  EACCES: The parent directory does not allow write permission to the process, or one of the directories in the path prefix of pathname did not allow search permission.
	//  ENOSPC: Not enough space left.
	//  EEXIST: pathname already exists (as a file or directory).
	//  EMLINK: Too much directories in parent (when creating a directory).
	//  ENOENT: A directory component in pathname does not exist.
	//  ENOTDIR: A component of the path prefix of pathname is not a directory.
	//  EROFS: pathname refers to a file on a read-only filesystem.
	// TODO
	return 0;
}

int sRmFile(const lString &addr, bool isDir)
{
	// Remove the file {addr}.
	// On success, return 0.
	// Else, -error_number where error_number is:
	//  EACCES: The parent directory does not allow write permission to the process, or one of the directories in the path prefix of pathname did not allow search permission.
	//  EBUSY: The file pathname cannot be unlinked because it is being used by the system or another process.
	//  EISDIR: pathname refers to a directory (and if isDir == false).
	//  EINVAL: pathname has . as last component (and if isDir == true).
	//  ENAMETOOLONG: pathname is too long (handle in here!).
	//  ENOENT: A directory component in pathname does not exist.
	//  ENOTDIR: A component of the path prefix of pathname is not a directory.
	//  ENOTEMPTY: pathname contains entries other than . and .. ; or, pathname has .. as its final component (and if isDir == true).
	//  EROFS: pathname refers to a file on a read-only filesystem.
	// TODO
	return 0;
}

int sMvFile(const lString &addrFrom, const lString &addrTo)
{
	// Rename the file {addrFrom} to {addrTo}.
	// On success, return 0.
	// Else, -error_number where error_number is:
	//  EACCES: The parent directory does not allow write permission to the process, or one of the directories in the path prefix of pathname did not allow search permission.
	//  EBUSY: (busy)
	//  EINVAL: The new pathname contained a path prefix of the old, or, more generally, an attempt was made to make a directory a subdirectory of itself.
	//  EISDIR: newpath is an existing directory, but oldpath is not a directory.
	//  EMLINK: oldpath already has the maximum number of links to it, or it was a directory and the directory containing newpath has the maximum number of links.
	//  ENOENT: The link named by oldpath does not exist; or, a directory component in newpath does not exist; or, oldpath or newpath is an empty string.
	//  ENOSPC: The device containing the file has no room for the new directory entry.
	//  ENOTDIR: A component used as a directory in oldpath or newpath is not, in fact, a directory.  Or, oldpath is a directory, and newpath exists but is not a directory.
	//  ENOTEMPTY: newpath is a nonempty directory, that is, contains entries other than "." and "..".
	//  EROFS: Read-only filesystem.
	// TODO
	return 0;
}

int sLink(const lString &addrFrom, const lString &addrTo)
{
	// Create a (hard) link from {addrFrom} to {addrTo} (these are not directories).
	// On success, return 0.
	// Else, -error_number where error_number is:
	//  EACCES: The parent directory does not allow write permission to the process, or one of the directories in the path prefix of pathname did not allow search permission.
	//  EEXIST: {addrTo} already exists (as a file or directory).
	//  EMLINK: The file referred to by oldpath already has the maximum number of links to it.
	//  ENOSPC: Not enough space left.
	//  ENOENT: A directory component in pathname does not exist.
	//  ENOTDIR: A component of the path prefix of pathname is not a directory.
	//  EPERM: addrTo is a directory.
	//  EPERM: The filesystem containing oldpath and newpath does not support the creation of hard links.
	//  EROFS: Read-only filesystem.
	// TODO
	return 0;
}

int sChMod(const lString &addr, mode_t st_mode)
{
	// Change the permissions of the file {addr} to ({st_mode} & 0x199)
	// On success, return 0.
	// Else, -error_number where error_number is:
	//  EACCES: The parent directory does not allow write permission to the process, or one of the directories in the path prefix of pathname did not allow search permission.
	//  ENOENT: A directory component in pathname does not exist / the file does not exist.
	//  ENOTDIR: A component of the path prefix of pathname is not a directory.
	//  EROFS: Read-only filesystem.
	//  EPERM: OK for "not possible".
	// TODO
	return true;
}

int sTruncate(const lString &addr, off_t newsize)
{
	// Change the size of the regular file {addr} to {newsize} bytes.
	// On success, return 0.
	// Else, -error_number where error_number is:
	//  EACCES: The parent directory does not allow write permission to the process, or one of the directories in the path prefix of pathname did not allow search permission.
	//  EINVAL: The argument {newsize} is larger than the maximum file size.
	//  EISDIR: The named file is a directory.
	//  ENOENT: A directory component in pathname does not exist / the file does not exist.
	//  ENOSPC: Not enough space left.
	//  ENOTDIR: A component of the path prefix of pathname is not a directory.
	//  EPERM: The underlying filesystem does not support extending a file beyond its current size.
	//  EROFS: Read-only filesystem.
	// TODO
	return 0;
}

int sUTime(const lString &addr, time_t st_atime, time_t st_mtime)
{
	// Change the last access date to st_atime and modification date to st_mtime.
	// On success, return 0.
	// Else, -error_number where error_number is:
	//  EACCES: The parent directory does not allow write permission to the process, or one of the directories in the path prefix of pathname did not allow search permission.
	//  ENOENT: A directory component in pathname does not exist / the file does not exist.
	//  ENOTDIR: A component of the path prefix of pathname is not a directory (not in man ???).
	//  EROFS: Read-only filesystem.
	// TODO
	return 0;
}

int sOpen(const lString &addr, int flags, int &fd)
{
	// Open the regular file {addr} according to the {flags} parameter:
	// O_RDONLY / O_WRONLY / O_RDWR for read-only, write-only, or read/write, respectively.
	// Other flags may be: (ignore others)
	//  O_APPEND: append (what if write/seek/write?)
	//  (O_DIRECT | O_DSYNC | O_SYNC): Do not use buffers
	//  O_NOATIME: Do not update atime.
	//  (O_NONBLOCK | O_NDELAY): Nonblocking mode.
	// fd is set to an arbitrary filehandle (positive integer) if successful.
	// On success, return 0.
	// Else, -error_number where error_number is:
	//  EACCES: The parent directory does not allow write permission to the process, or one of the directories in the path prefix of pathname did not allow search permission.
	//  EINTR: Timeout (for slow FS with a failing timeout)
	//  EISDIR: {addr} refers to an existing directory
	//  ENFILE: The system limit on the total number of open files has been reached.
	//  ENOENT: The named file does not exist.  Or, a directory component in pathname does not exist.
	//  ENOSPC: Not enough space left.
	//  ENOTDIR: A component of the path prefix of pathname is not a directory.
	//  EROFS: pathname refers to a file on a read-only filesystem and write access was requested.
	// TODO
	return 0;
}

int sRead(int fd, void *buf, int count, off_t offset)
{
	// Read {count} bytes from file {fd}, offset {offset}, inside the buffer {buf}.
	// On success, return the number of bytes read ({count} if EOF not reached).
	// Else, -error_number where error_number is:
	//  EAGAIN: The file has been marked nonblocking, and the read would block.
	//  EBADF: {fd} is not a valid file descriptor or is not open for reading.
	//  IO: I/O error.
	//  EOVERFLOW: Too large offset
	// TODO
	return 0;
}

