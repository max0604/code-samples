#include "server.h"

#include <QVBoxLayout>
#include <QGridLayout>
#include <QMessageBox>
#include <QSplitter>
#include <QPalette>
#include <QFile>
#include <iostream>
#include <string.h>

#include <QJsonDocument>
#include <QJsonValue>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonParseError>
#include <QVariantMap>

//-----------------------------------------------------------------------------
static QString json_path = "uci-config.json";

//-----------------------------------------------------------------------------
SimpleTcpServer::SimpleTcpServer(QWidget *parent) : QWidget(parent)
{
    hostLabel   = new QLabel(tr("&host:"));
    portLabel   = new QLabel(tr("&port:"));
    msgLabel    = new QLabel(tr("&msg"));
    statusLabel = new QLabel(tr("status:"));

    hostCombo = new QComboBox();

    portLineEdit = new QLineEdit(QString("9752"));
    msgLineEdit  = new QLineEdit;

    listenButton  = new QPushButton(tr("&Listen"));
    sendMsgButton = new QPushButton(tr("&Send"));
    closeButton   = new QPushButton(tr("&Close"));

    buttonBox = new QDialogButtonBox;

    dumpTextEdit = new QTextEdit;
    textEdit     = new QTextEdit;

    server = new QTcpServer(this);

    test_timer = new QTimer(this);

    //test_timer->setInterval(1000);
    test_timer->setInterval(100);
    test_timer->start();

    hostCombo->setEditable(true);
    hostCombo->addItem(QString("127.0.0.1"));
    hostCombo->setFocus();
    hostCombo->setCursor(this->cursor());

    portLineEdit->setValidator(new QIntValidator(1, 65535, this));

    hostLabel->setBuddy(hostCombo);
    portLabel->setBuddy(portLineEdit);
    msgLabel->setBuddy(msgLineEdit);

    listenButton->setDefault(true);
    listenButton->setEnabled(true);

    sendMsgButton->setDefault(true);
    listenButton->setEnabled(true);

    buttonBox->addButton(listenButton,  QDialogButtonBox::ActionRole);
    buttonBox->addButton(sendMsgButton, QDialogButtonBox::ActionRole);
    buttonBox->addButton(closeButton, QDialogButtonBox::RejectRole);
    QVBoxLayout* mainLayout   = new QVBoxLayout;
    QGridLayout* bottomLayout = new QGridLayout;

    bottomLayout->addWidget(hostLabel,    0, 0);
    bottomLayout->addWidget(hostCombo,    0, 1);
    bottomLayout->addWidget(portLabel,    1, 0);
    bottomLayout->addWidget(portLineEdit, 1, 1);
    bottomLayout->addWidget(msgLabel,     2, 0);
    bottomLayout->addWidget(msgLineEdit,  2, 1);
    bottomLayout->addWidget(buttonBox,    3, 0, 1, 2);
    bottomLayout->addWidget(statusLabel,  4, 0);

    QHBoxLayout* horizontalLayout = new QHBoxLayout;
    mainLayout->addItem(horizontalLayout);

    QSplitter* sp = new QSplitter;
    sp->addWidget(dumpTextEdit);
    sp->addWidget(textEdit);

    QPalette palette;

    palette.setColor(QPalette::Background, Qt::red);
    palette.setColor(QPalette::Base, Qt::lightGray);

    sp->setPalette(palette);

    horizontalLayout->addWidget(sp);

    mainLayout->addLayout(bottomLayout);

    setLayout(mainLayout);

    setWindowTitle(tr("TcpServer"));

    /* Коннектим сигналы k слотам */
    connect(listenButton,  SIGNAL(clicked()),       this, SLOT(listen()));
    connect(closeButton,   SIGNAL(clicked()),       this, SLOT(closeConnections()));
    connect(server,        SIGNAL(newConnection()), this, SLOT(newConnection()));
    connect(sendMsgButton, SIGNAL(clicked()),       this, SLOT(sendData()));
}

//-----------------------------------------------------------------------------
void SimpleTcpServer::listen()
{
    QHostAddress addr(hostCombo->currentText());

    std::cout << addr.toString().toStdString() << std::endl;
    std::cout << portLineEdit->text().toInt() << std::endl;

    if (!server->listen(addr, portLineEdit->text().toInt())) {
        QMessageBox::critical(this, tr("Server"),
                        tr("Unable to start the server: %1.") .arg(server->errorString()));
    }
    else {
        statusLabel->setText(QString("status: ") + QString("Listen ") + server->serverAddress().toString() +
                             QString(":") +
                             QVariant(server->serverPort()).toString());
    }
}

//-----------------------------------------------------------------------------
bool parseJson(const QByteArray& arr, QJsonDocument& json_doc)
{
    json_doc = QJsonDocument::fromJson(arr);

    if (json_doc.isNull())
    {
        qDebug() << "Failed to create JSON doc.";
        return false;
    }

    if (!json_doc.isObject())
    {
        qDebug() << "JSON is not an object.";
        return false;
    }

    QJsonObject json_obj = json_doc.object();

    if (json_obj.isEmpty()) {
        qDebug() << "JSON object is empty.";
        return false;
    }

    return true;
}

