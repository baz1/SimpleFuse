/*
 * Copyright (c) 2015, Rémi Bazin <bazin.remi@gmail.com>
 * All rights reserved.
 * See LICENSE for licensing details.
 */

#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <stdio.h>
#include <time.h>
#include <limits.h>
#include <qglobal.h>

#include "simplifier.h"
#include "qsimplefuse.h"

lString toLString(const char *str)
{
    lString result;
    result.str_value = str;
    result.str_len = strlen(str);
    return result;
}

#ifndef QT_NO_DEBUG
#define dispLog(...) fprintf(stderr, __VA_ARGS__);
#else
#define dispLog(...) /* NOP */
#endif

int s_getattr(const char *path, struct stat *statbuf)
{
    lString lPath = toLString(path);
    if (lPath.str_len > STR_LEN_MAX)
        return -ENAMETOOLONG;
    sAttr result;
    int ret_value;
    if ((ret_value = (QSimpleFuse::_instance)->sGetAttr(lPath, result)) < 0)
        return ret_value;
    PersistentData *data = PERSDATA;
    *statbuf = data->def_stat;
    statbuf->st_mode = result.mst_mode;
    statbuf->st_nlink = result.mst_nlink;
    statbuf->st_size = (result.mst_mode & 0x4000) ? 0x1000 : result.mst_size;
    statbuf->st_blocks = (statbuf->st_size + 0x01FF) >> 9; // really useful ???
    statbuf->st_atim.tv_sec = result.mst_atime;
    statbuf->st_mtim.tv_sec = result.mst_mtime;
    statbuf->st_ctim.tv_sec = result.mst_mtime;
    return 0;
}

int s_mknod(const char *path, mode_t mode, dev_t dev)
{
    Q_UNUSED(dev);
    lString lPath = toLString(path);
    if (lPath.str_len > STR_LEN_MAX)
        return -ENAMETOOLONG;
    if ((mode & 0xFE00) != 0x8000)
    {
        dispLog("Warning: s_mknod on \"%s\" with mode 0%o\n", path, mode);
        return -EPERM;
    }
    return (QSimpleFuse::_instance)->sMkFile(lPath, mode);
}

int s_mkdir(const char *path, mode_t mode)
{
    lString lPath = toLString(path);
    if (lPath.str_len > STR_LEN_MAX)
        return -ENAMETOOLONG;
    if ((mode & 0xFE00) != 0x4000)
    {
        dispLog("Warning: s_mkdir on \"%s\" with mode 0%o\n", path, mode);
        return -EPERM;
    }
    return (QSimpleFuse::_instance)->sMkFile(lPath, mode);
}

int s_unlink(const char *path)
{
    lString lPath = toLString(path);
    if (lPath.str_len > STR_LEN_MAX)
        return -ENAMETOOLONG;
    return (QSimpleFuse::_instance)->sRmFile(lPath, false);
}

int s_rmdir(const char *path)
{
    lString lPath = toLString(path);
    if (lPath.str_len > STR_LEN_MAX)
        return -ENAMETOOLONG;
    return (QSimpleFuse::_instance)->sRmFile(lPath, true);
}

int s_rename(const char *path, const char *newpath)
{
    lString lPathFrom = toLString(path);
    if (lPathFrom.str_len > STR_LEN_MAX)
        return -ENAMETOOLONG;
    lString lPathTo = toLString(newpath);
    if (lPathTo.str_len > STR_LEN_MAX)
        return -ENAMETOOLONG;
    return (QSimpleFuse::_instance)->sMvFile(lPathFrom, lPathTo);
}

int s_link(const char *path, const char *newpath)
{
    lString lPathFrom = toLString(newpath);
    if (lPathFrom.str_len > STR_LEN_MAX)
        return -ENAMETOOLONG;
    lString lPathTo = toLString(path);
    if (lPathTo.str_len > STR_LEN_MAX)
        return -ENAMETOOLONG;
    return (QSimpleFuse::_instance)->sLink(lPathFrom, lPathTo);
}

int s_chmod(const char *path, mode_t mode)
{
    if (mode & 0xE00)
    {
        dispLog("Warning: s_chmod on \"%s\" with mode 0%o\n", path, mode);
        return -EPERM;
    }
    lString lPath = toLString(path);
    if (lPath.str_len > STR_LEN_MAX)
        return -ENAMETOOLONG;
    return (QSimpleFuse::_instance)->sChMod(lPath, mode);
}

