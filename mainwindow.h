#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "host.h"
#include "datablock.h"
#include <QMainWindow>
#include <QTcpServer>
#include <QUdpSocket>
#include <QTcpSocket>
#include <string>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void debug(QString msg);
    void createNewHost(QString IP, QString MAC, QTcpSocket* socket);
    void updateHostsGui();
    void updateDatablocksGui();
    QString getMACFromIp(QString ipAdress);
    void TCPsend(QString msg, QTcpSocket* socket);
    void TCPsend(QByteArray msg, QTcpSocket* socket);
    void sendDataBlock(QTcpSocket* socket, DataBlock* db);
    void processTCPBuffer(QTcpSocket* socket);
    void protocolSend();
    void sendHas(QTcpSocket* socket);
    void sendAsk(QTcpSocket* socket, QString blockId);

private slots:
    void processUDPPendingDatagrams();
    void newTCPConnection();
    void readyReadTCP();
    void disconnectedTCP();
    void sendButtonClicked();

private:
    Ui::MainWindow *ui;
    QString localhostname;
    QString localhostIP; // To "fix"
    QList<QString> localhostAdresses;
    QString localMacAddress;
    QString localNetmask;
    QUdpSocket *udpSocket;

    QTcpServer* tcpServer; // Handler for incoming Tcp

    QMap<QString, Host*> hosts; // MAC to Host
    QMap<QTcpSocket*, QString> socketToMAC; // Socket to MAC

    QMap<QString, DataBlock*> dataRegister; // Datablock id to Datablock

};
#endif // MAINWINDOW_H
