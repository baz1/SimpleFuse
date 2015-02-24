/*
 * Copyright (c) 2015, Rémi Bazin <bazin.remi@gmail.com>
 * All rights reserved.
 * See LICENSE for licensing details.
 */

#ifndef __QSIMPLEFUSE_H__
#define __QSIMPLEFUSE_H__

/* Do we want to output the FUSE debug messages in DEBUG mode? */
#define ALLOW_FULL_DEBUG 1

#include <sys/types.h>
#include <sys/stat.h>
#include <QString>

struct lString
{
    const char *str_value;
    size_t str_len;
};

#define SF_MODE_DIRECTORY       0x4000
#define SF_MODE_REGULARFILE     0x8000

struct sAttr
{
    mode_t    mst_mode;  // (0x4000 if directory, 0x8000 if file) + file permissions (9 lowest bits, owner-read is highest, others-execute is lowest)
    nlink_t   mst_nlink; // number of hard links
    off_t     mst_size;  // size if file
    time_t    mst_atime; // Last access time
    time_t    mst_mtime; // Last modification time
};

class QSimpleFuse
{
public:
    explicit QSimpleFuse(QString mountPoint, bool singlethreaded = false);
    virtual ~QSimpleFuse();
    bool checkStatus();
public:
    /* Initialize */
    virtual void sInit();

    /* Destroy */
    virtual void sDestroy();

    /* Get the filesystem size and free size */
    virtual int sGetSize(quint64 &size, quint64 &free);

    /* Get file attributes */
    virtual int sGetAttr(const lString &pathname, sAttr &attr);

    /* Create a file */
    virtual int sMkFile(const lString &pathname, mode_t mst_mode);

    /* Remove a file */
    virtual int sRmFile(const lString &pathname, bool isDir);

    /* Rename a file */
    virtual int sMvFile(const lString &pathBefore, const lString &pathAfter);

    /* Make a link */
    virtual int sLink(const lString &pathFrom, const lString &pathTo);

    /* Change file permissions */
    virtual int sChMod(const lString &pathname, mode_t mst_mode);

    /* Change the size of a file */
    virtual int sTruncate(const lString &pathname, off_t newsize);

    /* Change last modification/access time of a file */
    virtual int sUTime(const lString &pathname, time_t mst_atime, time_t mst_mtime);

    /* Open a file */
    virtual int sOpen(const lString &pathname, int flags, int &fd);

    /* Read open file */
    virtual int sRead(int fd, void *buf, int count, off_t offset);

    /* Write open file */
    virtual int sWrite(int fd, const void *buf, int count, off_t offset);

    /* Flush cached data */
    virtual int sSync(int fd);

    /* Close a file */
    virtual int sClose(int fd);

    /* Open a directory */
    virtual int sOpenDir(const lString &pathname, int &fd);

    /* Read a directory entry */
    virtual int sReadDir(int fd, char *&name);

    /* Close a directory */
    virtual int sCloseDir(int fd);

    /* Check access to a file */
    virtual int sAccess(const lString &pathname, int mode);

    /* Change the size of an open file */
    virtual int sFTruncate(int fd, off_t newsize);

    /* Get open file attributes */
    virtual int sFGetAttr(int fd, sAttr &attr);
private:
    bool is_ok;
    int argc;
    char **argv;
public: /* Intended for private use only */
    static QSimpleFuse * volatile _instance;
};

#endif /* Not __QSIMPLEFUSE_H__ */