int s_chown(const char *path, uid_t uid, gid_t gid)
{
    Q_UNUSED(path);
    Q_UNUSED(uid);
    Q_UNUSED(gid);
    dispLog("Warning: Entered s_chown with \"%s\" --> %d:%d\n", path, uid, gid);
    return -EPERM;
}

int s_truncate(const char *path, off_t newsize)
{
    if (newsize < 0)
        return -EINVAL;
    lString lPath = toLString(path);
    if (lPath.str_len > STR_LEN_MAX)
        return -ENAMETOOLONG;
    return (QSimpleFuse::_instance)->sTruncate(lPath, newsize);
}

int s_utime(const char *path, utimbuf *ubuf)
{
    lString lPath = toLString(path);
    if (lPath.str_len > STR_LEN_MAX)
        return -ENAMETOOLONG;
    time_t mst_atime, mst_mtime;
    if (ubuf)
    {
        mst_atime = ubuf->actime;
        mst_mtime = ubuf->modtime;
    } else {
        time(&mst_atime);
        mst_mtime = mst_atime;
    }
    return (QSimpleFuse::_instance)->sUTime(lPath, mst_atime, mst_mtime);
}

int s_open(const char *path, fuse_file_info *fi)
{
    if (fi->flags & (O_ASYNC | O_DIRECTORY | O_TMPFILE | O_CREAT | O_EXCL | O_TRUNC | O_PATH))
    {
        dispLog("Warning: s_open on \"%s\" with flags 0x%08x\n", path, fi->flags);
        return -EOPNOTSUPP;
    }
    // O_NONBLOCK and O_NDELAY simply ignored.
    lString lPath = toLString(path);
    if (lPath.str_len > STR_LEN_MAX)
        return -ENAMETOOLONG;
    int fhv = 0;
    int ret_value = (QSimpleFuse::_instance)->sOpen(lPath, fi->flags, fhv);
    fi->fh = (uint64_t) fhv;
    return ret_value;
}

int s_read(const char *path, char *buf, size_t size, off_t offset, fuse_file_info *fi)
{
    Q_UNUSED(path);
    if (size > INT_MAX)
        return -EINVAL;
    return (QSimpleFuse::_instance)->sRead(fi->fh, buf, (int) size, offset);
}

int s_write(const char *path, const char *buf, size_t size, off_t offset, fuse_file_info *fi)
{
    Q_UNUSED(path);
    if (size > INT_MAX)
        return -EINVAL;
    return (QSimpleFuse::_instance)->sWrite(fi->fh, buf, (int) size, offset);
}

int s_flush(const char *path, fuse_file_info *fi)
{
    Q_UNUSED(path);
    // I did not really get the difference with a normal sync, but let's do the same action.
    return (QSimpleFuse::_instance)->sSync(fi->fh);
}

int s_release(const char *path, fuse_file_info *fi)
{
    Q_UNUSED(path);
    return (QSimpleFuse::_instance)->sClose(fi->fh);
}

int s_fsync(const char *path, int datasync, fuse_file_info *fi)
{
    Q_UNUSED(path);
    // We do not care about meta data being flushed here.
    Q_UNUSED(datasync);
    return (QSimpleFuse::_instance)->sSync(fi->fh);
}

int s_opendir(const char *path, fuse_file_info *fi)
{
    lString lPath = toLString(path);
    if (lPath.str_len > STR_LEN_MAX)
        return -ENAMETOOLONG;
    int fhv = 0;
    int ret_value = (QSimpleFuse::_instance)->sOpenDir(lPath, fhv);
    fi->fh = (uint64_t) fhv;
    return ret_value;
}

int s_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset,
    fuse_file_info *fi)
{
    Q_UNUSED(path);
    Q_UNUSED(offset);
    int ret_value;
    char *name;
    while (((ret_value = (QSimpleFuse::_instance)->sReadDir(fi->fh, name)) == 0) && name)
    {
        if (filler(buf, name, NULL, 0) != 0)
        {
            dispLog("Warning: readdir filler:  buffer full\n");
            return -ENOMEM;
        }
    }
    return ret_value;
}

int s_releasedir(const char *path, fuse_file_info *fi)
{
    Q_UNUSED(path);
    return (QSimpleFuse::_instance)->sCloseDir(fi->fh);
}

