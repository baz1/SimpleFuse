#ifndef MYFS_H
#define MYFS_H

#include "sfuse/qsimplefuse.h"

class MyFS : public QSimpleFuse
{
public:
    MyFS(QString mountPoint);
};

#endif // MYFS_H
