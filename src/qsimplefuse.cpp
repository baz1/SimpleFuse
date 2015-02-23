/*
 * Copyright (c) 2015, Rémi Bazin <bazin.remi@gmail.com>
 * All rights reserved.
 * See LICENSE for licensing details.
 *
 * Source inspired by the qt-fuse example by qknight (https://github.com/qknight/qt-fuse-example)
 */

/*!
    \class QSimpleFuse
    \inmodule SimpleFuse
    \ingroup SimpleFuse

    \brief QSimpleFuse allows a simple use of FUSE.

    To use this class, inherit it and implement some of the virtual functions.
    The return codes are documented with macros defined in the header file \c errno.h.
    You should therefore include it in order to return the correct values.

    As you will notice in this documentation, each of the functions you might implement
    correspond to one or more UNIX system calls. You will then be able to look into
    further documentation, on the Internet for instance.
    Please note however, that your implementation does not need to set the \c errno value,
    only return the opposit value of the error if any happens.

    All the functions that you do not implement will be considered "unsupported" by the filesystem.

    \warning You need to add the following lines to your .pro file:

    DEFINES += "_FILE_OFFSET_BITS=64"

    LIBS += -lfuse
*/

#include "qsimplefuse.h"
#include "simplifier.h"

#include <string.h>
#include <QCoreApplication>
#include <QStringList>
#include <errno.h>

#ifndef FUSE_USE_VERSION
#define FUSE_USE_VERSION 26
#endif

#include <fuse.h>
#include <fuse/fuse_lowlevel.h>


#ifndef QT_NO_DEBUG
#include <stdio.h>
#endif

/*!
    \class lString
    \inmodule SimpleFuse
    \ingroup SimpleFuse

    \brief C string with length information.
*/

/*!
    \variable lString::str_len

    The length of the string (ending null character excluded).
*/

/*!
    \variable lString::str_value

    The C value of the string.
*/

/*!
    \class sAttr
    \inmodule SimpleFuse
    \ingroup SimpleFuse

    \brief Structure holding the file attributes.
*/

/*!
    \variable sAttr::mst_mode

    The type of file (SF_MODE_DIRECTORY or SF_MODE_REGULARFILE) combined with the access right flags.
    The access rights are using the 9 lower bits of \c mst_mode (mask \c 0x1FF).
    Below are the meaning of each bit for the access rights.
    \table
        \header
            \li Bit (flag)
            \li Description
        \row
            \li 0x01
            \li Execute right for other people
        \row
            \li 0x02
            \li Write right for other people
        \row
            \li 0x04
            \li Read right for other people
        \row
            \li 0x08
            \li Execute right for the group
        \row
            \li 0x10
            \li Write right for the group
        \row
            \li 0x20
            \li Read right for the group
        \row
            \li 0x40
            \li Execute right for the owner
        \row
            \li 0x80
            \li Write right for the owner
        \row
            \li 0x100
            \li Read right for the owner
    \endtable
    All the other bits of this variable are to be ignored.
*/

/*!
    \variable sAttr::mst_nlink

    The number of hard links of this file.
    If you do not support hard links, this variable should be
    1 for a regular file, and (2 + number-of-subdirectories) for a directory.
*/

/*!
    \variable sAttr::mst_size

    This is the size of the file, in bytes.
    Ignore this entry for directories.
*/

/*!
    \variable sAttr::mst_atime

    Last access time (seconds elapsed since epoch, as returned by the \c time() function from time.h).
*/

/*!
    \variable sAttr::mst_mtime

    Last modification time (seconds elapsed since epoch, as returned by the \c time() function from time.h).
*/

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

/*!
    Initializes the filesystem.

    \note This default implementation does nothing.

    \sa QSimpleFuse::sDestroy()
*/
void QSimpleFuse::sInit()
{
}

/*!
    Terminates and destroys the filesystem.

    \note This default implementation does nothing.

    \sa QSimpleFuse::sInit()
*/
void QSimpleFuse::sDestroy()
{
}

