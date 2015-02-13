#include "myfs.h"

#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>

#include <QByteArray>

#define READONLY_FS 0

#define DIR_BLOCK_SIZE 0x400
#define REG_BLOCK_SIZE 0x1000

#define MAX_OPEN_FILES 1000

#if DIR_BLOCK_SIZE < REG_BLOCK_SIZE
#define MIN_BLOCK_SIZE DIR_BLOCK_SIZE
#else
#define MIN_BLOCK_SIZE REG_BLOCK_SIZE
#endif

#define SEEK_ERROR ((off_t) (-1))

static char str_buffer[0x100];

/* We will make it single-threaded to avoid any further concurrency issues */
MyFS::MyFS(QString mountPoint, QString filename) : QSimpleFuse(mountPoint, true), filename(convStr(filename)), fd(-1)
{
}

MyFS::~MyFS()
{
    delete[] filename;
}

void MyFS::createNewFilesystem(QString filename)
{
    char *cFilename = convStr(filename);
    int fd = creat(cFilename, 0666);
    delete[] cFilename;
    if (fd < 0)
    {
        perror("creat");
        return;
    }
    if (ftruncate(fd, 0x100000) < 0) /* 1MB as a starting size */
    {
        perror("ftruncate");
        close(fd);
        return;
    }
    quint32 addr;
    quint16 mshort;
    addr = htonl(8);
    if (write(fd, &addr, 4) != 4) goto abort;
    addr = htonl(DIR_BLOCK_SIZE + 8);
    if (write(fd, &addr, 4) != 4) goto abort;
    addr = htonl(DIR_BLOCK_SIZE);
    if (write(fd, &addr, 4) != 4) goto abort;
    addr = 0;
    if (write(fd, &addr, 4) != 4) goto abort;
    addr = htonl(time(0));
    if (write(fd, &addr, 4) != 4) goto abort;
    mshort = htons(2);
    if (write(fd, &mshort, 2) != 2) goto abort;
    mshort = htons(SF_MODE_DIRECTORY | 0777);
    if (write(fd, &mshort, 2) != 2) goto abort;
    addr = htonl(8);
    if (write(fd, &addr, 4) != 4) goto abort;
    str_buffer[0] = 1;
    if (write(fd, str_buffer, 1) != 1) goto abort;
    str_buffer[1] = '.';
    if (write(fd, str_buffer + 1, 1) != 1) goto abort;
    /* /../ is the same as / (root directory) (overwritten by fuse, but let's do it the right way) */
    if (write(fd, &addr, 4) != 4) goto abort;
    str_buffer[0] = 2;
    if (write(fd, str_buffer, 1) != 1) goto abort;
    str_buffer[0] = '.';
    if (write(fd, str_buffer, 2) != 2) goto abort;
    addr = 0;
    if (write(fd, &addr, 4) != 4) goto abort;
    if (lseek(fd, DIR_BLOCK_SIZE + 8, SEEK_SET) != DIR_BLOCK_SIZE + 8) goto abort;
    addr = htonl(0x100000 - DIR_BLOCK_SIZE - 8);
    if (write(fd, &addr, 4) != 4) goto abort;
    addr = 0;
    if (write(fd, &addr, 4) != 4) goto abort;
    close(fd);
    return;
abort:
    perror("read");
    close(fd);
}

void MyFS::sInit()
{
    fd = open(filename, O_RDWR);
    if (fd < 0)
    {
        perror("open");
        return;
    }
    if (read(fd, &root_address, 4) != 4)
        goto read_error;
    root_address = ntohl(root_address);
    if (read(fd, &first_blank, 4) != 4)
        goto read_error;
    first_blank = ntohl(first_blank);
    return;
read_error:
    perror("read");
    close(fd);
    fd = -1;
}

void MyFS::sDestroy()
{
    if (fd >= 0)
    {
        close(fd);
        fd = -1;
    }
}

