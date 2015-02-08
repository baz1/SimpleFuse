#include "myfs.h"

#include <stdio.h>
#include <string.h>

#include <QByteArray>

/* We will make it single-threaded to avoid any further concurrency issues */
MyFS::MyFS(QString mountPoint, QString filename) : QSimpleFuse(mountPoint, true)
{
    this->filename = new char[filename.length() + 1];
    QByteArray filenameData = filename.toLocal8Bit();
    memcpy(this->filename, filenameData.data(), filename.length());
    this->filename[filename.length()] = 0;
}

MyFS::~MyFS()
{
    // TODO
}

void MyFS::sInit()
{
    // TODO
    delete[] filename;
    // TODO
}
