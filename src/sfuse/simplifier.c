/*
 * Copyright (c) 2015, Rémi Bazin <bazin.remi@gmail.com>
 * All rights reserved.
 * See LICENSE for licensing details.
 */

#define FUSE_USE_VERSION 26

#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <fuse.h>
#include <stdio.h>
#include <time.h>
#include <limits.h>

#include "reqs.h"

#define dispLog(data,...) if ((data)->log_file) vfprintf((data)->log_file, __VA_ARGS__);

int s_getattr(const char *path, struct stat *statbuf)
{
	lString lPath = toLString(path);
	if (lPath.str_len > STR_LEN_MAX)
		return -ENAMETOOLONG;
	sAttr result;
	int ret_value;
	if ((ret_value = sGetAttr(lPath, result)) < 0)
		return ret_value;
	PersistentData *data = PERSDATA;
	*statbuf = data->def_stat;
	statbuf->st_mode = result.st_mode;
	statbuf->st_nlink = result.st_nlink;
	statbuf->st_size = (result.st_mode & 0x4000) ? 0x1000 : result.st_size;
	statbuf->st_blocks = (statbuf->st_size + 0x01FF) >> 9; // really useful ???
	statbuf->st_atim.tv_sec = result.st_atime;
	statbuf->st_mtim.tv_sec = result.st_mtime;
	statbuf->st_ctim.tv_sec = result.st_mtime;
	return 0;
}

int s_mknod(const char *path, mode_t mode, dev_t dev)
{
	lString lPath = toLString(path);
	if (lPath.str_len > STR_LEN_MAX)
		return -ENAMETOOLONG;
	if ((mode & 0xFE00) != 0x8000)
	{
		dispLog(PERSDATA, "Warning: s_mknod on \"%s\" with mode 0%o\n", path, mode);
		return -EPERM;
	}
	return sMkFile(lPath, mode);
}

int s_mkdir(const char *path, mode_t mode)
{
	lString lPath = toLString(path);
	if (lPath.str_len > STR_LEN_MAX)
		return -ENAMETOOLONG;
	if ((mode & 0xFE00) != 0x4000)
	{
		dispLog(PERSDATA, "Warning: s_mkdir on \"%s\" with mode 0%o\n", path, mode);
		return -EPERM;
	}
	return sMkFile(lPath, mode);
}

int s_unlink(const char *path)
{
	lString lPath = toLString(path);
	if (lPath.str_len > STR_LEN_MAX)
		return -ENAMETOOLONG;
	return sRmFile(lPath, false);
}

int s_rmdir(const char *path)
{
	lString lPath = toLString(path);
	if (lPath.str_len > STR_LEN_MAX)
		return -ENAMETOOLONG;
	return sRmFile(lPath, true);
}

int s_rename(const char *path, const char *newpath)
{
	lString lPathFrom = toLString(path);
	if (lPathFrom.str_len > STR_LEN_MAX)
		return -ENAMETOOLONG;
	lString lPathTo = toLString(newpath);
	if (lPathTo.str_len > STR_LEN_MAX)
		return -ENAMETOOLONG;
	return sMvFile(lPathFrom, lPathTo);
}

int s_link(const char *path, const char *newpath)
{
	lString lPathFrom = toLString(newpath);
	if (lPathFrom.str_len > STR_LEN_MAX)
		return -ENAMETOOLONG;
	lString lPathTo = toLString(path);
	if (lPathTo.str_len > STR_LEN_MAX)
		return -ENAMETOOLONG;
	return sLink(lPathFrom, lPathTo);
}

int s_chmod(const char *path, mode_t mode)
{
	if (mode & 0xE00)
	{
		dispLog(PERSDATA, "Warning: s_chmod on \"%s\" with mode 0%o\n", path, mode);
		return -EPERM;
	}
	lString lPath = toLString(path);
	if (lPath.str_len > STR_LEN_MAX)
		return -ENAMETOOLONG;
	return sChMod(lPath, mode);
}