int MyFS::sGetAttr(const lString &pathname, sAttr &attr)
{
    if (fd < 0) return -EIO;
    quint32 addr;
    lString shallowCopy = pathname;
    int ret_value = getAddress(shallowCopy, addr);
    if (ret_value != 0)
        return ret_value;
    addr += 8;
    if (lseek(fd, addr, SEEK_SET) != addr)
        return -EIO;
    if (read(fd, &addr, 4) != 4)
        return -EIO;
    attr.mst_atime = (time_t) ntohl(addr);
    attr.mst_mtime = attr.mst_atime;
    quint16 mshort;
    if (read(fd, &mshort, 2) != 2)
        return -EIO;
    attr.mst_nlink = (nlink_t) ntohs(mshort);
    if (read(fd, &mshort, 2) != 2)
        return -EIO;
    attr.mst_mode = (mode_t) ntohs(mshort);
    if (attr.mst_mode & SF_MODE_REGULARFILE)
    {
        if (read(fd, &addr, 4) != 4)
            return -EIO;
        attr.mst_size = (off_t) ntohl(addr);
    }
    return 0;
}

int MyFS::sMkFile(const lString &pathname, mode_t mst_mode)
{
#if READONLY_FS
    Q_UNUSED(pathname);
    Q_UNUSED(mst_mode);
    return -EROFS;
#else
    /* Get the last part of the path */
    lString shallowCopy = pathname;
    if (shallowCopy.str_len == 0)
        return -ENOENT;
    while ((shallowCopy.str_len > 0) && (shallowCopy.str_value[shallowCopy.str_len - 1] == '/'))
        --shallowCopy.str_len;
    if (shallowCopy.str_len == 0)
        return -EEXIST;
    int len = shallowCopy.str_len;
    while (shallowCopy.str_value[shallowCopy.str_len - 1] != '/')
        --shallowCopy.str_len;
    len -= shallowCopy.str_len;
    int start = shallowCopy.str_len;
    if (len > 0xFF)
        return -ENAMETOOLONG;
    /* Get the address of the parent directory */
    quint32 dirAddr;
    int ret_value = getAddress(shallowCopy, dirAddr);
    if (ret_value != 0)
        return ret_value;
    /* Check whether or not this is indeed a directory */
    if (lseek(fd, dirAddr, SEEK_SET) != dirAddr)
        return -EIO;
    quint32 block_size;
    if (read(fd, &block_size, 4) != 4)
        return -EIO;
    block_size = ntohl(block_size);
    quint32 next_block;
    if (read(fd, &next_block, 4) != 4)
        return -EIO;
    if (read(fd, str_buffer, 4) != 4)
        return -EIO;
    quint16 mshort;
    if (read(fd, &mshort, 2) != 2)
        return -EIO;
    quint16 nlink = ntohs(mshort);
    if (read(fd, &mshort, 2) != 2)
        return -EIO;
    mshort = ntohs(mshort);
    if (mshort & SF_MODE_REGULARFILE)
        return -ENOTDIR;
    /* Check the permissions */
    if (!(mshort & S_IWUSR))
        return -EACCES;
    /* Create the file block */
    quint32 file;
    ret_value = getBlock(mst_mode & SF_MODE_DIRECTORY ? DIR_BLOCK_SIZE : REG_BLOCK_SIZE, file);
    if (ret_value != 0)
        return ret_value;
    quint32 createDate = htonl(time(0)), addr;
    if (write(fd, &createDate, 4) != 4)
        return -EIO;
    mshort = htons(2);
    if (write(fd, &mshort, 2) != 2)
        return -EIO;
    mshort = htons(mst_mode);
    if (write(fd, &mshort, 2) != 2)
        return -EIO;
    if (mst_mode & SF_MODE_DIRECTORY)
    {
        addr = htonl(file);
        if (write(fd, &addr, 4) != 4)
            return -EIO;
        str_buffer[0] = 1;
        if (write(fd, str_buffer, 1) != 1)
            return -EIO;
        str_buffer[1] = '.';
        if (write(fd, str_buffer + 1, 1) != 1)
            return -EIO;
        addr = htonl(dirAddr);
        if (write(fd, &addr, 4) != 4)
            return -EIO;
        str_buffer[0] = 2;
        if (write(fd, str_buffer, 1) != 1)
            return -EIO;
        str_buffer[0] = '.';
        if (write(fd, str_buffer, 2) != 2)
            return -EIO;
        addr = 0;
        if (write(fd, &addr, 4) != 4)
            return -EIO;
    } else {
        quint32 fsize = 0;
        if (write(fd, &fsize, 4) != 4)
            return -EIO;
    }
    /* Modify the last modification time */
    addr = dirAddr + 8;
    if (lseek(fd, addr, SEEK_SET) != addr)
        return -EIO;
    if (write(fd, &createDate, 4) != 4)
        return -EIO;
    /* Add new entry */
    quint32 currentPart = dirAddr;
    if (mst_mode & SF_MODE_DIRECTORY)
    {
        addr = dirAddr + 12;
        if (lseek(fd, addr, SEEK_SET) != addr)
            return -EIO;
        if (nlink == 0xFFFF)
            return -EMLINK;
        nlink = htons(nlink + 1);
        if (write(fd, &nlink, 2) != 2)
            return -EIO;
        if (read(fd, &mshort, 2) != 2)
            return -EIO;
    } else {
        addr = dirAddr + 16;
        if (lseek(fd, addr, SEEK_SET) != addr)
            return -EIO;
    }
    while (true)
    {
        if (read(fd, &addr, 4) != 4)
            return -EIO;
        if (addr == 0)
        {
            off_t currentPos = lseek(fd, 0, SEEK_CUR);
            if (currentPos == SEEK_ERROR)
                return -EIO;
            quint32 used = (quint32) currentPos;
            used -= currentPart;
            if (block_size - used >= (quint32) len + 5)
            {
                /* Add entry to the existing list */
                currentPos -= 4;
                if (lseek(fd, currentPos, SEEK_SET) != currentPos)
                    return -EIO;
                file = htonl(file);
                if (write(fd, &file, 4) != 4)
                    return -EIO;
                unsigned char sLen = (unsigned char) len;
                if (write(fd, &sLen, 1) != 1)
                    return -EIO;
                if (write(fd, pathname.str_value + start, len) != len)
                    return -EIO;
                if (write(fd, &addr, 4) != 4)
                    return -EIO;
            } else if (next_block != 0)
            {
                /* Go to the next part */
                next_block = ntohl(next_block);
                if (lseek(fd, next_block, SEEK_SET) != next_block)
                    return -EIO;
                currentPart = next_block;
                if (read(fd, &block_size, 4) != 4)
                    return -EIO;
                block_size = ntohl(block_size);
                if (read(fd, &next_block, 4) != 4)
                    return -EIO;
                continue;
            } else {
                /* Create a new part to add the entry */
                ret_value = getBlock(DIR_BLOCK_SIZE, next_block);
                if (ret_value != 0)
                {
                    freeBlock(file);
                    return ret_value;
                }
                file = htonl(file);
                if (write(fd, &file, 4) != 4)
                    return -EIO;
                unsigned char sLen = (unsigned char) len;
                if (write(fd, &sLen, 1) != 1)
                    return -EIO;
                if (write(fd, pathname.str_value + start, len) != len)
                    return -EIO;
                if (write(fd, &addr, 4) != 4)
                    return -EIO;
                currentPart += 4;
                if (lseek(fd, currentPart, SEEK_SET) != currentPart)
                    return -EIO;
                next_block = htonl(next_block);
                if (write(fd, &next_block, 4) != 4)
                    return -EIO;
            }
            return 0;
        } else {
            unsigned char sLen;
            if (read(fd, &sLen, 1) != 1)
                return -EIO;
            if (read(fd, str_buffer, sLen) != sLen)
                return -EIO;
            if ((sLen == len) && (memcmp(str_buffer, pathname.str_value + start, len) == 0))
            {
                freeBlock(file);
                return -EEXIST;
            }
        }
    }
#endif /* READONLY_FS */
}

