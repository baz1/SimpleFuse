#include "qsimplefuse.h"

#include <stdio.h>
#define FUSE_USE_VERSION 26
#include <fuse.h>

QSimpleFuse::QSimpleFuse(QString mountPoint)
{
    /* Check whether or not this is a new instance */
    if (QSimpleFuse::_instance)
    {
#ifndef QT_NO_DEBUG
        fprintf(stderr, "Error: Attempted to create a second instance of QSimpleFuse.\n");
#endif
        is_ok = false;
        return;
    }
    is_ok = true;
}

bool QSimpleFuse::checkStatus()
{
    return is_ok;
}
