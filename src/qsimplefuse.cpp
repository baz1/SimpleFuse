/*
 * Copyright (c) 2015, Rémi Bazin <bazin.remi@gmail.com>
 * All rights reserved.
 * See LICENSE for licensing details.
 *
 * Source inspired by the qt-fuse example by qknight (https://github.com/qknight/qt-fuse-example)
 */

/*!
    \class QSimpleFuse

    \brief QSimpleFuse allows a simple use of FUSE.

    \warning You need to add the following lines to your .pro file:

    DEFINES += "_FILE_OFFSET_BITS=64"

    LIBS += -lfuse
*/

#include "qsimplefuse.h"
#include "simplifier.h"

#include <string.h>
#include <QCoreApplication>
#include <QStringList>

#define FUSE_USE_VERSION 26
#include <fuse.h>
#include <fuse/fuse_lowlevel.h>


#ifndef QT_NO_DEBUG
#include <stdio.h>
#endif

static struct fuse_server {
    pthread_t pid;
    struct fuse *fuse;
    struct fuse_chan *ch;
    int failed;
    char *mountpoint;
    int multithreaded;
    int foreground;
} fs;

static void *fuse_thread(void *arg)
{
    Q_UNUSED(arg);
    if (fs.multithreaded)
    {
        if (fuse_loop_mt(fs.fuse) < 0)
        {
#ifndef QT_NO_DEBUG
            perror("fuse_loop_mt");
#endif
            fs.failed = 1;
        }
    } else {
        if (fuse_loop(fs.fuse) < 0)
        {
#ifndef QT_NO_DEBUG
            perror("fuse_loop");
#endif
            fs.failed = 1;
        }
    }
    return NULL;
}

QSimpleFuse * volatile QSimpleFuse::_instance = NULL;

char *getCStrFromQStr(const QString &str)
{
    QByteArray strData = str.toLocal8Bit();
    char *result = new char[strData.length() + 1];
    memcpy(result, strData.constData(), strData.length());
    result[strData.length()] = 0;
    return result;
}

/*!
    Constructs and initialize a new FUSE filesystem.

    This filesystem is mounted at \a mountPoint.
    If \a singlethreaded is \c true, it will not be multithreaded.

    \warning Only one single instance at a time can be created / used.
*/
QSimpleFuse::QSimpleFuse(QString mountPoint, bool singlethreaded)
{
    /* Check whether or not this is a new instance */
    if (QSimpleFuse::_instance)
    {
#ifndef QT_NO_DEBUG
        fprintf(stderr, "Error: Attempted to create a second instance of QSimpleFuse.\n");
#endif
        is_ok = false;
        return;
    }
    QSimpleFuse::_instance = this;
    /* Initialize fs */
    memset(&fs, 0, sizeof(fs));
    /* Recreating custom arguments */
    argc = singlethreaded ? 3 : 2;
    argv = new char*[argc];
    argv[0] = getCStrFromQStr(QCoreApplication::arguments().first());
    argv[1] = getCStrFromQStr(mountPoint);
    if (singlethreaded)
        argv[2] = getCStrFromQStr("-s");
    /* Parsing arguments */
    struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
    int res = fuse_parse_cmdline(&args, &fs.mountpoint, &fs.multithreaded, &fs.foreground);
    if (res == -1)
    {
#ifndef QT_NO_DEBUG
        fprintf(stderr, "QSimpleFuse: Error on fuse_parse_cmdline.\n");
#endif
        is_ok = false;
        return;
    }
    /* Mounting FS */
    fs.ch = fuse_mount(fs.mountpoint, &args);
    if (!fs.ch)
    {
#ifndef QT_NO_DEBUG
        perror("fuse_mount");
#endif
        is_ok = false;
        return;
    }
    makeSimplifiedFuseOperations();
    fs.fuse = fuse_new(fs.ch, &args, getSimplifiedFuseOperations(), sizeof(fuse_operations), NULL);
    fuse_opt_free_args(&args);
    if (!fs.fuse)
    {
#ifndef QT_NO_DEBUG
        perror("fuse_new");
#endif
        goto cancelmount;
    }
    if (pthread_create(&fs.pid, NULL, fuse_thread, NULL) != 0)
    {
#ifndef QT_NO_DEBUG
        perror("pthread_create");
#endif
        goto cancelmount;
    }
    is_ok = true;
    return;
cancelmount:
    fuse_unmount(fs.mountpoint, fs.ch);
    is_ok = false;
}