int MyFS::sRmFile(const lString &pathname, bool isDir)
{
#if READONLY_FS
    Q_UNUSED(pathname);
    Q_UNUSED(isDir);
    return -EROFS;
#else
    /* Get the last part of the path */
    lString shallowCopy = pathname;
    if (shallowCopy.str_len == 0)
        return -ENOENT;
    while ((shallowCopy.str_len > 0) && (shallowCopy.str_value[shallowCopy.str_len - 1] == '/'))
        --shallowCopy.str_len;
    if (shallowCopy.str_len == 0)
        return -EEXIST;
    int len = shallowCopy.str_len;
    while (shallowCopy.str_value[shallowCopy.str_len - 1] != '/')
        --shallowCopy.str_len;
    len -= shallowCopy.str_len;
    int start = shallowCopy.str_len;
    /* Get the address of the parent directory */
    quint32 dirAddr, beforeAddr, currentAddr;
    int ret_value = getAddress(shallowCopy, dirAddr);
    if (ret_value != 0)
        return ret_value;
    /* Check whether or not this is indeed a directory */
    currentAddr = dirAddr + 4;
    if (lseek(fd, currentAddr, SEEK_SET) != currentAddr)
        return -EIO;
    quint32 next_block;
    if (read(fd, &next_block, 4) != 4)
        return -EIO;
    if (read(fd, str_buffer, 4) != 4)
        return -EIO;
    quint16 mshort;
    if (read(fd, &mshort, 2) != 2)
        return -EIO;
    quint16 nlink = ntohs(mshort);
    if (read(fd, &mshort, 2) != 2)
        return -EIO;
    mshort = ntohs(mshort);
    if (mshort & SF_MODE_REGULARFILE)
        return -ENOTDIR;
    /* Check the permissions */
    if (!(mshort & S_IWUSR))
        return -EACCES;
    /* Look for the pathname entry */
    while (true)
    {
        quint32 addr;
        if (read(fd, &addr, 4) != 4)
            return -EIO;
        if (!addr)
        {
            if (!next_block)
                return -ENOENT;
            beforeAddr = currentAddr;
            currentAddr = ntohl(next_block) + 4;
            if (lseek(fd, currentAddr, SEEK_SET) != currentAddr)
                return -EIO;
            if (read(fd, &next_block, 4) != 4)
                return -EIO;
            continue;
        }
        unsigned char nameLen;
        if (read(fd, &nameLen, 1) != 1)
            return -EIO;
        if (read(fd, &str_buffer, nameLen) != nameLen)
            return -EIO;
        if ((nameLen == len) && (memcmp(pathname.str_value + start, str_buffer, len) == 0))
        {
            /* Found it. */
            addr = ntohl(addr);
            off_t nextEntry = lseek(fd, 0, SEEK_CUR);
            if (nextEntry == SEEK_ERROR)
                return -EIO;
            /* Check if isDir has the right value. */
            if (lseek(fd, addr + 14, SEEK_SET) != addr + 14)
                return -EIO;
            if (read(fd, &mshort, 2) != 2)
                return -EIO;
            mshort = ntohs(mshort);
            if (isDir ^ ((bool) (mshort & SF_MODE_DIRECTORY)))
                return isDir ? -ENOTDIR : -EISDIR;
            /* If this is a directory, check if it is empty */
            if (isDir)
            {
                int myfd;
                char *entryName;
                ret_value = sOpenDir(pathname, myfd);
                if (ret_value != 0)
                    return ret_value;
                while (true)
                {
                    ret_value = sReadDir(myfd, entryName);
                    if (ret_value != 0)
                        return ret_value;
                    if (entryName == NULL)
                        break;
                    if (strcmp(entryName, ".") == 0)
                        continue;
                    if (strcmp(entryName, "..") == 0)
                        continue;
                    return -ENOTEMPTY;
                }
                sCloseDir(myfd);
            }
            /* Remove addr (or just decrease the link counter) */
            if (!isDir)
            {
                if (lseek(fd, addr + 12, SEEK_SET) == SEEK_ERROR)
                    return -EIO;
                if (read(fd, &mshort, 2) != 2)
                    return -EIO;
                mshort = ntohs(mshort) - 1;
                if (mshort)
                {
                    if (lseek(fd, -2, SEEK_CUR) == SEEK_ERROR)
                        return -EIO;
                    mshort = htons(mshort);
                    if (write(fd, &mshort, 2) != 2)
                        return -EIO;
                } else {
                    ret_value = freeBlocks(addr);
                    if (ret_value != 0)
                        return ret_value;
                }
            } else {
                ret_value = freeBlocks(addr);
                if (ret_value != 0)
                    return ret_value;
            }
            /* Remove the corresponding entry in the parent */
            if (lseek(fd, nextEntry, SEEK_SET) != nextEntry)
                return -EIO;
            if (read(fd, &addr, 4) != 4)
                return -EIO;
            if ((!addr) && (((quint32) nextEntry) == currentAddr + 4))
            {
                /* Empty part to remove */
                if (lseek(fd, beforeAddr, SEEK_SET) != beforeAddr)
                    return -EIO;
                if (write(fd, &next_block, 4) != 4)
                    return -EIO;
                ret_value = freeBlock(currentAddr - 4);
                if (ret_value != 0)
                    return ret_value;
            } else {
                /* Move following entries */
                len += 5;
                if (lseek(fd, -(len + 4), SEEK_CUR) == SEEK_ERROR)
                    return -EIO;
                if (write(fd, &addr, 4) != 4)
                    return -EIO;
                while (addr)
                {
                    if (lseek(fd, len, SEEK_CUR) == SEEK_ERROR)
                        return -EIO;
                    if (read(fd, &nameLen, 1) != 1)
                        return -EIO;
                    if (read(fd, &str_buffer, nameLen) != nameLen)
                        return -EIO;
                    if (write(fd, &addr, 4) != 4)
                        return -EIO;
                    if (lseek(fd, -(len + 5 + (int) nameLen), SEEK_CUR) == SEEK_ERROR)
                        return -EIO;
                    if (write(fd, &nameLen, 1) != 1)
                        return -EIO;
                    if (write(fd, &str_buffer, nameLen) != nameLen)
                        return -EIO;
                    if (write(fd, &addr, 4) != 4)
                        return -EIO;
                }
            }
            /* Change the last modification time */
            dirAddr += 8;
            if (lseek(fd, dirAddr, SEEK_SET) != dirAddr)
                return -EIO;
            addr = htonl(time(0));
            if (write(fd, &addr, 4) != 4)
                return -EIO;
            /* And the number of hard links if need be */
            if (isDir)
            {
                nlink = htons(nlink - 1);
                if (write(fd, &nlink, 2) != 2)
                    return -EIO;
            }
            return 0;
        }
    }
#endif /* READONLY_FS */
}

