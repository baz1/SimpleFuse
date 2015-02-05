/*
 * Copyright (c) 2015, Rémi Bazin <bazin.remi@gmail.com>
 * All rights reserved.
 * See LICENSE for licensing details.
 */

#ifndef __QSIMPLEFUSE_H__
#define __QSIMPLEFUSE_H__

#include <QString>

#define QSF_STATUS_OK     true
#define QSF_STATUS_ERROR  false

class QSimpleFuse
{
public:
    explicit QSimpleFuse(QString mountPoint, bool singlethreaded = false);
    ~QSimpleFuse();
    bool checkStatus();
private:
    bool is_ok;
private:
    static QSimpleFuse * volatile _instance;
};

#endif /* Not __QSIMPLEFUSE_H__ */