int s_fsyncdir(const char *path, int datasync, fuse_file_info *fi)
{
    Q_UNUSED(path);
    Q_UNUSED(datasync);
    Q_UNUSED(fi);
    return 0; // Directories should always be directly synchronized.
}

void *s_init(fuse_conn_info *conn)
{
    Q_UNUSED(conn);
    (QSimpleFuse::_instance)->sInit();
    return PERSDATA;
}

void s_destroy(void *userdata)
{
    Q_UNUSED(userdata);
    (QSimpleFuse::_instance)->sDestroy();
}

int s_access(const char *path, int mask)
{
    lString lPath = toLString(path);
    if (lPath.str_len > STR_LEN_MAX)
        return -ENAMETOOLONG;
    return (QSimpleFuse::_instance)->sAccess(lPath, mask);
}

int s_create(const char *path, mode_t mode, fuse_file_info *fi)
{
    lString lPath = toLString(path);
    if (lPath.str_len > STR_LEN_MAX)
        return -ENAMETOOLONG;
    if ((mode & 0xFE00) != 0x8000)
    {
        dispLog("Warning: s_mknod on \"%s\" with mode 0%o\n", path, mode);
        return -EPERM;
    }
    int ret_value = (QSimpleFuse::_instance)->sMkFile(lPath, (mode & 0x1FF) | 0x8000), fhv = 0;
    if (ret_value == -EEXIST)
    {
        ret_value = (QSimpleFuse::_instance)->sOpen(lPath, O_WRONLY | O_TRUNC, fhv);
        fi->fh = (uint64_t) fhv;
        return ret_value;
    }
    if (ret_value < 0)
        return ret_value;
    ret_value = (QSimpleFuse::_instance)->sOpen(lPath, O_WRONLY, fhv);
    fi->fh = (uint64_t) fhv;
    return ret_value;
}

int s_ftruncate(const char *path, off_t offset, fuse_file_info *fi)
{
    Q_UNUSED(path);
    return (QSimpleFuse::_instance)->sFTruncate(fi->fh, offset);
}

int s_fgetattr(const char *path, struct stat *statbuf, fuse_file_info *fi)
{
    Q_UNUSED(path);
    sAttr result;
    int ret_value;
    if ((ret_value = (QSimpleFuse::_instance)->sFGetAttr(fi->fh, result)) < 0)
        return ret_value;
    PersistentData *data = PERSDATA;
    *statbuf = data->def_stat;
    statbuf->st_mode = result.mst_mode;
    statbuf->st_nlink = result.mst_nlink;
    statbuf->st_size = (result.mst_mode & 0x4000) ? 0x1000 : result.mst_size;
    statbuf->st_blocks = (statbuf->st_size + 0x01FF) >> 9; // really useful ???
    statbuf->st_atim.tv_sec = result.mst_atime;
    statbuf->st_mtim.tv_sec = result.mst_mtime;
    statbuf->st_ctim.tv_sec = result.mst_mtime;
    return 0;
}

fuse_operations s_oper;

void makeSimplifiedFuseOperations()
{
    memset(&s_oper, 0, sizeof(s_oper));
    s_oper.getattr = s_getattr;
    s_oper.mknod = s_mknod;
    s_oper.mkdir = s_mkdir;
    s_oper.unlink = s_unlink;
    s_oper.rmdir = s_rmdir;
    s_oper.rename = s_rename;
    s_oper.link = s_link;
    s_oper.chmod = s_chmod;
    s_oper.chown = s_chown;
    s_oper.truncate = s_truncate;
    s_oper.utime = s_utime;
    s_oper.open = s_open;
    s_oper.read = s_read;
    s_oper.write = s_write;
    s_oper.flush = s_flush;
    s_oper.release = s_release;
    s_oper.fsync = s_fsync;
    s_oper.opendir = s_opendir;
    s_oper.readdir = s_readdir;
    s_oper.releasedir = s_releasedir;
    s_oper.fsyncdir = s_fsyncdir;
    s_oper.init = s_init;
    s_oper.destroy = s_destroy;
    s_oper.access = s_access;
    s_oper.create = s_create;
    s_oper.ftruncate = s_ftruncate;
    s_oper.fgetattr = s_fgetattr;
}

fuse_operations *getSimplifiedFuseOperations()
{
    return &s_oper;
}