int s_chown(const char *path, uid_t uid, gid_t gid)
{
	dispLog(PERSDATA, "Warning: Entered s_chown with \"%s\" --> %d:%d\n", path, uid, gid);
	return -EPERM;
}

int s_truncate(const char *path, off_t newsize)
{
	if (newsize < 0)
		return -EINVAL;
	lString lPath = toLString(path);
	if (lPath.str_len > STR_LEN_MAX)
		return -ENAMETOOLONG;
	return sTruncate(lPath, newsize);
}

int s_utime(const char *path, struct utimbuf *ubuf)
{
	lString lPath = toLString(path);
	if (lPath.str_len > STR_LEN_MAX)
		return -ENAMETOOLONG;
	time_t st_atime, st_mtime;
	if (ubuf)
	{
		st_atime = ubuf->actime;
		st_mtime = ubuf->modtime;
	} else {
		time(&st_atime);
		st_mtime = st_atime;
	}
	return sUTime(lPath, st_atime, st_mtime);
}

int s_open(const char *path, struct fuse_file_info *fi)
{
	if (fi->flags & (O_ASYNC | O_DIRECTORY | O_TMPFILE | O_CREAT | O_EXCL | O_TRUNC | O_PATH))
	{
		dispLog(PERSDATA, "Warning: s_open on \"%s\" with flags 0x%08x\n", path, fi->flags);
		return -EOPNOTSUPP;
	}
	// O_NONBLOCK and O_NDELAY simply ignored.
	lString lPath = toLString(path);
	if (lPath.str_len > STR_LEN_MAX)
		return -ENAMETOOLONG;
	return sOpen(lPath, fi->flags, fi->fh);
}

int s_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
	if (size > INT_MAX)
		return -EINVAL;
	return sRead(fi->fh, buf, (int) size, offset);
}

int s_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
	if (size > INT_MAX)
		return -EINVAL;
	return sWrite(fi->fh, buf, (int) size, offset);
}

int s_flush(const char *path, struct fuse_file_info *fi)
{
	// I did not really get the difference with a normal sync, but let's do the same action.
	return sSync(fi->fh);
}

int s_release(const char *path, struct fuse_file_info *fi)
{
	return sClose(fi->fh);
}

int s_fsync(const char *path, int datasync, struct fuse_file_info *fi)
{
	// We do not care about meta data being flushed here.
	return sSync(fi->fh);
}

int s_opendir(const char *path, struct fuse_file_info *fi)
{
	lString lPath = toLString(path);
	if (lPath.str_len > STR_LEN_MAX)
		return -ENAMETOOLONG;
	return sOpenDir(lPath, fi->fh);
}

int s_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset,
	struct fuse_file_info *fi)
{
	int ret_value;
	char *name;
	while (((ret_value = sReadDir(fi->fh, &name)) == 0) && name)
	{
		if (filler(buf, name, NULL, 0) != 0)
		{
			dispLog(PERSDATA, "Warning: readdir filler:  buffer full\n");
			return -ENOMEM;
		}
	}
	return ret_value;
}

int s_releasedir(const char *path, struct fuse_file_info *fi)
{
	return sCloseDir(fi->fh);
}

int s_fsyncdir(const char *path, int datasync, struct fuse_file_info *fi)
{
	return 0; // Directories should always be directly synchronized.
}

void *s_init(struct fuse_conn_info *conn)
{
	return PERSDATA;
}

void s_destroy(void *userdata)
{
	PersistentData *data = PERSDATA;
	if (data->log_file)
	{
		fclose(data->log_file);
		data->log_file = 0;
	}
}

int s_access(const char *path, int mask)
{
	lString lPath = toLString(path);
	if (lPath.str_len > STR_LEN_MAX)
		return -ENAMETOOLONG;
	return sAccess(lPath, mask);
}

