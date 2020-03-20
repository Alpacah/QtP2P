#ifndef DATABLOCK_H
#define DATABLOCK_H

#include <QString>
#include <QDateTime>

class DataBlock{
public:
    DataBlock(QString author, QString content, QDateTime* date);
    DataBlock(QString id, QString author, QString content, QDateTime* date);
    QString toString();

    QString id; // Assigned during creation
    QString author; // MAC adress of the author
    QString content; // Content of the datablock
    QDateTime* date; // Emission dateTime
};

#endif // DATABLOCK_H
