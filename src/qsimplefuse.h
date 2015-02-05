#ifndef __QSIMPLEFUSE_H__
#define __QSIMPLEFUSE_H__

#include <QString>

class QSimpleFuse
{
public:
    explicit QSimpleFuse(QString mountPoint);
    ~QSimpleFuse();
private:
    bool is_ok;
private:
    static QSimpleFuse * volatile _instance;
};

#endif /* Not __QSIMPLEFUSE_H__ */