int MyFS::sOpenDir(const lString &pathname, int &fd)
{
    OpenFile myDir;
    lString shallowCopy = pathname;
    int ret_value = getAddress(shallowCopy, myDir.nodeAddr);
    if (ret_value != 0)
        return ret_value;
    if (lseek(this->fd, myDir.nodeAddr + 4, SEEK_SET) != myDir.nodeAddr + 4)
        return -EIO;
    if (read(this->fd, &myDir.nextAddr, 4) != 4)
        return -EIO;
    if (read(this->fd, str_buffer, 6) != 6)
        return -EIO;
    quint16 mshort;
    if (read(this->fd, &mshort, 4) != 4)
        return -EIO;
    mshort = ntohs(mshort);
    if (mshort & SF_MODE_REGULARFILE)
        return -ENOTDIR;
    fd = 0;
    while ((fd < openFiles.count()) && openFiles.at(fd).nodeAddr) ++fd;
    myDir.currentAddr = myDir.nodeAddr + 16;
    myDir.nextAddr = ntohl(myDir.nextAddr);
    myDir.isRegular = false;
    if (fd == openFiles.count())
    {
        if (fd > MAX_OPEN_FILES)
            return -ENFILE;
        openFiles.append(myDir);
    } else {
        openFiles[fd] = myDir;
    }
    return 0;
}

