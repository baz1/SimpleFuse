/*
 * Copyright (c) 2015, Rémi Bazin <bazin.remi@gmail.com>
 * All rights reserved.
 * See LICENSE for licensing details.
 *
 * Source inspired by the qt-fuse example by qknight (https://github.com/qknight/qt-fuse-example)
 */

#include "qsimplefuse.h"

#include <string.h>
#include <QCoreApplication>
#include <QStringList>

#define FUSE_USE_VERSION 26
#include <fuse.h>
#include <fuse/fuse_lowlevel.h>


#ifndef QT_NO_DEBUG
#include <stdio.h>
#endif

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

char *getCStrFromQStr(const QString &str)
{
    QByteArray strData = str.toLocal8Bit();
    char *result = new char[strData.length() + 1];
    memcpy(result, strData.constData(), strData.length());
    result[strData.length()] = 0;
    return result;
}

QSimpleFuse::QSimpleFuse(QString mountPoint, bool singlethreaded)
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
    /* Initialize fs */
    memset(&fs, 0, sizeof(fs));
    /* Recreating custom arguments */
    argc = singlethreaded ? 3 : 2;
    argv = new char*[argc];
    argv[0] = getCStrFromQStr(QCoreApplication::arguments().first());
    argv[1] = getCStrFromQStr(mountPoint);
    if (singlethreaded)
        argv[2] = getCStrFromQStr("-s");
    /* Parsing arguments */
    struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
    int res = fuse_parse_cmdline(&args, &fs.mountpoint, &fs.multithreaded, &fs.foreground);
    if (res == -1)
    {
#ifndef QT_NO_DEBUG
        fprintf(stderr, "QSimpleFuse: Error on fuse_parse_cmdline.\n");
#endif
        is_ok = false;
        return;
    }
    /* Mounting FS */
    fs.ch = fuse_mount(fs.mountpoint, &args);
    if (!fs.ch)
    {
#ifndef QT_NO_DEBUG
        perror("fuse_mount");
#endif
        is_ok = false;
        return;
    }
    makeSimplifiedFuseOperations();
    fs.fuse = fuse_new(fs.ch, &args, getSimplifiedFuseOperations(), sizeof(fuse_operations), NULL);
    fuse_opt_free_args(&args);
    if (!fs.fuse)
    {
#ifndef QT_NO_DEBUG
        perror("fuse_new");
#endif
        goto cancelmount;
    }
    if (pthread_create(&fs.pid, NULL, fuse_thread, NULL) != 0)
    {
#ifndef QT_NO_DEBUG
        perror("pthread_create");
#endif
        goto cancelmount;
    }
    is_ok = true;
    return;
cancelmount:
    fuse_unmount(fs.mountpoint, fs.ch);
    is_ok = false;
}

QSimpleFuse::~QSimpleFuse()
{
    if (is_ok)
    {
        /* Aborting FS */
        fuse_session_exit(fuse_get_session(fs.fuse));
        fuse_unmount(fs.mountpoint, fs.ch);
        pthread_join(fs.pid, NULL);
        fs.fuse = NULL;
        /* Allow a future new instance */
        if (_instance == this)
            _instance = NULL;
        is_ok = false;
    }
    if (argv)
    {
        for (int i = 0; i < argc; ++i)
            delete[] argv[i];
        delete[] argv;
        argv = NULL;
    }
}

bool QSimpleFuse::checkStatus()
{
    return is_ok && (!fs.failed);
}
