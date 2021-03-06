#ifndef MYFS_H
#define MYFS_H

#include "sfuse/qsimplefuse.h"

#include <QHash>

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
        4-7: Address of the next part of the file (or 0 if this is the last part)

        And then, for each first part:

        8-11: Last modification time
        12-13: Number of hard links
        14-15: SF_MODE_DIRECTORY for directory, SF_MODE_REGULARFILE for file, plus access rights

        If this is a directory, for each entry: (starts here for next parts)
            * Address (4 bytes) (0 to end the list, which might then be continued on the next part of this "directory file")
            * Name length (NULL terminating byte excluded) (1 byte)
            * Name (without the NULL terminating byte)

        If this is a regular file:
            * The size of its data (4 bytes)
            * Its data (starts here for next parts)
*/

struct OpenFile
{
    quint32 nodeAddr;
    quint32 partAddr; /* Only used in regular files */
    quint32 partLength; /* Only used in regular files */
    quint32 nextAddr;
    quint32 currentAddr;
    quint32 partOffset; /* Only used in regular files */
    quint32 fileLength; /* Only used in regular files */
    quint8 flags; /* Only used in regular files (see constants below) */
    bool isRegular;
};

#define OPEN_FILE_FLAGS_PREAD    1
#define OPEN_FILE_FLAGS_PWRITE   2
#define OPEN_FILE_FLAGS_NOATIME  4
#define OPEN_FILE_FLAGS_MODIFIED 8

class MyFS : public QSimpleFuse
{
public:
    MyFS(QString mountPoint, QString filename);
    ~MyFS();
    static void createNewFilesystem(QString filename);
    void sInit();
    void sDestroy();
    int sGetSize(quint64 &size, quint64 &free);
    int sGetAttr(const lString &pathname, sAttr &attr);
    int sMkFile(const lString &pathname, quint16 mst_mode);
    int sRmFile(const lString &pathname, bool isDir);
    int sMvFile(const lString &pathBefore, const lString &pathAfter);
    int sLink(const lString &pathFrom, const lString &pathTo);
    int sChMod(const lString &pathname, quint16 mst_mode);
    int sTruncate(const lString &pathname, quint64 newsize);
    int sUTime(const lString &pathname, time_t mst_atime, time_t mst_mtime);
    int sOpen(const lString &pathname, int flags, quint32 &fd);
    int sRead(quint32 fd, void *buf, quint32 count, quint64 offset);
    int sWrite(quint32 fd, const void *buf, quint32 count, quint64 offset);
    int sSync(quint32 fd);
    int sClose(quint32 fd);
    int sOpenDir(const lString &pathname, quint32 &fd);
    int sReadDir(quint32 fd, char *&name);
    int sCloseDir(quint32 fd);
    int sAccess(const lString &pathname, quint8 mode);
    int sFTruncate(quint32 fd, quint64 newsize);
    int sFGetAttr(quint32 fd, sAttr &attr);
private:
    int myLink(quint32 file, const lString &pathname, quint32 *parentAddr = 0);
    int myUnlink(const lString &pathname, bool &isDir, quint32 *nodeAddr = 0);
    int myGetAttr(quint32 addr, sAttr &attr);
    int myTruncate(quint32 addr, quint32 newsize);
    bool setPosition(OpenFile &file, quint32 offset);
    bool myWriteB(quint32 size);
    static char *convStr(const QString &str);
    int getBlocks(quint32 size, quint32 &addr);
    int getBlock(quint32 size, quint32 &addr);
    int freeBlocks(quint32 addr);
    int freeBlock(quint32 addr);
    /* Warning: the following function does not preserve pathname (length changed) */
    int getAddress(lString &pathname, quint32 &result);
private:
    char *filename;
    int fd;
    quint32 root_address, first_blank;
    QHash<QString, quint32> cache;
    QList<OpenFile> openFiles;
};

#endif // MYFS_H
