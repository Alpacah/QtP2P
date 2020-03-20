#include "datablock.h"
#include <QDateTime>
#include <QCryptographicHash>

DataBlock::DataBlock(QString author, QString content, QDateTime* date){
    // Use this constructor if id is not provided, build id
    QString hashSeed = QString::number(this->date->currentMSecsSinceEpoch());
    hashSeed += author;
    hashSeed += content;
    QByteArray hashedSeed = QCryptographicHash::hash(hashSeed.toUtf8(), QCryptographicHash::Sha1);
    QString id = hashedSeed.toHex();
    id = id.left(8);

    this->id = id;
    this->author = author;
    this->content = content;
    this->date = date;
}

DataBlock::DataBlock(QString id, QString author, QString content, QDateTime* date){
    // Use this constructor if id is provided (received)
    this->id = id;
    this->author = author;
    this->content = content;
    this->date = date;
}

QString DataBlock::toString(){
    QString output = this->date->toString("[hh:mm:ss] ");
    output += this->author;
    output += QString::fromStdString(": ");
    output += this->content;
    return output;
}
