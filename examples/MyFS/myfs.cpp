#include "myfs.h"

/* We will make it single-threaded to avoid any further concurrency issues */
MyFS::MyFS(QString mountPoint) : QSimpleFuse(mountPoint, true)
{
}
