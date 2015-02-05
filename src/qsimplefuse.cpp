#include "qsimplefuse.h"

#include <stdio.h>

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
