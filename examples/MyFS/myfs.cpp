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

char *MyFS::convStr(const QString &str)
{
    char *result = new char[str.length() + 1];
    QByteArray strData = str.toLocal8Bit();
    memcpy(result, strData.data(), str.length());
    result[str.length()] = 0;
    return result;
}
