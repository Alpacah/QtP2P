#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "host.h"
#include "datablock.h"
#include <string>
#include <QApplication>
#include <QUdpSocket>
#include <QTcpServer>
#include <QTcpSocket>
#include <QHostInfo>
#include <QProcess>
#include <QNetworkInterface>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>

#define UDPPORT 4242
#define TCPPORT 2424

MainWindow::MainWindow(QWidget *parent): QMainWindow(parent), ui(new Ui::MainWindow){
    // Setup GUI
    ui->setupUi(this);
    connect(ui->sendButton, SIGNAL(clicked()), this, SLOT(sendButtonClicked()));
    this->debug("GUI setup OK");

    // Get locahost network infos
    localhostname = QHostInfo::localHostName();
    QList<QHostAddress> hostList = QHostInfo::fromName(localhostname).addresses();
    foreach (const QHostAddress& address, hostList) {
       if (address.protocol() == QAbstractSocket::IPv4Protocol && address.isLoopback() == false) {
            localhostIP = address.toString();
       }
       localhostAdresses.append(address.toString());
    }
    foreach (const QNetworkInterface& networkInterface, QNetworkInterface::allInterfaces()) {
       foreach (const QNetworkAddressEntry& entry, networkInterface.addressEntries()) {
           if (entry.ip().toString() == localhostIP) {
               localMacAddress = networkInterface.hardwareAddress();
               localNetmask = entry.netmask().toString();
               break;
           }
       }
    }
    this->debug("Local network config fetched:");
    this->debug(QString::fromStdString("  Localhost name: ") + localhostname);
    this->debug(QString::fromStdString("  IP = ") + localhostIP);
    this->debug(QString::fromStdString("  MAC = ") + localMacAddress);
    this->debug(QString::fromStdString("  NetMask = ") + localNetmask);

    // Create TCP Server
    tcpServer = new QTcpServer(this);
    connect(tcpServer, SIGNAL(newConnection()), this, SLOT(newTCPConnection()));
    tcpServer->listen(QHostAddress::Any, TCPPORT);
    this->debug(QString::fromStdString("TCP server listening on: ") +  QString::number(TCPPORT));

    // Create UDP socket
    udpSocket = new QUdpSocket(this);
    udpSocket->bind(UDPPORT, QUdpSocket::ShareAddress);
    connect(udpSocket, SIGNAL(readyRead()), this, SLOT(processUDPPendingDatagrams()));
    this->debug("UDP socket connect OK");

    // Send a "UDP discover"
    QString discoverRequest = QString::fromStdString("[DISCOVER]");
    QByteArray datagram = discoverRequest.toUtf8();
    udpSocket->writeDatagram(datagram.data(), datagram.size(), QHostAddress::Broadcast, UDPPORT);
    this->debug(QString::fromStdString("UDP discover message SENT on port ") + QString::number(UDPPORT));
}

void MainWindow::debug(QString msg){
    qDebug(msg.toLatin1());
    this->ui->debugListWidget->addItem(msg);
}

void MainWindow::processUDPPendingDatagrams(){
    // UDP Incoming packets handler
    while (udpSocket->hasPendingDatagrams()) {
        // Grab datagram
        QByteArray datagram;
        QHostAddress grabbedIPQHost;
        datagram.resize(udpSocket->pendingDatagramSize());
        udpSocket->readDatagram(datagram.data(), datagram.size(), &grabbedIPQHost, NULL);
        QString data = QString(datagram);

        // Fetch IP and MAC from sender
        QString grabbedIP = QHostAddress(grabbedIPQHost.toIPv4Address()).toString();
        QString grabbedMAC = this->getMACFromIp(grabbedIP);

        // Debug it
        this->debug("Incoming UDP broadcast msg:");
        this->debug(QString::fromStdString("  ") + datagram.data());

        // Action routing
        if (!localhostAdresses.contains(grabbedIP)){
            if (data == "[DISCOVER]"){ // [DISCOVER] is broadcasted when a client is launched
                // TCP Connect to the distant host
                QTcpSocket* socket = new QTcpSocket(this);
                connect(socket, SIGNAL(readyRead()), SLOT(readyReadTCP()));
                connect(socket, SIGNAL(disconnected()), SLOT(disconnectedTCP()));

                socket->connectToHost(grabbedIP, TCPPORT);
                if (socket->waitForConnected(3000)){
                    this->createNewHost(grabbedIP, grabbedMAC, socket);

                    this->sendHas(socket);
                }else{
                    this->debug(QString::fromStdString("  Error: unable to connect to host: ") + grabbedMAC + QString::fromStdString(" ") + grabbedIP);
                }
            }else{
                this->debug(QString::fromStdString("  Error: unknown action ") + data);
            }
        }else{
            this->debug("  This packet is our own, ignoring");
        }
    }
}