int s_create(const char *path, mode_t mode, struct fuse_file_info *fi)
{
	lString lPath = toLString(path);
	if (lPath.str_len > STR_LEN_MAX)
		return -ENAMETOOLONG;
	if ((mode & 0xFE00) != 0x8000)
	{
		dispLog(PERSDATA, "Warning: s_mknod on \"%s\" with mode 0%o\n", path, mode);
		return -EPERM;
	}
	int ret_value = sMkFile(lPath, (mode & 0x1FF) | 0x8000);
	if (ret_value == -EEXIST)
		return sOpen(lPath, O_WRONLY | O_TRUNC, fi->fh);
	if (ret_value < 0)
		return ret_value;
	return sOpen(lPath, O_WRONLY, fi->fh);
}

int s_ftruncate(const char *path, off_t offset, struct fuse_file_info *fi)
{
	return sFTruncate(fi->fh, offset);
}

int s_fgetattr(const char *path, struct stat *statbuf, struct fuse_file_info *fi)
{
	sAttr result;
	int ret_value;
	if ((ret_value = sFGetAttr(fi->fh, result)) < 0)
		return ret_value;
	PersistentData *data = PERSDATA;
	*statbuf = data->def_stat;
	statbuf->st_mode = result.st_mode;
	statbuf->st_nlink = result.st_nlink;
	statbuf->st_size = (result.st_mode & 0x4000) ? 0x1000 : result.st_size;
	statbuf->st_blocks = (statbuf->st_size + 0x01FF) >> 9; // really useful ???
	statbuf->st_atim.tv_sec = result.st_atime;
	statbuf->st_mtim.tv_sec = result.st_mtime;
	statbuf->st_ctim.tv_sec = result.st_mtime;
	return 0;
}

struct fuse_operations s_oper = {
	.getattr = s_getattr,
	.readlink = NULL,
	.getdir = NULL,
	.mknod = s_mknod,
	.mkdir = s_mkdir,
	.unlink = s_unlink,
	.rmdir = s_rmdir,
	.symlink = NULL,
	.rename = s_rename,
	.link = s_link,
	.chmod = s_chmod,
	.chown = s_chown,
	.truncate = s_truncate,
	.utime = s_utime,
	.open = s_open,
	.read = s_read,
	.write = s_write,
	.statfs = NULL,
	.flush = s_flush,
	.release = s_release,
	.fsync = s_fsync,
	.setxattr = NULL,
	.getxattr = NULL,
	.listxattr = NULL,
	.removexattr = NULL,
	.opendir = s_opendir,
	.readdir = s_readdir,
	.releasedir = s_releasedir,
	.fsyncdir = s_fsyncdir,
	.init = s_init,
	.destroy = s_destroy,
	.access = s_access,
	.create = s_create,
	.ftruncate = s_ftruncate,
	.fgetattr = s_fgetattr,
	.lock = NULL,
	.utimens = NULL, /* Fall back to utime implementation */
	.bmap = NULL,
	.ioctl = NULL,
	.poll = NULL,
	.write_buf = NULL, /* Fall back to write implementation */
	.read_buf = NULL, /* Fall back to read implementation */
	.flock = NULL,
	.fallocate = NULL
};

int main(int argc, char *argv[])
{
	/* Check UID (security measure) */
	if ((getuid() == 0) || (geteuid() == 0))
	{
		fprintf(stderr, "Error: This program does not need the root privileges.\n");
		return 1;
	}
	/* Initialize persistent data */
	PersistentData *data = malloc(sizeof(PersistentData));
	if (!data)
	{
		perror("Error in malloc");
		return 1;
	}
	memset(&data->def_stat, 0, sizeof(struct stat));
	data->def_stat.st_uid = geteuid();
	data->def_stat.st_gid = getegid();
	/* Parse command-line arguments */
	// TODO: Correctly parse command-line arguments.
	if (strcmp(argv[argc - 1], "-v") == 0)
	{
		data->log_file = fopen("simplefuse.log", "w");
		if (!data->log_file)
		{
			perror("Error while opening log file");
			free(data);
			return 1;
		}
		setlinebuf(data->log_file);
		argv[argc - 1] = NULL;
		--argc;
	} else {
		data->log_file = NULL;
	}
	// TODO: Some stuff (?)
	fuse_stat = fuse_main(argc, argv, &s_oper, data);
	return fuse_stat;
}

