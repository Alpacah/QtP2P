#ifndef HOST_H
#define HOST_H

#include <QString>
#include <QTcpSocket>

class Host{
public:
    Host(QString MAC, QString IP, QTcpSocket* socket);

    QString MAC;
    QString IP;
    QTcpSocket* socket;

    QByteArray *buffer;
    qint32 *sizeBuffer;
};

#endif // HOST_H