/*!
    Gets the size of the filesystem (number of bytes put in \a size) and its free space (number of bytes written in \a free).

    Returns \c 0 on success, or one of these values on error:
    \table
        \header
            \li Return value
            \li Description
        \row
            \li -EIO
            \li I/O error.
    \endtable
*/
int QSimpleFuse::sGetSize(quint64 &size, quint64 &free)
{
    Q_UNUSED(size);
    Q_UNUSED(free);
    return -ENOSYS;
}

/*!
    Gets the file attributes of \a pathname and store them into \a attr (see "man 2 lstat").

    Returns \c 0 on success, or one of these values on error:
    \table
        \header
            \li Return value
            \li Description
        \row
            \li -EACCESS
            \li Search permission denied for one of the directories in the path prefix of \a pathname.
        \row
            \li -ENOENT
            \li A component of \a pathname does not exist, or \a pathname is an empty string.
        \row
            \li -ENOTDIR
            \li A component of the path prefix of \a pathname is not a directory.
        \row
            \li -EIO
            \li I/O error.
    \endtable

    \sa QSimpleFuse::sFGetAttr()
*/
int QSimpleFuse::sGetAttr(const lString &pathname, sAttr &attr)
{
    Q_UNUSED(pathname);
    Q_UNUSED(attr);
    return -ENOSYS;
}

/*!
    Creates the file \a pathname with the parameters \a mst_mode (see "man 2 mknod").
    The 9 lowest bits of \a mst_mode set the permissions and one of the flags
    SF_MODE_DIRECTORY or SF_MODE_REGULARFILE is set to indicate the file type.

    Returns \c 0 on success, or one of these values on error:
    \table
        \header
            \li Return value
            \li Description
        \row
            \li -EACCESS
            \li The parent directory does not allow write permission to the process, or one of the directories in the path prefix of \a pathname did not allow search permission.
        \row
            \li -EEXIST
            \li \a pathname already exists (as a file or directory).
        \row
            \li -EMLINK
            \li Too many directories in the parent directory (when creating a directory).
        \row
            \li -ENOENT
            \li A directory component in \a pathname does not exist.
        \row
            \li -ENOSPC
            \li Not enough space left.
        \row
            \li -ENOTDIR
            \li A component of the path prefix of \a pathname is not a directory.
        \row
            \li -EROFS
            \li This is a read-only filesystem.
        \row
            \li -EIO
            \li I/O error.
    \endtable

    \sa sAttr::mst_mode
*/
int QSimpleFuse::sMkFile(const lString &pathname, mode_t mst_mode)
{
    Q_UNUSED(pathname);
    Q_UNUSED(mst_mode);
    return -ENOSYS;
}

/*!
    Removes the file \a pathname, which is supposed to be a directory if \a isDir is \c true,
    or a regular file if \a isDir is \c false (see "man 2 unlink" and "man 2 rmdir" according to the value of \a isDir).

    Returns \c 0 on success, or one of these values on error:
    \table
        \header
            \li Return value
            \li Description
        \row
            \li -EACCESS
            \li The parent directory does not allow write permission to the process, or one of the directories in the path prefix of \a pathname did not allow search permission.
        \row
            \li -EBUSY
            \li The file \a pathname cannot be unlinked because it is being used by the system or another process.
        \row
            \li -EISDIR
            \li \a pathname refers to a directory and \a isDir is \c false.
        \row
            \li -ENOENT
            \li A directory component in \a pathname does not exist.
        \row
            \li -ENOTDIR
            \li A component of the path prefix of \a pathname is not a directory,
                or \a pathname is not a directory and \a isDir is \c true.
        \row
            \li -ENOTEMPTY
            \li \a pathname contains entries other than . and .. (and \a isDir is \c true).
        \row
            \li -EROFS
            \li This is a read-only filesystem.
        \row
            \li -EIO
            \li I/O error.
    \endtable
*/
int QSimpleFuse::sRmFile(const lString &pathname, bool isDir)
{
    Q_UNUSED(pathname);
    Q_UNUSED(isDir);
    return -ENOSYS;
}