int MyFS::sReadDir(int fd, char *&name)
{
    if ((fd >= openFiles.count()) || (!openFiles.at(fd).nodeAddr) || openFiles.at(fd).isRegular)
        return -EBADF;
    OpenFile *file = &openFiles[fd];
    if (lseek(this->fd, file->currentAddr, SEEK_SET) != file->currentAddr)
        return -EIO;
    quint32 addr;
    while (true)
    {
        if (read(this->fd, &addr, 4) != 4)
            return -EIO;
        if (addr == 0)
        {
            if (!file->nextAddr)
            {
                name = NULL;
                return 0;
            }
            file->nextAddr += 4;
            if (lseek(this->fd, file->nextAddr, SEEK_SET) != file->nextAddr)
                goto ioerror;
            file->currentAddr = file->nextAddr + 4;
            if (read(this->fd, &file->nextAddr, 4) != 4)
                goto ioerror;
            file->nextAddr = ntohl(file->nextAddr);
            continue;
        }
        unsigned char sLen;
        if (read(this->fd, &sLen, 1) != 1)
            return -EIO;
        if (read(this->fd, str_buffer, sLen) != sLen)
            return -EIO;
        file->currentAddr += 5;
        file->currentAddr += sLen;
        str_buffer[sLen] = 0;
        name = str_buffer;
        return 0;
    }
ioerror:
    file->nodeAddr = 0;
    return -EIO;
}

