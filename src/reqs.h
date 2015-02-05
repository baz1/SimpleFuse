/*
 * Copyright (c) 2015, Rémi Bazin <bazin.remi@gmail.com>
 * All rights reserved.
 * See LICENSE for licensing details.
 */

#ifndef __REQS_H__
#define __REQS_H__

#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

#define STR_LEN_MAX 255

struct lString
{
    const char *str_value;
    size_t str_len;
};

struct sAttr
{
    mode_t    mst_mode;  // (0x4000 if directory, 0x8000 if file) + file permissions (9 lowest bits, owner-read is highest, others-execute is lowest)
    nlink_t   mst_nlink; // number of hard links
    off_t     mst_size;  // size if file
    time_t    mst_atime; // Last access time
    time_t    mst_mtime; // Last modification time
};

struct PersistentData
{
    struct stat def_stat;
};

#define PERSDATA ((PersistentData*) fuse_get_context()->private_data) // TODO change

/* Convert a C string into a lString (which contains the string length) */
lString toLString(const char *str);

/* Get file attributes */
int sGetAttr(const lString &addr, sAttr &attr);

/* Create a file */
int sMkFile(const lString &addr, mode_t mst_mode);

/* Remove a file */
int sRmFile(const lString &addr, bool isDir);

/* Rename a file */
int sMvFile(const lString &addrFrom, const lString &addrTo);

/* Make a link */
int sLink(const lString &addrFrom, const lString &addrTo);

/* Change file permissions */
int sChMod(const lString &addr, mode_t mst_mode);

/* Change the size of a file */
int sTruncate(const lString &addr, off_t newsize);

/* Change last modification/access time of a file */
int sUTime(const lString &addr, time_t mst_atime, time_t mst_mtime);

/* Open a file */
int sOpen(const lString &addr, int flags, int &fd);

/* Read open file */
int sRead(int fd, void *buf, int count, off_t offset);

/* Write open file */
int sWrite(int fd, const void *buf, int count, off_t offset);

/* Flush cached data */
int sSync(int fd);

/* Close a file */
int sClose(int fd);

/* Open a directory */
int sOpenDir(const lString &addr, int &fd);

/* Read a directory entry */
int sReadDir(int fd, char *&name);

/* Close a directory */
int sCloseDir(int fd);

/* Check access to a file */
int sAccess(const lString &addr, int mode);

/* Change the size of an open file */
int sFTruncate(int fd, off_t newsize);

/* Get open file attributes */
int sFGetAttr(int fd, sAttr &attr);

#endif /* Not __REQS_H__ */