/*!
    Destructs the object and unmount the filesystem.
*/
QSimpleFuse::~QSimpleFuse()
{
    if (is_ok)
    {
        /* Aborting FS */
        fuse_session_exit(fuse_get_session(fs.fuse));
        fuse_unmount(fs.mountpoint, fs.ch);
        pthread_join(fs.pid, NULL);
        fs.fuse = NULL;
        /* Allow a future new instance */
        if (_instance == this)
            _instance = NULL;
        is_ok = false;
    }
    if (argv)
    {
        for (int i = 0; i < argc; ++i)
            delete[] argv[i];
        delete[] argv;
        argv = NULL;
    }
}

/*!
    Returns \c true if the initialization and mount have been successful,
    \c false otherwise.
*/
bool QSimpleFuse::checkStatus()
{
    return is_ok && (!fs.failed);
}

int QSimpleFuse::sGetAttr(const lString &addr, sAttr &attr)
{
    // Get the file attributes of {addr} into {attr}
    // On success, return 0.
    // Else, -error_number where error_number is:
    //  EFAULT: Bad address.
    //  ENOENT: A component of pathname does not exist, or pathname is an empty string.
    //  ENOTDIR: A component of the path prefix of pathname is not a directory.
    // TODO
    return -ENOSYS;
}

int QSimpleFuse::sMkFile(const lString &addr, mode_t mst_mode)
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
    return -ENOSYS;
}

int QSimpleFuse::sRmFile(const lString &addr, bool isDir)
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
    return -ENOSYS;
}

int QSimpleFuse::sMvFile(const lString &addrFrom, const lString &addrTo)
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
    return -ENOSYS;
}

int QSimpleFuse::sLink(const lString &addrFrom, const lString &addrTo)
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
    return -ENOSYS;
}

int QSimpleFuse::sChMod(const lString &addr, mode_t mst_mode)
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
    return -ENOSYS;
}

int QSimpleFuse::sTruncate(const lString &addr, off_t newsize)
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
    return -ENOSYS;
}

int QSimpleFuse::sUTime(const lString &addr, time_t mst_atime, time_t mst_mtime)
{
    // Change the last access date to st_atime and modification date to st_mtime.
    // On success, return 0.
    // Else, -error_number where error_number is:
    //  EACCES: The parent directory does not allow write permission to the process, or one of the directories in the path prefix of pathname did not allow search permission.
    //  ENOENT: A directory component in pathname does not exist / the file does not exist.
    //  ENOTDIR: A component of the path prefix of pathname is not a directory (not in man ???).
    //  EROFS: Read-only filesystem.
    // TODO
    return -ENOSYS;
}

int QSimpleFuse::sOpen(const lString &addr, int flags, int &fd)
{
    // Open the regular file {addr} according to the {flags} parameter:
    // O_RDONLY / O_WRONLY / O_RDWR for read-only, write-only, or read/write, respectively.
    // Other flags may be: (ignore others)
    //  O_APPEND: append (what if write/seek/write?)
    //  (O_DIRECT | O_DSYNC | O_SYNC): Do not use buffers
    //  O_NOATIME: Do not update atime.
    //  O_TRUNC: Truncate file to length 0 (must have write access).
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
    return -ENOSYS;
}

int QSimpleFuse::sRead(int fd, void *buf, int count, off_t offset)
{
    // Read {count} bytes from file {fd}, offset {offset}, inside the buffer {buf}.
    // On success, return the number of bytes read ({count} if EOF not reached).
    // Else, -error_number where error_number is:
    //  EAGAIN: The file has been marked nonblocking, and the read would block.
    //  EBADF: {fd} is not a valid file descriptor or is not open for reading.
    //  IO: I/O error.
    //  EOVERFLOW: Too large offset.
    // TODO
    return -ENOSYS;
}