int MyFS::sCloseDir(int fd)
{
    if ((fd >= openFiles.count()) || (!openFiles.at(fd).nodeAddr) || openFiles.at(fd).isRegular)
        return -EBADF;
    OpenFile *file = &openFiles[fd];
    file->nodeAddr = 0;
    while ((!openFiles.isEmpty()) && (!openFiles.last().nodeAddr))
        openFiles.removeLast();
    return 0;
}

char *MyFS::convStr(const QString &str)
{
    char *result = new char[str.length() + 1];
    QByteArray strData = str.toLocal8Bit();
    memcpy(result, strData.data(), str.length());
    result[str.length()] = 0;
    return result;
}

/* Allocates the block, writes its size in the first 4 bytes, 0 on the 4 next bytes, puts its address into addr and returns 0 on success.
    In this case, fd will point to the 9-th byte of that block at the end of the call. */
int MyFS::getBlock(quint32 size, quint32 &addr)
{
#if READONLY_FS
    Q_UNUSED(size);
    Q_UNUSED(addr);
    return -EROFS;
#else
    quint32 currentAddr = first_blank, refAddr = 4, bsize;
    while (true)
    {
        if (lseek(fd, currentAddr, SEEK_SET) != currentAddr)
            return -EIO;
        if (read(fd, &bsize, 4) != 4)
            return -EIO;
        bsize = ntohl(bsize);
        if (bsize >= size)
        {
            if (bsize >= size + MIN_BLOCK_SIZE)
            {
                /* We split the space in two parts */
                if (lseek(fd, currentAddr, SEEK_SET) != currentAddr)
                    return -EIO;
                bsize -= size;
                addr = currentAddr + bsize;
                bsize = htonl(bsize);
                if (write(fd, &bsize, 4) != 4)
                    return -EIO;
                if (lseek(fd, addr, SEEK_SET) != addr)
                    return -EIO;
                bsize = htonl(size);
                if (write(fd, &bsize, 4) != 4)
                    return -EIO;
                currentAddr = 0;
                if (write(fd, &currentAddr, 4) != 4)
                    return -EIO;
            } else {
                /* We use all the space */
                addr = currentAddr;
                if (read(fd, &currentAddr, 4) != 4)
                    return -EIO;
                if (lseek(fd, refAddr, SEEK_SET) != refAddr)
                    return -EIO;
                if (write(fd, &currentAddr, 4) != 4)
                    return -EIO;
                if (lseek(fd, addr + 4, SEEK_SET) != addr + 4)
                    return -EIO;
                bsize = 0;
                if (write(fd, &bsize, 4) != 4)
                    return -EIO;
            }
            return 0;
        }
        refAddr = currentAddr + 4;
        if (read(fd, &currentAddr, 4) != 4)
            return -EIO;
        currentAddr = ntohl(currentAddr);
    }
#endif /* READONLY_FS */
}

