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

#define DIR_BLOCK_SIZE 0x400

static char str_buffer[0xFF];

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
    /* /../ is the same as / (root directory) */
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
    fprintf(stderr, "Entered sGetAttr with %s\n", pathname.str_value);
    if (fd < 0) return -EIO;
    quint32 addr;
    lString shallowCopy = pathname;
    int ret_value = getAddress(shallowCopy, addr);
    if (ret_value != 0)
        return ret_value;
    addr += 4;
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
    fprintf(stderr, "Out of sGetAttr\n");
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
    int start = pathname.str_len + 1;
    /* Run this function recursively on the beginning of the path */
    int ret_value = getAddress(pathname, result);
    if (ret_value != 0)
        return ret_value;
    /* Look for the last part of the pathname in the parent directory */
    result += 10;
    if (lseek(fd, result, SEEK_SET) != result)
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
        if (addr == 0)
            return -ENOENT;
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
