#ifndef NETWORKTHREAD_H
#define NETWORKTHREAD_H

#include "mainwindow.h"
#include <QThread>
#include <QUdpSocket>

class networkThread : public QThread{
public:
    networkThread(MainWindow* window);

protected:
    void run();

private slots:
    void processPendingDatagrams();

private:
    MainWindow* window;
    QUdpSocket* udpSock;

};

#endif // NETWORKTHREAD_H