/* Frees the block at address addr and its following parts, and returns 0 on success. */
int MyFS::freeBlocks(quint32 addr)
{
    int ret_value;
    quint32 next_block;
    while (true)
    {
        if (lseek(fd, addr + 4, SEEK_SET) == SEEK_ERROR)
            return -EIO;
        if (read(fd, &next_block, 4) != 4)
            return -EIO;
        ret_value = freeBlock(addr);
        if (ret_value != 0)
            return ret_value;
        if (!next_block)
            break;
        addr = ntohl(next_block);
    }
    return 0;
}

/* Frees the block at address addr and returns 0 on success. */
int MyFS::freeBlock(quint32 addr)
{
#if READONLY_FS
    Q_UNUSED(addr);
    return -EROFS;
#else
    /* Get the length of the block to free */
    quint32 block_len;
    if (lseek(fd, addr, SEEK_SET) != addr)
        return -EIO;
    if (read(fd, &block_len, 4) != 4)
        return -EIO;
    block_len = ntohl(block_len);
    /* Search for the next free block */
    quint32 refAddr = 0, currentAddr = first_blank, prev_len = 0;
    while (currentAddr < addr)
    {
        refAddr = currentAddr;
        if (lseek(fd, currentAddr, SEEK_SET) != currentAddr)
            return -EIO;
        if (read(fd, &prev_len, 4) != 4)
            return -EIO;
        prev_len = ntohl(prev_len);
        if (read(fd, &currentAddr, 4) != 4)
            return -EIO;
        if (!currentAddr)
            break;
        currentAddr = ntohl(currentAddr);
    }
    /* Check merging requirements */
    if (refAddr + prev_len == addr)
    {
        if (addr + block_len == currentAddr)
        {
            /* Merge with the previous and the next free block */
            quint32 currentLen, currentNext;
            if (lseek(fd, currentAddr, SEEK_SET) != currentAddr)
                return -EIO;
            if (read(fd, &currentLen, 4) != 4)
                return -EIO;
            currentLen = ntohl(currentLen);
            if (read(fd, &currentNext, 4) != 4)
                return -EIO;
            if (lseek(fd, refAddr, SEEK_SET) != refAddr)
                return -EIO;
            block_len += prev_len;
            block_len += currentLen;
            block_len = htonl(block_len);
            if (write(fd, &block_len, 4) != 4)
                return -EIO;
            if (write(fd, &currentNext, 4) != 4)
                return -EIO;
        } else {
            /* Merge with the previous free block */
            if (lseek(fd, refAddr, SEEK_SET) != refAddr)
                return -EIO;
            block_len += prev_len;
            block_len = htonl(block_len);
            if (write(fd, &block_len, 4) != 4)
                return -EIO;
            currentAddr = htonl(currentAddr);
            if (write(fd, &currentAddr, 4) != 4)
                return -EIO;
        }
    } else {
        /* Change the link of the previous block */
        refAddr += 4;
        if (lseek(fd, refAddr, SEEK_SET) != refAddr)
            return -EIO;
        refAddr = htonl(addr);
        if (write(fd, &refAddr, 4) != 4)
            return -EIO;
        if (addr + block_len == currentAddr)
        {
            /* Merge with the next free block */
            quint32 currentLen, currentNext;
            if (lseek(fd, currentAddr, SEEK_SET) != currentAddr)
                return -EIO;
            if (read(fd, &currentLen, 4) != 4)
                return -EIO;
            currentLen = ntohl(currentLen);
            if (read(fd, &currentNext, 4) != 4)
                return -EIO;
            if (lseek(fd, addr, SEEK_SET) != addr)
                return -EIO;
            block_len += currentLen;
            block_len = htonl(block_len);
            if (write(fd, &block_len, 4) != 4)
                return -EIO;
            if (write(fd, &currentNext, 4) != 4)
                return -EIO;
        } else {
            /* Do not merge with anything */
            addr += 4;
            if (lseek(fd, addr, SEEK_SET) != addr)
                return -EIO;
            currentAddr = htonl(currentAddr);
            if (write(fd, &currentAddr, 4) != 4)
                return -EIO;
        }
    }
    return 0;
#endif /* READONLY_FS */
}

