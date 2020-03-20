#include "host.h"
#include <QString>
#include <QTcpSocket>

Host::Host(QString MAC, QString IP, QTcpSocket* socket){
    this->MAC = MAC;
    this->IP = IP;
    this->socket = socket;

    this->buffer = new QByteArray;
    this->sizeBuffer = new qint32(0);
}