void MainWindow::newTCPConnection(){
    // TCP Incoming new connection handler
    this->debug("New TCP Connection");

    while (tcpServer->hasPendingConnections()){
        // Create a socket with the new connection
        QTcpSocket *socket = tcpServer->nextPendingConnection();
        connect(socket, SIGNAL(readyRead()), SLOT(readyReadTCP()));
        connect(socket, SIGNAL(disconnected()), SLOT(disconnectedTCP()));

        // Creating a new corresponding host if it don't exists
        QString IP = socket->peerAddress().toString().replace("::ffff:", "");
        QString MAC = getMACFromIp(IP);

        this->createNewHost(IP, MAC, socket);
    }
}

QString MainWindow::getMACFromIp(QString ipAddress){
    // Get MAC from IP using ARP
    QString MAC;

    // Check if the IP belongs to us
    if (localhostAdresses.contains(ipAddress)){
        MAC = localMacAddress;
    }else{
        QProcess process;
        process.start(QString("arp -a %1").arg(ipAddress));
        if (process.waitForFinished()){
            QString result = process.readAll();
            QStringList list = result.split(QRegularExpression("\\s+"));
            if(list.contains(ipAddress))
                MAC = list.at(list.indexOf(ipAddress) + 1);
        }

        // 00-e8-9a [...] -> 00:E8:9A [...]
        MAC = MAC.replace("-", ":");
        MAC = MAC.toUpper();
    }

    return MAC;
}

void MainWindow::TCPsend(QByteArray dataToSend, QTcpSocket *socket){
    QByteArray sizeByteArray;
    qint32 size = 0;
    sizeByteArray.setNum(size);
    sizeByteArray.resize(sizeof(qint32));
    size = sizeByteArray.size() + dataToSend.size();
    sizeByteArray.setNum(size);
    sizeByteArray.resize(sizeof(qint32));
    dataToSend = sizeByteArray + dataToSend;
    socket->write(dataToSend);

    this->debug(QString::fromStdString("Sent TCP message of size: ") + QString::number(size) + QString::fromStdString("|") + QString::number(dataToSend.size()));

}

void MainWindow::TCPsend(QString msg, QTcpSocket* socket){
    this->TCPsend(msg.toUtf8(), socket);
}

void MainWindow::readyReadTCP(){
    this->debug("ReadyReadTCP");

    QTcpSocket* socket = static_cast<QTcpSocket*>(sender());
    Host* peer = this->hosts[this->socketToMAC[socket]];

    while (socket->bytesAvailable()){
        peer->buffer = new QByteArray(*peer->buffer + socket->readAll()); // dirty, to fix
        this->processTCPBuffer(socket);
    }
}

void MainWindow::processTCPBuffer(QTcpSocket* socket){
    Host* peer = this->hosts[this->socketToMAC[socket]];
    this->debug(QString::fromStdString("  Buffer state: ") + QString::fromStdString(peer->buffer->toStdString()));

    if (*peer->sizeBuffer == 0){
        // We need to fetch the message size
        if ((unsigned int)peer->buffer->size() >= sizeof(qint32)){
            QByteArray sizeByteArray = peer->buffer->left(sizeof(qint32));
            *peer->buffer = peer->buffer->right(peer->buffer->size() - sizeof(qint32));
            *peer->sizeBuffer = sizeByteArray.toInt();
            this->debug(QString::fromStdString("  Fetched size: ") + QString::number(*peer->sizeBuffer));

            this->processTCPBuffer(socket);
        }
    }else{
        // We are now fetching the message content
        this->debug(QString::fromStdString("  Now fetching content ") + QString::number(peer->buffer->size() + sizeof(qint32)) + QString::fromStdString("/") + QString::number(*peer->sizeBuffer));
        if (peer->buffer->size() + sizeof(qint32) >= (unsigned int)*peer->sizeBuffer){
            QByteArray dataByteArray = peer->buffer->left(*peer->sizeBuffer - sizeof(qint32));
            QString data = QString::fromStdString(dataByteArray.toStdString());

            // Process message
            QJsonDocument jsonDoc = QJsonDocument::fromJson(dataByteArray);
            QJsonObject jsonData = jsonDoc.object();
            this->debug(jsonData["tag"].toString());

            if (jsonData["tag"].toString() == "[BLOCK]"){
                // A block is transmitted
                QString id = jsonData["id"].toString();
                QString author = jsonData["author"].toString();
                QString content = jsonData["content"].toString();
                qint64 msSinceEpoch = jsonData["date"].toString().toLongLong();
                QDateTime* date = new QDateTime(QDateTime::fromMSecsSinceEpoch(msSinceEpoch));

                DataBlock* db = new DataBlock(id, author, content, date);
                this->dataRegister[db->id] = db;

                this->updateDatablocksGui();
                this->debug(QString::fromStdString("Received block ") + db->id + QString::fromStdString(" from ") + this->socketToMAC[socket] + QString::fromStdString(" msSinceEpoch: ") + QString::number(msSinceEpoch) + QString::fromStdString(" ") + jsonData["date"].toString());
            }else if (jsonData["tag"].toString() == "[ASK]"){
                // Ask for a certain block - TODO: secure by checking if we effectively own it
                this->debug(QString::fromStdString("  ") + jsonData["id"].toString());
                this->sendDataBlock(peer->socket, this->dataRegister[jsonData["id"].toString()]);
            }else if (jsonData["tag"].toString() == "[HAS]"){
                // Lists the blocks that a host owns
                QJsonArray blocks = jsonData["blocks"].toArray();

                for (int i = 0; i < blocks.count(); i++){
                    QString blockId = blocks.at(i).toString();

                    if (!this->dataRegister.keys().contains(blockId)){
                        this->debug(QString::fromStdString("  ") + blockId + QString::fromStdString(" | MISSING"));
                        this->sendAsk(socket, blockId);
                    }else{
                        this->debug(QString::fromStdString("  ") + blockId);
                    }
                }
            }else{
                // Unrecognized tag
                this->debug("Unrecognized tag");
            }

            // Recursivity
            *peer->buffer = peer->buffer->right(peer->buffer->size() - (*peer->sizeBuffer - sizeof(qint32)));
            *peer->sizeBuffer = 0;
            this->processTCPBuffer(socket);
        }
    }
}

