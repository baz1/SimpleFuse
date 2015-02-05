/*
 * Copyright (c) 2015, Rémi Bazin <bazin.remi@gmail.com>
 * All rights reserved.
 * See LICENSE for licensing details.
 *
 * Source inspired by the qt-fuse example by qknight (https://github.com/qknight/qt-fuse-example)
 */

#include "qsimplefuse.h"

#include <stdio.h>
#define FUSE_USE_VERSION 26
#include <fuse.h>

static struct fuse_server {
    pthread_t pid;
    struct fuse *fuse;
    struct fuse_chan *ch;
    int failed;
    char *mountpoint;
    int multithreaded;
    int foreground;
} fs;

static void *fuse_thread(void *arg)
{
    Q_UNUSED(arg);
    if (fs.multithreaded)
    {
        if (fuse_loop_mt(fs.fuse) < 0)
        {
#ifndef QT_NO_DEBUG
            perror("fuse_loop_mt");
#endif
            fs.failed = 1;
        }
    } else {
        if (fuse_loop(fs.fuse) < 0)
        {
#ifndef QT_NO_DEBUG
            perror("fuse_loop");
#endif
            fs.failed = 1;
        }
    }
    return NULL;
}

QSimpleFuse * volatile QSimpleFuse::_instance = NULL;

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
    QSimpleFuse::_instance = this;
    // TODO
    is_ok = true;
}

QSimpleFuse::~QSimpleFuse()
{
}

bool QSimpleFuse::checkStatus()
{
    return is_ok;
}
