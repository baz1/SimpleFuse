#ifndef MYFS_H
#define MYFS_H

#include "sfuse/qsimplefuse.h"

/*
    This implementation is an example of the usage of QSimpleFuse.
    As such, it is not efficient at all and not recommended for any real use.

    It works as follows:
    Each data block starts with its size on 4 bytes (network byte order ie. big endian).
    There are three types of data blocks : FREE_BLOCK (free memory) and FILE_BLOCK (file).
    The 8 very first bytes of the file however are not part of any block.
    They are used to define the address of the root directory / (bytes 0-3) and the first free block (bytes 4-7).

    FREE_BLOCK:
        Its size is followed by the address of the next free block (4 bytes) and useless bytes.
        A null value is written if there is no such next free block.

    FILE_BLOCK:
        Its size is followed by the following bytes:
        4-7: Last modification time
        8-9: Number of hard links
        10-11: SF_MODE_DIRECTORY for directory, SF_MODE_REGULARFILE for file, plus access rights

        If this is a directory, for each entry:
            * Address (4 bytes) (0 to end the list)
            * Name length (NULL terminating byte included) (1 byte)
            * Name (C string)

        If this is a regular file:
            * The size of its data (4 bytes)
            * Its data
*/

class MyFS : public QSimpleFuse
{
public:
    MyFS(QString mountPoint, QString filename);
    ~MyFS();
    static void createNewFilesystem(QString filename);
    void sInit();
    void sDestroy();
private:
    static char *convStr(const QString &str);
private:
    char *filename;
    int fd;
    quint32 root_address, first_blank;
};

#endif // MYFS_H
