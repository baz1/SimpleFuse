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
    quint16   mst_mode;  // (0x4000 if directory, 0x8000 if file) + file permissions (9 lowest bits, owner-read is highest, others-execute is lowest)
    quint32   mst_nlink; // number of hard links
    quint64   mst_size;  // size if file
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
    virtual int sGetSize(quint64 &bsize, quint64 &bfree);

    /* Get file attributes */
    virtual int sGetAttr(const lString &pathname, sAttr &attr);

    /* Create a file */
    virtual int sMkFile(const lString &pathname, quint16 mst_mode);

    /* Remove a file */
    virtual int sRmFile(const lString &pathname, bool isDir);

    /* Rename a file */
    virtual int sMvFile(const lString &pathBefore, const lString &pathAfter);

    /* Make a link */
    virtual int sLink(const lString &pathFrom, const lString &pathTo);

    /* Change file permissions */
    virtual int sChMod(const lString &pathname, quint16 mst_mode);

    /* Change the size of a file */
    virtual int sTruncate(const lString &pathname, quint64 newsize);

    /* Change last modification/access time of a file */
    virtual int sUTime(const lString &pathname, time_t mst_atime, time_t mst_mtime);

    /* Open a file */
    virtual int sOpen(const lString &pathname, int flags, quint32 &fd);

    /* Read open file */
    virtual int sRead(quint32 fd, void *buf, quint32 count, quint64 offset);

    /* Write open file */
    virtual int sWrite(quint32 fd, const void *buf, quint32 count, quint64 offset);

    /* Flush cached data */
    virtual int sSync(quint32 fd);

    /* Close a file */
    virtual int sClose(quint32 fd);

    /* Open a directory */
    virtual int sOpenDir(const lString &pathname, quint32 &fd);

    /* Read a directory entry */
    virtual int sReadDir(quint32 fd, char *&name);

    /* Close a directory */
    virtual int sCloseDir(quint32 fd);

    /* Check access to a file */
    virtual int sAccess(const lString &pathname, quint8 mode);

    /* Change the size of an open file */
    virtual int sFTruncate(quint32 fd, quint64 newsize);

    /* Get open file attributes */
    virtual int sFGetAttr(quint32 fd, sAttr &attr);
private:
    bool is_ok;
    int argc;
    char **argv;
public: /* Intended for private use only */
    static QSimpleFuse * volatile _instance;
};

#endif /* Not __QSIMPLEFUSE_H__ */