/*!
    Renames the file \a pathBefore as \a pathAfter (see "man 2 rename").

    Returns \c 0 on success, or one of these values on error:
    \table
        \header
            \li Return value
            \li Description
        \row
            \li -EACCESS
            \li Write permission is denied for the directory containing \a pathBefore or \a pathAfter, or,
                search permission is denied for one of the directories in the path prefix of \a pathBefore or \a pathAfter,
                or \a pathBefore is a directory and does not allow write permission (needed to update the ..  entry).
        \row
            \li -EBUSY
            \li Some of the resources involved are being used by the system or another process.
        \row
            \li -EINVAL
            \li The new pathname contained a path prefix of the old, or, more generally, an attempt was made to make a directory a subdirectory of itself.
        \row
            \li -EISDIR
            \li \a pathAfter is an existing directory, but \a pathBefore is not a directory.
        \row
            \li -EMLINK
            \li \a pathBefore already has the maximum number of links to it,
                or it was a directory and the directory containing \a pathAfter has the maximum number of links.
        \row
            \li -ENOENT
            \li The link named by \a pathBefore does not exist; or, a directory component in \a pathAfter does not exist; or, \a pathBefore or \a pathAfter is an empty string.
        \row
            \li -ENOSPC
            \li Not enough space left.
        \row
            \li -ENOTDIR
            \li A component used as a directory in \a pathBefore or \a pathAfter is not, in fact, a directory.
                Or, \a pathBefore is a directory, and \a pathAfter exists but is not a directory.
        \row
            \li -ENOTEMPTY
            \li \a pathAfter is a nonempty directory, that is, contains entries other than "." and "..".
        \row
            \li -EROFS
            \li This is a read-only filesystem.
        \row
            \li -EIO
            \li I/O error.
    \endtable
*/
int QSimpleFuse::sMvFile(const lString &pathBefore, const lString &pathAfter)
{
    Q_UNUSED(pathBefore);
    Q_UNUSED(pathAfter);
    return -ENOSYS;
}

/*!
    Creates a new (hard) link from \a pathFrom to \a pathTo (these are not directories) (see "man 2 link").

    Returns \c 0 on success, or one of these values on error:
    \table
        \header
            \li Return value
            \li Description
        \row
            \li -EACCESS
            \li Write access to the directory containing \a pathFrom is denied,
                or search permission is denied for one of the directories in the path prefix of \a pathFrom or \a pathTo.
        \row
            \li -EEXIST
            \li \a pathFrom already exists (as a file or directory).
        \row
            \li -EMLINK
            \li The file referred to by \a pathTo already has the maximum number of links to it.
        \row
            \li -ENOENT
            \li A directory component in \a pathFrom or \a pathTo does not exist.
        \row
            \li -ENOSPC
            \li Not enough space left.
        \row
            \li -ENOTDIR
            \li A component used as a directory in \a pathFrom or \a pathTo is not, in fact, a directory.
        \row
            \li -EPERM
            \li \a pathTo is a directory.
        \row
            \li -EROFS
            \li This is a read-only filesystem.
        \row
            \li -EIO
            \li I/O error.
    \endtable
*/
int QSimpleFuse::sLink(const lString &pathFrom, const lString &pathTo)
{
    Q_UNUSED(pathFrom);
    Q_UNUSED(pathTo);
    return -EPERM;
}

/*!
    Changes the permissions of the file \a pathname to the ones specified in the lower 9 bits of \a mst_mode (see "man 2 chmod").

    Returns \c 0 on success, or one of these values on error:
    \table
        \header
            \li Return value
            \li Description
        \row
            \li -EACCESS
            \li Search permission denied for one of the directories in the path prefix of \a pathname.
        \row
            \li -ENOENT
            \li A directory component in \a pathname does not exist, or the file \a pathname does not exist.
        \row
            \li -ENOTDIR
            \li A component of the path prefix of \a pathname is not a directory.
        \row
            \li -EPERM
            \li Permission denied.
        \row
            \li -EROFS
            \li This is a read-only filesystem.
        \row
            \li -EIO
            \li I/O error.
    \endtable

    \sa sAttr::mst_mode
*/
int QSimpleFuse::sChMod(const lString &pathname, mode_t mst_mode)
{
    Q_UNUSED(pathname);
    Q_UNUSED(mst_mode);
    return -ENOSYS;
}

