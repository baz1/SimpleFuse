/*
 * Copyright (c) 2015, Rémi Bazin <bazin.remi@gmail.com>
 * All rights reserved.
 * See LICENSE for licensing details.
 */

#include <string.h>
#include <errno.h>

#define STR_LEN_MAX 255

typedef struct lString_struct
{
	char *str_value;
	size_t str_len;
} lString;

typedef struct sAttr_struct
{
	mode_t st_mode; // (0x4000 if directory, 0x8000 if file) + file permissions (9 lowest bits, owner-read is highest, others-execute is lowest)
	nlink_t st_nlink; // number of hard links
	off_t st_size; // size if file
	time_t st_atime; // Last access time
	time_t st_mtime; // Last modification time
} sAttr;

typedef struct PersistentData_struct
{
	struct stat def_stat;
	FILE *log_file;
} PersistentData;

#define PERSDATA ((PersistentData*) fuse_get_context()->private_data)

/* Convert a C string into a lString (which contains the string length) */
lString toLString(char *str);

/* Get file attributes */
bool sGetAttr(const lString &addr, sAttr &attr);

/* Create a file */
bool sMkFile(const lString &addr, mode_t st_mode);

/* Remove a file */
bool sRmFile(const lString &addr, bool isDir);

/* Rename a file */
bool sMvFile(const lString &addrFrom, const lString &addrTo);

/* Make a link */
bool sLink(const lString &addrFrom, const lString &addrTo);

/* Change file permissions */
bool sChMod(const lString &addr, mode_t st_mode);

/* Change the size of a file */
bool sTruncate(const lString &addr, off_t newsize);

/* Change last modification/access time of a file */
bool sUTime(const lString &addr, time_t st_atime, time_t st_mtime);

/* Open a file */
int sOpen(const lString &addr, int flags);

