#include "networkthread.h"
#include "mainwindow.h"
#include "winsock2.h"
#include <QUdpSocket>
#include <iostream>
#include <conio.h>

#pragma comment(lib, "Ws2_32.lib")

#define PORT 4242

networkThread::networkThread(MainWindow* window){
    this->window = window;

    // UDP socket setting up
    udpSock = new QUdpSocket(this);
    udpSock->bind(PORT, QUdpSocket::ShareAddress);
    connect(udpSock, SIGNAL(readyRead()), this, SLOT(processPendingDatagrams()));

    this->window->debug("Binded UDP listener");
}

void networkThread::run(){
    // Send sequence
    QByteArray dgram = "hello";
    udpSock->writeDatagram(dgram.data(), dgram.size(), QHostAddress::Broadcast, PORT);
    this->window->debug("Broadcasted datagram");

    this->window->debug("Closing network Thread");
}

void networkThread::processPendingDatagrams(){
    while (udpSock->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(udpSock->pendingDatagramSize());
        udpSock->readDatagram(datagram.data(), datagram.size());

        this->window->debug("Incoming datagram:");
        this->window->debug(datagram.data());
    }
}