/*!
    Changes the size of the regular file \a pathname to \a newsize bytes, writing new zeros if necessary (see "man 2 truncate").

    Returns \c 0 on success, or one of these values on error:
    \table
        \header
            \li Return value
            \li Description
        \row
            \li -EACCES
            \li Search permission is denied for a component of the path prefix, or the named file is not writable by the user.
        \row
            \li -EINVAL
            \li The argument \a newsize is larger than the maximum file size.
        \row
            \li -EISDIR
            \li \a pathname is a directory.
        \row
            \li -ENOENT
            \li A directory component in \a pathname does not exist or the file does not exist.
        \row
            \li -ENOSPC
            \li Not enough space left.
        \row
            \li -ENOTDIR
            \li A component of the path prefix of pathname is not a directory.
        \row
            \li -EPERM
            \li The underlying filesystem does not support extending a file beyond its current size.
        \row
            \li -EROFS
            \li This is a read-only filesystem.
        \row
            \li -EIO
            \li I/O error.
    \endtable

    \sa QSimpleFuse::sFTruncate()
*/
int QSimpleFuse::sTruncate(const lString &pathname, off_t newsize)
{
    Q_UNUSED(pathname);
    Q_UNUSED(newsize);
    return -ENOSYS;
}

/*!
    Changes the last access date of \a pathname to \a mst_atime and modification date to \a mst_mtime (see "man 2 utime").

    Returns \c 0 on success, or one of these values on error:
    \table
        \header
            \li Return value
            \li Description
        \row
            \li -EACCES
            \li Search permission is denied for a component of the path prefix, or the named file is not writable by the user.
        \row
            \li -ENOENT
            \li A directory component in \a pathname does not exist or the file does not exist.
        \row
            \li -ENOTDIR
            \li A component of the path prefix of pathname is not a directory.
        \row
            \li -EROFS
            \li This is a read-only filesystem.
        \row
            \li -EIO
            \li I/O error.
    \endtable
*/
int QSimpleFuse::sUTime(const lString &pathname, time_t mst_atime, time_t mst_mtime)
{
    Q_UNUSED(pathname);
    Q_UNUSED(mst_atime);
    Q_UNUSED(mst_mtime);
    return -ENOSYS;
}

/*!
    Opens the regular file \a pathname according to the \a flags parameter (see "man 2 open").
    Flags that should not be ignored are reported in the following table.
    \table
        \header
            \li Flag value
            \li Description
        \row
            \li O_RDONLY
            \li Read only.
        \row
            \li O_WRONLY
            \li Write only.
        \row
            \li O_RDWR
            \li Read and write.
        \row
            \li O_APPEND
            \li Append data.
        \row
            \li O_DIRECT | O_DSYNC | O_SYNC
            \li Do not use buffered IO (simplified interpretation).
        \row
            \li O_NOATIME
            \li Do not update the last access time.
        \row
            \li O_TRUNC
            \li Truncate file to length 0 (must have write access).
    \endtable

    \a fd is set to an arbitrary filehandle (positive integer) if successful.

    Returns \c 0 on success, or one of these values on error:
    \table
        \header
            \li Return value
            \li Description
        \row
            \li -EACCES
            \li One of the directories in the path prefix of pathname did not allow search permission,
                the requested access to the file is not allowed or the file did not exist yet and write access to the parent directory is not allowed.
        \row
            \li -EISDIR
            \li \a pathname refers to an existing directory.
        \row
            \li -ENFILE
            \li The system limit on the total number of open files has been reached.
        \row
            \li -ENOENT
            \li The named file does not exist, or a directory component in \a pathname does not exist.
        \row
            \li -ENOSPC
            \li Not enough space left.
        \row
            \li -ENOTDIR
            \li A component of the path prefix of pathname is not a directory.
        \row
            \li -EBUSY
            \li The file \a pathname cannot be opened with writing access because it is being used by the system or another process.
        \row
            \li -EROFS
            \li This is a read-only filesystem and write access was requested.
        \row
            \li -EIO
            \li I/O error.
    \endtable
*/
int QSimpleFuse::sOpen(const lString &pathname, int flags, int &fd)
{
    Q_UNUSED(pathname);
    Q_UNUSED(flags);
    Q_UNUSED(fd);
    return -ENOSYS;
}