int QSimpleFuse::sWrite(int fd, const void *buf, int count, off_t offset)
{
    // Write {count} bytes in file {fd}, offset {offset}, from the buffer {buf}.
    // On success, return the number of bytes written ({count}).
    // Else, -error_number where error_number is:
    //  EAGAIN: The file has been marked nonblocking, and the write would block.
    //  EBADF: {fd} is not a valid file descriptor or is not open for writing.
    //  IO: I/O error.
    //  EOVERFLOW: Too large offset.
    //  EFBIG: Attempted to write past the maximum offset.
    //  ENOSPC: Not enough space left.
    // TODO
    return -ENOSYS;
}

int QSimpleFuse::sSync(int fd)
{
    // Flush any cached data for the file {fd}.
    // On success, return 0.
    // Else, -error_number where error_number is:
    //  EBADF: {fd} is not a valid file descriptor.
    //  IO: I/O error.
    // TODO
    return -ENOSYS;
}

int QSimpleFuse::sClose(int fd)
{
    // Close the file {fd}.
    // On success, return 0.
    // Else, -error_number where error_number is:
    //  EBADF: {fd} is not a valid open file descriptor.
    //  IO: I/O error.
    // TODO
    return -ENOSYS;
}

int QSimpleFuse::sOpenDir(const lString &addr, int &fd)
{
    // Open the directory {addr}.
    // fd is set to an arbitrary filehandle (positive integer) if successful.
    // On success, return 0.
    // Else, -error_number where error_number is:
    //  EACCESS: Permission denied.
    //  ENFILE: Too many files are currently open in the system.
    //  ENOENT: Directory does not exist, or {addr} is an empty string.
    //  ENOTDIR: {addr} is not a directory.
    // TODO
    return -ENOSYS;
}

int QSimpleFuse::sReadDir(int fd, char *&name)
{
    // Read the next entry in the directory stream of {fd}.
    // This puts the name of this entry in {name} (changing its pointer value) or NULL if there is no more (no error then).
    // On success, return 0.
    // Else, -error_number where error_number is:
    //  EBADF: {fd} is not a valid directory descriptor opened for reading.
    // TODO
    return -ENOSYS;
}

int QSimpleFuse::sCloseDir(int fd)
{
    // Close the directory {fd}.
    // On success, return 0.
    // Else, -error_number where error_number is:
    //  EBADF: {fd} is not a valid directory descriptor opened for reading.
    // TODO
    return -ENOSYS;
}

int QSimpleFuse::sAccess(const lString &addr, int mode)
{
    // Check access to a file / directory.
    // If mode is F_OK, check if the file exists.
    // Else, it is a mask of R_OK, W_OK and X_OK to test read, write and execute permissions.
    // On success, return 0.
    // Else, -error_number where error_number is:
    //  EACCES: The requested access would be denied to the file, or search permission is denied for one of the directories in the path prefix of pathname.
    //  ENOENT: A directory component in pathname does not exist.
    //  ENOTDIR: A component of the path prefix of pathname is not a directory.
    //  EROFS: Write permission was requested for a file on a read-only filesystem.
    //  ETXTBSY: Write access was requested to an executable which is being executed.
    // TODO
    return -ENOSYS;
}

int QSimpleFuse::sFTruncate(int fd, off_t newsize)
{
    // Change the size of the regular file {fd} to {newsize} bytes (same as sTruncate but with a file descriptor).
    // On success, return 0.
    // Else, -error_number where error_number is:
    //  EBADF: {fd} is not a valid file descriptor opened for writing.
    // TODO
    return -ENOSYS;
}

int QSimpleFuse::sFGetAttr(int fd, sAttr &attr)
{
    // Get the file attributes of {addr} into {attr}
    // On success, return 0.
    // Else, -error_number where error_number is:
    //  EBADF: {fd} is not a valid file descriptor.
    // TODO
    return -ENOSYS;
}