int MyFS::getAddress(lString &pathname, quint32 &result)
{
    if (pathname.str_value[0] != '/') return -ENOENT;
    while ((pathname.str_value[pathname.str_len - 1] == '/') && (pathname.str_len > 0))
        --pathname.str_len;
    if (pathname.str_len == 0)
    {
        result = root_address;
        return 0;
    }
    /* Checking whether or not the result is in the cache */
    QString sValue = QString::fromLocal8Bit(pathname.str_value, pathname.str_len);
    result = cache.value(sValue, 0);
    if (result != 0)
        return 0;
    /* Get the last part of the path */
    int len = pathname.str_len;
    while (pathname.str_value[pathname.str_len - 1] != '/')
        --pathname.str_len;
    len -= pathname.str_len;
    int start = pathname.str_len;
    /* Run this function recursively on the beginning of the path */
    int ret_value = getAddress(pathname, result);
    if (ret_value != 0)
        return ret_value;
    /* Look for the last part of the pathname in the parent directory */
    result += 4;
    if (lseek(fd, result, SEEK_SET) != result)
        return -EIO;
    if (read(fd, &result, 4) != 4)
        return -EIO;
    if (read(fd, &str_buffer, 6) != 6)
        return -EIO;
    quint16 mshort;
    if (read(fd, &mshort, 2) != 2)
        return -EIO;
    mshort = ntohs(mshort);
    if (!(mshort & SF_MODE_DIRECTORY))
        return -ENOTDIR;
    if (!(mshort & S_IXUSR))
        return -EACCES;
    while (true)
    {
        quint32 addr;
        if (read(fd, &addr, 4) != 4)
            return -EIO;
        if (!addr)
        {
            if (!result)
                return -ENOENT;
            result = ntohl(result) + 4;
            if (lseek(fd, result, SEEK_SET) != result)
                return -EIO;
            if (read(fd, &result, 4) != 4)
                return -EIO;
            continue;
        }
        unsigned char nameLen;
        if (read(fd, &nameLen, 1) != 1)
            return -EIO;
        if (read(fd, &str_buffer, nameLen) != nameLen)
            return -EIO;
        if ((nameLen == len) && (memcmp(pathname.str_value + start, str_buffer, len) == 0))
        {
            result = ntohl(addr);
            /* Store the result in the cache (we won't care about limiting the cache size) */
            cache[sValue] = result;
            return 0;
        }
    }
}