/*!
    Reads \a count bytes from file \a fd, offset \a offset, inside the buffer \a buf (see "man 2 read").

    Returns the number of bytes read on success.
    This number should be equal to \a count provided that the end of the file has not been reached.
    Else, returns one of these values:
    \table
        \header
            \li Return value
            \li Description
        \row
            \li -EBADF
            \li \a fd is not a valid file descriptor or is not open for reading.
        \row
            \li -EOVERFLOW
            \li Too large offset.
        \row
            \li -EIO
            \li I/O error.
    \endtable
*/
int QSimpleFuse::sRead(int fd, void *buf, int count, off_t offset)
{
    Q_UNUSED(fd);
    Q_UNUSED(buf);
    Q_UNUSED(count);
    Q_UNUSED(offset);
    return -ENOSYS;
}

/*!
    Writes \a count bytes in file \a fd, offset \a offset, from the buffer \a buf (see "man 2 write").

    Returns the number of bytes written on success.
    This number should be equal to \a count.
    Else, returns one of these values:
    \table
        \header
            \li Return value
            \li Description
        \row
            \li -EBADF
            \li \a fd is not a valid file descriptor or is not open for writing.
        \row
            \li -EOVERFLOW
            \li Too large offset.
        \row
            \li -EFBIG
            \li Attempted to write past the maximum (system-defined) offset.
        \row
            \li -ENOSPC
            \li Not enough space left.
        \row
            \li -EIO
            \li I/O error.
    \endtable
*/
int QSimpleFuse::sWrite(int fd, const void *buf, int count, off_t offset)
{
    Q_UNUSED(fd);
    Q_UNUSED(buf);
    Q_UNUSED(count);
    Q_UNUSED(offset);
    return -ENOSYS;
}

/*!
    Flushes any cached data for the file \a fd (see "man 2 fsync").

    Returns \c 0 on success, or one of these values on error:
    \table
        \header
            \li Return value
            \li Description
        \row
            \li -EBADF
            \li \a fd is not a valid open file descriptor.
        \row
            \li -EIO
            \li I/O error.
    \endtable

    \warning The default implementation of this function returns 0
        without doing anything.
*/
int QSimpleFuse::sSync(int fd)
{
    Q_UNUSED(fd);
    return 0;
}

/*!
    Closes the file \a fd (see "man 2 close").

    Returns \c 0 on success, or one of these values on error:
    \table
        \header
            \li Return value
            \li Description
        \row
            \li -EBADF
            \li \a fd is not a valid open file descriptor.
        \row
            \li -EIO
            \li I/O error.
    \endtable
*/
int QSimpleFuse::sClose(int fd)
{
    Q_UNUSED(fd);
    return -ENOSYS;
}

/*!
    Opens the directory \a pathname (see "man 3 opendir").

    \a fd is set to an arbitrary filehandle (positive integer) if successful.

    Returns \c 0 on success, or one of these values on error:
    \table
        \header
            \li Return value
            \li Description
        \row
            \li -EACCESS
            \li Permission denied.
        \row
            \li -ENFILE
            \li Too many files are currently open in the system.
        \row
            \li -ENOENT
            \li A directory component in \a pathname does not exist, or \a pathname is an empty string.
        \row
            \li -ENOTDIR
            \li A component of the path prefix of \a pathname or \a pathname itself is not a directory.
        \row
            \li -EIO
            \li I/O error.
    \endtable
*/
int QSimpleFuse::sOpenDir(const lString &pathname, int &fd)
{
    Q_UNUSED(pathname);
    Q_UNUSED(fd);
    return -ENOSYS;
}

