/*
 * Copyright (c) 2015, Rémi Bazin <bazin.remi@gmail.com>
 * All rights reserved.
 * See LICENSE for licensing details.
 *
 * Source inspired by the qt-fuse example by qknight (https://github.com/qknight/qt-fuse-example)
 */

/*!
    \class QDaemon
    \inmodule SimpleFuse
    \ingroup SimpleFuse

    \brief QDaemon allows simple signal handling.

    To use this class, construct a single instance at the beginning of your program.
    As soon as it is created, it will catch all the INT, HUP and TERM signals.
    Link this class' signals to whatever functions you wish immediately after that.
    When you are done capturing those, just destruct the QDaemon object.
*/

#include "qdaemon.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <signal.h>

int QDaemon::sigintFd[2];
int QDaemon::sighupFd[2];
int QDaemon::sigtermFd[2];
volatile QDaemon *QDaemon::_instance = 0;
struct sigaction QDaemon::oldSigInt, QDaemon::oldSigHup, QDaemon::oldSigTerm;
bool QDaemon::pairsDone = false;

/*!
    Constructs and initialize a new QDaemon object with parent \a parent, capturing the INT, HUP and TERM signals.

    \warning Only one single instance at a time can be created / used.
*/
QDaemon::QDaemon(QObject *parent) : QObject(parent)
{
    /* Check whether or not this is the only instance (never mind the slight concurrency threat) */
    if (_instance)
    {
        qFatal("MyDaemon already has an active instance!");
        exit(1);
    }
    _instance = this;
    /* Create socket pairs, if not already done */
    if (!pairsDone)
    {
        if (socketpair(AF_UNIX, SOCK_STREAM, 0, sigintFd))
        {
            qFatal("Couldn't create INT socketpair");
            exit(1);
        }
        if (socketpair(AF_UNIX, SOCK_STREAM, 0, sighupFd))
        {
            qFatal("Couldn't create HUP socketpair");
            exit(1);
        }
        if (socketpair(AF_UNIX, SOCK_STREAM, 0, sigtermFd))
        {
            qFatal("Couldn't create TERM socketpair");
            exit(1);
        }
        pairsDone = true;
    }
    /* Create and connect notifiers to the end of these pairs */
    snInt = new QSocketNotifier(sigintFd[1], QSocketNotifier::Read, this);
    connect(snInt, SIGNAL(activated(int)), this, SLOT(handleSigInt()));
    snHup = new QSocketNotifier(sighupFd[1], QSocketNotifier::Read, this);
    connect(snHup, SIGNAL(activated(int)), this, SLOT(handleSigHup()));
    snTerm = new QSocketNotifier(sigtermFd[1], QSocketNotifier::Read, this);
    connect(snTerm, SIGNAL(activated(int)), this, SLOT(handleSigTerm()));
    /* Start capturing, thus linking the signals to the other end of the pairs */
    /* (never mind the slight timeout between this and the QObject::connect that is following) */
    struct sigaction sigInt, sigHup, sigTerm;
    sigInt.sa_handler = intSignalHandler;
    sigemptyset(&sigInt.sa_mask);
    sigInt.sa_flags = 0;
    sigInt.sa_flags |= SA_RESTART;
    if (sigaction(SIGINT, &sigInt, &oldSigInt))
    {
        qFatal("Error while installing the signal handle for SIGINT");
        exit(1);
    }
    sigHup.sa_handler = hupSignalHandler;
    sigemptyset(&sigHup.sa_mask);
    sigHup.sa_flags = 0;
    sigHup.sa_flags |= SA_RESTART;
    if (sigaction(SIGHUP, &sigHup, &oldSigHup))
    {
        qFatal("Error while installing the signal handle for SIGHUP");
        exit(1);
    }
    sigTerm.sa_handler = termSignalHandler;
    sigemptyset(&sigTerm.sa_mask);
    sigTerm.sa_flags = 0;
    sigTerm.sa_flags |= SA_RESTART;
    if (sigaction(SIGTERM, &sigTerm, &oldSigTerm))
    {
        qFatal("Error while installing the signal handle for SIGTERM");
        exit(1);
    }
}

/*!
    Destructs the object and stops capturing signals.
*/
QDaemon::~QDaemon()
{
    /* Restore previous handlers */
    if (sigaction(SIGHUP, &oldSigInt, 0))
    {
        qFatal("Error while restoring the signal handle for SIGINT");
        exit(1);
    }
    if (sigaction(SIGHUP, &oldSigHup, 0))
    {
        qFatal("Error while restoring the signal handle for SIGHUP");
        exit(1);
    }
    if (sigaction(SIGHUP, &oldSigTerm, 0))
    {
        qFatal("Error while restoring the signal handle for SIGTERM");
        exit(1);
    }
    /* Delete notifiers */
    delete snInt;
    delete snHup;
    delete snTerm;
    /* Allow future new instances */
    _instance = 0;
}

/*!
    \fn QDaemon::sigINT()

    This signals is emitted when the application receives an INT signal.
*/

/*!
    \fn QDaemon::sigHUP()

    This signals is emitted when the application receives a HUP signal.
*/

/*!
    \fn QDaemon::sigTERM()

    This signals is emitted when the application receives a TERM signal.
*/

void QDaemon::intSignalHandler(int)
{
    quint8 a = 1;
    if (write(sigintFd[0], &a, 1) != 1)
    {
        qt_noop(); /* Signal ignored */
    }
}

void QDaemon::hupSignalHandler(int)
{
    quint8 a = 1;
    if (write(sighupFd[0], &a, 1) != 1)
    {
        qt_noop(); /* Signal ignored */
    }
}

void QDaemon::termSignalHandler(int)
{
    quint8 a = 1;
    if (write(sigtermFd[0], &a, 1) != 1)
    {
        qt_noop(); /* Signal ignored */
    }
}

void QDaemon::handleSigInt()
{
    snInt->setEnabled(false);
    quint8 tmp;
    if (read(sigintFd[1], &tmp, 1) != 1) qt_noop();
    emit sigINT();
    snInt->setEnabled(true);
}

void QDaemon::handleSigHup()
{
    snHup->setEnabled(false);
    quint8 tmp;
    if (read(sighupFd[1], &tmp, 1) != 1) qt_noop();
    emit sigHUP();
    snHup->setEnabled(true);
}

void QDaemon::handleSigTerm()
{
    snTerm->setEnabled(false);
    quint8 tmp;
    if (read(sigtermFd[1], &tmp, 1) != 1) qt_noop();
    emit sigTERM();
    snTerm->setEnabled(true);
}
