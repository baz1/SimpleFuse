#ifndef MYFS_H
#define MYFS_H

#include "sfuse/qsimplefuse.h"

class MyFS : public QSimpleFuse
{
public:
    MyFS(QString mountPoint, QString filename);
    ~MyFS();
    void sInit();
private:
    char *filename;
};

#endif // MYFS_H