/*!
    Reads the next entry in the directory stream of \a fd (see "man 3 readdir").
    This puts the name of this entry in \a name (changing its pointer value) or NULL if there is no more (no error then).

    Returns \c 0 on success, or one of these values on error:
    \table
        \header
            \li Return value
            \li Description
        \row
            \li -EBADF
            \li \a fd is not a valid directory descriptor opened for reading.
        \row
            \li -EIO
            \li I/O error.
    \endtable
*/
int QSimpleFuse::sReadDir(int fd, char *&name)
{
    Q_UNUSED(fd);
    Q_UNUSED(name);
    return -ENOSYS;
}

/*!
    Closes the directory \a fd (see "man 3 closedir").

    Returns \c 0 on success, or one of these values on error:
    \table
        \header
            \li Return value
            \li Description
        \row
            \li -EBADF
            \li \a fd is not a valid directory descriptor opened for reading.
        \row
            \li -EIO
            \li I/O error.
    \endtable
*/
int QSimpleFuse::sCloseDir(int fd)
{
    Q_UNUSED(fd);
    return -ENOSYS;
}

/*!
    Checks access to a file / directory (see "man 2 access").

    If \a mode is F_OK, check if the file exists.
    Else, it is a mask of R_OK, W_OK and X_OK to test read, write and execute permissions respectively.

    Returns \c 0 on success (access granted), or one of these values on error:
    \table
        \header
            \li Return value
            \li Description
        \row
            \li -EACCES
            \li The requested access would be denied to the file, or search permission is denied for one of the directories in the path prefix of pathname.
        \row
            \li -ENOENT
            \li A directory component in \a pathname does not exist.
        \row
            \li -ENOTDIR
            \li A component of the path prefix of \a pathname is not a directory.
        \row
            \li -EROFS
            \li Write permission was requested for a file, and this is a read-only filesystem.
        \row
            \li -ETXTBSY
            \li Write access was requested to an executable which is being used by the system or another process.
        \row
            \li -EIO
            \li I/O error.
    \endtable

    \sa QSimpleFuse::sOpen()
*/
int QSimpleFuse::sAccess(const lString &pathname, int mode)
{
    Q_UNUSED(pathname);
    Q_UNUSED(mode);
    return -ENOSYS;
}

/*!
    Changes the size of the regular file \a fd to \a newsize bytes (same as sTruncate but with a file descriptor) (see "man 2 ftruncate").

    Returns \c 0 on success, or one of these values on error:
    \table
        \header
            \li Return value
            \li Description
        \row
            \li -EBADF
            \li \a fd is not a valid file descriptor opened for writing.
        \row
            \li -EINVAL
            \li The argument \a newsize is larger than the maximum file size.
        \row
            \li -ENOSPC
            \li Not enough space left.
        \row
            \li -EPERM
            \li The underlying filesystem does not support extending a file beyond its current size.
        \row
            \li -EIO
            \li I/O error.
    \endtable

    \sa QSimpleFuse::sTruncate()
*/
int QSimpleFuse::sFTruncate(int fd, off_t newsize)
{
    Q_UNUSED(fd);
    Q_UNUSED(newsize);
    return -ENOSYS;
}

/*!
    Gets the file attributes of \a fd and store them into \a attr (same as sGetAttr but with a file descriptor) (see "man 2 fstat").

    Returns \c 0 on success, or one of these values on error:
    \table
        \header
            \li Return value
            \li Description
        \row
            \li -EBADF
            \li \a fd is not a valid file descriptor.
        \row
            \li -EIO
            \li I/O error.
    \endtable

    \sa QSimpleFuse::sGetAttr()
*/
int QSimpleFuse::sFGetAttr(int fd, sAttr &attr)
{
    Q_UNUSED(fd);
    Q_UNUSED(attr);
    return -ENOSYS;
}