//-----------------------------------------------------------------------------
void SimpleTcpServer::readData()
{
    QTcpSocket* s = (QTcpSocket*)sender();
    QByteArray arr = s->readAll();

    QString rx;

    rx.append("RX[");
    rx.append(QVariant(arr.length()).toString());
    rx.append("bytes]:\n");

    QString data;
    data.append(rx);
    data.append(arr.toHex('_'));

/*
    QFile logFile("./SERVER_TRAFFIK.log");
    logFile.open(QIODevice::ReadWrite | QIODevice::Append);
    logFile.write(data.toStdString().c_str());
    logFile.close();
*/

    dumpTextEdit->setTextColor(QColor("blue"));
    dumpTextEdit->append(data);

    textEdit->setTextColor(QColor("blue"));
    QString data2;
    data2.append(rx);
    data2.append(arr);
    textEdit->append(data2);
}

//-----------------------------------------------------------------------------
void SimpleTcpServer::sendData()
{
    if (sockets.size() <= 0)
    {
        statusLabel->setText(QString("status: ") + QString("No available connections\n"));
        return;
    }

    QFile json_file(json_path);
    json_file.open(QIODevice::ReadWrite);
    QByteArray json_data = json_file.readAll();
    json_file.close();

    QString json_str(json_data);

    for (int i = 0; i < sockets.size(); ++i) {
        //sockets.at(i)->write(msgLineEdit->text().toStdString().c_str());
        sockets.at(i)->write(json_data);
        sockets.at(i)->waitForBytesWritten();
    }

    QString tx;

    tx.append("TX[");
    //tx.append(QVariant(msgLineEdit->text().length()).toString());
    tx.append(json_str);
    tx.append("bytes]:\n");

/*
    QString data;
    data.append(tx);
    data.append(QByteArray(msgLineEdit->text().toStdString().c_str()).toHex());
    dumpTextEdit->setTextColor(QColor("green"));
    dumpTextEdit->append(data);
*/
    QString data2;
    data2.append(tx);
    data2.append(msgLineEdit->text());

    textEdit->setTextColor(QColor("green"));
    textEdit->append(data2);
}

//-----------------------------------------------------------------------------
void SimpleTcpServer::closeConnections()
{
    for (int i = 0; i < sockets.size(); ++i)
    {
        if (sockets.at(i)->isValid() && sockets.at(i)->isOpen())
        {
            statusLabel->setText(QString("status: ") + QString("Disconnected ") +
                                 sockets.at(i)->peerAddress().toString() +
                                                QString(":") +
                                 QVariant(sockets.at(i)->peerPort()).toString());
            sockets.at(i)->disconnectFromHost();
            sockets.at(i)->close();
        }
    }

    qDebug() << "server->close";

    server->close();
    sockets.clear();
    statusLabel->setText(QString("status: ") + QString("Disconnected"));
}

//-----------------------------------------------------------------------------
SimpleTcpServer::~SimpleTcpServer()
{
    closeConnections();
    server->close();
}

//-----------------------------------------------------------------------------
void SimpleTcpServer::acceptError(QAbstractSocket::SocketError socketError)
{
    QTcpSocket* s = (QTcpSocket*)sender();

    switch (socketError) {
    case QAbstractSocket::RemoteHostClosedError:
        break;
    case QAbstractSocket::HostNotFoundError:
        QMessageBox::information(this, tr("Server"),
                                       tr("The host was not found. Please check the "
                                       "host name and port settings."));
        break;
    case QAbstractSocket::ConnectionRefusedError:
        QMessageBox::information(this, tr("Server"),
                                       tr("The connection was refused by the peer. "
                                       "Make sure the server is running, "
                                       "and check that the host name and port "
                                       "settings are correct."));
        break;
    default:
        QMessageBox::information(this, tr("Server"),
                                       tr("The following error occurred: %1.")
                                       .arg(s->errorString()));
    }
}

//-----------------------------------------------------------------------------
void SimpleTcpServer::clientDisconnnected()
{
    // QTcpSocket *clientSocket = qobject_cast<QTcpSocket *>(QObject::sender());
    QTcpSocket* s = (QTcpSocket*)sender();

    statusLabel->setText(QString("status: ") + QString("Disconnnected ") +
                         s->peerAddress().toString() + QString(":") +
                         QVariant(s->peerPort()).toString());
    s->close();
}

//-----------------------------------------------------------------------------
void SimpleTcpServer::newConnection()
{
    QTcpSocket* socket = server->nextPendingConnection();
    statusLabel->setText(QString("status: ") + QString("Connected ") +
                         socket->peerAddress().toString() + QString(":") +
                         QVariant(socket->peerPort()).toString());
    connect(socket, SIGNAL(readyRead()),    this, SLOT(readData()));
    connect(socket, SIGNAL(disconnected()), this, SLOT(clientDisconnnected()));
    sockets.append(socket);
}