void MainWindow::disconnectedTCP(){
    this->debug("disconnectedTCP");

    QTcpSocket* socket = static_cast<QTcpSocket*>(sender());
    this->hosts.remove(this->socketToMAC[socket]);
    this->updateHostsGui();
}

void MainWindow::createNewHost(QString IP, QString MAC, QTcpSocket* socket){
    Host* host = new Host(MAC, IP, socket);
    this->hosts[MAC] = host;
    this->socketToMAC[socket] = MAC;

    this->updateHostsGui();

    this->debug(QString::fromStdString("Created new Host ") + MAC + QString::fromStdString(" ") + IP);
}

void MainWindow::updateHostsGui(){
    this->ui->hostsListWidget->clear();
    for (auto e: this->hosts.keys()){
        this->ui->hostsListWidget->addItem(e);
    }
}

void MainWindow::updateDatablocksGui(){
    this->ui->blocksListWidget->clear();
    this->ui->mainListWidget->clear();

    for (auto e: this->dataRegister.keys()){
        // Blocks View
        this->ui->blocksListWidget->addItem(e);

        // Main view
        this->ui->mainListWidget->addItem(this->dataRegister[e]->toString());
    }
}

void MainWindow::sendDataBlock(QTcpSocket* socket, DataBlock* db){
    // Build json object
    QJsonObject jsonData;
    jsonData["tag"] = "[BLOCK]";
    jsonData["id"] = db->id;
    jsonData["author"] = db->author;
    jsonData["content"] = db->content;
    jsonData["date"] = QString::number(db->date->toMSecsSinceEpoch());

    // Format it
    QJsonDocument jsonDoc(jsonData);
    QByteArray datagram = jsonDoc.toJson();
    this->TCPsend(datagram, socket);
    this->debug(QString::fromStdString("Sent block ") + db->id + QString::fromStdString(" to ") + this->socketToMAC[socket] + QString::fromStdString(" msSinceEpoch: ") + QString::number(db->date->toMSecsSinceEpoch()));
}

void MainWindow::sendHas(QTcpSocket* socket){
    // Build json object
    QJsonObject jsonData;
    jsonData["tag"] = "[HAS]";

    QJsonArray blocksArray;
    for (auto e: this->dataRegister.keys()){
        blocksArray.append(e);
    }

    jsonData["blocks"] = blocksArray;

    // Format it
    QJsonDocument jsonDoc(jsonData);
    QByteArray datagram = jsonDoc.toJson();
    this->TCPsend(datagram, socket);
    this->debug(QString::fromStdString("Sent block [HAS] to ") + this->socketToMAC[socket]);
}

void MainWindow::sendAsk(QTcpSocket* socket, QString blockId){
    // Build json object
    QJsonObject jsonData;
    jsonData["tag"] = "[ASK]";
    jsonData["id"] = blockId;

    // Format it
    QJsonDocument jsonDoc(jsonData);
    QByteArray datagram = jsonDoc.toJson();
    this->TCPsend(datagram, socket);
    this->debug(QString::fromStdString("Asked block ") + blockId + QString::fromStdString(" to ") + this->socketToMAC[socket]);
}

void MainWindow::sendButtonClicked(){
    QString content = this->ui->sendEdit->text();

    QDateTime* date = new QDateTime(QDateTime::currentDateTime());
    DataBlock* db = new DataBlock(localMacAddress, content, date);
    this->dataRegister[db->id] = db;
    this->updateDatablocksGui();

    for (auto e: this->hosts.keys()){
        //this->TCPsend(content, this->hosts[e]->socket);
        this->sendDataBlock(this->hosts[e]->socket, db);
    }

    this->ui->sendEdit->clear();

    this->debug("Sent text content to all peers");
}

MainWindow::~MainWindow(){
    delete ui;
}

