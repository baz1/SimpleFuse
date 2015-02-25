/*
 * Copyright (c) 2015, Rémi Bazin <bazin.remi@gmail.com>
 * All rights reserved.
 * See LICENSE for licensing details.
 *
 * Source inspired by the qt-fuse example by qknight (https://github.com/qknight/qt-fuse-example)
 */

#ifndef __QDAEMON_H__
#define __QDAEMON_H__

#include <QSocketNotifier>

class QDaemon : public QObject
{
    Q_OBJECT
public:
    QDaemon(QObject *parent = 0);
    ~QDaemon();
signals:
    /* Qt signaling */
    void sigINT();
    void sigHUP();
    void sigTERM();
private:
    /* Unix signal handlers */
    static void intSignalHandler(int unused);
    static void hupSignalHandler(int unused);
    static void termSignalHandler(int unused);
private slots:
    /* Qt signal handlers */
    void handleSigInt();
    void handleSigHup();
    void handleSigTerm();
private:
    static int sigintFd[2];
    static int sighupFd[2];
    static int sigtermFd[2];
    static volatile QDaemon *instance;
    static struct sigaction oldSigInt, oldSigHup, oldSigTerm;
    static bool pairsDone;
private:
    QSocketNotifier *snInt;
    QSocketNotifier *snHup;
    QSocketNotifier *snTerm;
};

#endif /* Not __QDAEMON_H__ */
