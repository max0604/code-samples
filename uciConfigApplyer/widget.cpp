#include "widget.h"
#include <QGridLayout>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QByteArray>
#include <QPlainTextDocumentLayout>
#include <QVariant>
#include <QColor>
#include <QApplication>
#include <QScreen>
#include <iostream>
#include <QFile>

#include <QJsonDocument>
#include <QJsonValue>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonParseError>
#include <QVariantMap>
#include <uci.h>
#include <string.h>

//--- forward declarations ----------------------------------------------------
static int write_cfg_value(const char* group, const char* section,
                       const char* option, const char* value);

//-----------------------------------------------------------------------------
tcpClient::tcpClient(QWidget *parent) : QWidget(parent)
{
    QRect r = QApplication::screens().at(0)->geometry();
    ///qDebug() << "width = " << r.width() << " height = " << r.height();
    this->resize(r.width()/2, r.height()/2);

    hostLabel   = new QLabel(tr("&host:"));
    portLabel   = new QLabel(tr("&port:"));
    statusLabel = new QLabel(tr("status:"));

    hostCombo = new QComboBox;
    portLineEdit = new QLineEdit;
    connectButton    = new QPushButton(tr("&Connect"));
    disconnectButton = new QPushButton(tr("&Disconnect"));
    buttonBox = new QDialogButtonBox;
    dumpTextEdit = new QTextEdit;
    splitter = new QSplitter(this);
    socket = new QTcpSocket(this);

    /* Настраиваем */

    hostCombo->setEditable(true);

    portLineEdit->setValidator(new QIntValidator(1, 65535, this));
    portLineEdit->setFocus();

    hostLabel->setBuddy(hostCombo);
    portLabel->setBuddy(portLineEdit);

    connectButton->setDefault(true);
    connectButton->setEnabled(true);

    buttonBox->addButton(connectButton,    QDialogButtonBox::ActionRole);
    buttonBox->addButton(disconnectButton, QDialogButtonBox::RejectRole);

    QVBoxLayout* mainLayout   = new QVBoxLayout;
    QHBoxLayout* upperLayout  = new QHBoxLayout;
    QGridLayout* bottomLayout = new QGridLayout;

    bottomLayout->addWidget(hostLabel,    0, 0);
    bottomLayout->addWidget(hostCombo,    0, 1);
    bottomLayout->addWidget(portLabel,    1, 0);
    bottomLayout->addWidget(portLineEdit, 1, 1);
    bottomLayout->addWidget(buttonBox,    3, 0, 1, 2);
    bottomLayout->addWidget(statusLabel,  4, 0);

#if 1
    /// отладка
    hostCombo->addItem("127.0.0.1");
    portLineEdit->setText("9752");
#endif

    splitter->addWidget(dumpTextEdit);
    upperLayout->addWidget(splitter);

    mainLayout->addLayout(upperLayout);
    mainLayout->addLayout(bottomLayout);

    setLayout(mainLayout);

    setWindowTitle(tr("UciConfigInterpretator"));

    connect(socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(displayError(QAbstractSocket::SocketError)));
    connect(portLineEdit, SIGNAL(textChanged(QString)), this, SLOT(activateConnectButton()));
    connect(connectButton, SIGNAL(clicked()), this, SLOT(connectToServer()));
    connect(disconnectButton, SIGNAL(clicked()), this, SLOT(disconnectFromServer()));
    connect(socket, SIGNAL(connected()), this, SLOT(showConnectedStatus()));
    connect(socket, SIGNAL(disconnected()), this, SLOT(showDisconnectedStatus()));
    connect(socket, SIGNAL(readyRead()), this, SLOT(readData()));
}

//-----------------------------------------------------------------------------
tcpClient::~tcpClient()
{
    if (socket->isOpen()) {
        socket->close();
    }
}

//-----------------------------------------------------------------------------
void tcpClient::displayError(QAbstractSocket::SocketError socketError)
{
    switch (socketError) {
    case QAbstractSocket::RemoteHostClosedError:
        break;
    case QAbstractSocket::HostNotFoundError:
        QMessageBox::information(this, tr("Client"),
                                       tr("The host was not found. Please check the "
                                       "host name and port settings."));
        break;
    case QAbstractSocket::ConnectionRefusedError:
        QMessageBox::information(this, tr("Client"),
                                       tr("The connection was refused by the peer. "
                                       "Make sure the server is running, "
                                       "and check that the host name and port "
                                       "settings are correct."));
        break;
    default:
        QMessageBox::information(this, tr("Client"),
                                       tr("The following error occurred: %1.")
                                       .arg(socket->errorString()));
    }
}

//-----------------------------------------------------------------------------
void tcpClient::activateConnectButton()
{
    connectButton->setEnabled(!hostCombo->currentText().isEmpty() && !portLineEdit->text().isEmpty());
}

//-----------------------------------------------------------------------------
void tcpClient::connectToServer()
{
    socket->abort();
    socket->connectToHost(hostCombo->currentText(), portLineEdit->text().toInt());
}

//-----------------------------------------------------------------------------
void tcpClient::disconnectFromServer()
{
    socket->disconnectFromHost();
}

//-----------------------------------------------------------------------------
void tcpClient::showConnectedStatus()
{
    statusLabel->setText("status: Connected");
}

//-----------------------------------------------------------------------------
void tcpClient::showDisconnectedStatus()
{
    statusLabel->setText("status: Disconnected");
}

//----------------------------------------------------------------------------
void tcpClient::readData()
{
    QByteArray arr = socket->readAll();

    QJsonParseError jsonError;
    QJsonDocument doc = QJsonDocument::fromJson(arr, &jsonError);
    if (jsonError.error != QJsonParseError::NoError)
    {
        qDebug() << jsonError.errorString();
    }

    QString rx;
    rx.append("RX[");
    rx.append(QVariant(arr.length()).toString());
    rx.append("bytes]:\n");
    QString data;
    data.append(rx);
    //data.append(arr.toHex('_'));
    data.append(QString(arr));
    dumpTextEdit->setTextColor(QColor("red"));
    dumpTextEdit->append(data);

    static QString cmd;
    static QJsonObject interfaceConfig;

    QJsonObject jObject = doc.object();

    for (auto it = jObject.begin(); it != jObject.end(); ++it)
    {
        qDebug() << it.key() << " " << it.value();

        if (it.key() == "command")
        {
            cmd = it.value().toString();
        }

        if (it.key() == "network")
        {
            if (it.value().isArray())
            {
                QJsonArray arr = it.value().toArray();
                for (int i = 0; i < arr.size(); ++i)
                {
                    QJsonObject obj = arr.at(i).toObject();

                    for (auto it1 = obj.begin(); it1 != obj.end(); ++it1)
                    {
                        qDebug() << it1.key() << " " << it1.value();

                        if (it1.value().isObject())
                        {
                            QJsonObject obj1 = it1.value().toObject();
                            interfaceConfig = obj1;
                            for(auto it2 = obj1.begin(); it2 != obj1.end(); ++it2)
                            {
                                qDebug() << it2.key() << " " << it2.value();
                            }
                        }
                    }
                }
            }
        }
    }

    if (cmd == "status")
    {
        statusLabel->setText("Receive STATUS command");
        auto data = QJsonObject(
        {
            qMakePair(QString("result"), QString("OK")),
        });
        QJsonDocument doc;
        doc.setObject(data);
        socket->write(doc.toJson(QJsonDocument::Indented));
    }
    else if (cmd == "reboot")
    {
        statusLabel->setText("Receive REBOOT command");
    }
    else if (cmd == "shutdown")
    {
        statusLabel->setText("Receive SHUTDOWN command");
    }
    else if (cmd == "read_config")
    {
        statusLabel->setText("Receive READ_CONFIG command");
    }
    else if (cmd == "write_config")
    {
        statusLabel->setText("Receive WRITE_CONFIG command");

        QJsonObject data;

        for (auto it = interfaceConfig.begin();
                  it != interfaceConfig.end(); ++it)
        {
            if (write_cfg_value("network", "interface", it.key().toStdString().c_str(),
                        it.value().toString().toStdString().c_str()) == -1)
            {
                printf("write_cfg_value fail\n");
                data = QJsonObject( { qMakePair(QString("result"), QString("NOT OK")),
                                qMakePair(QString("description"), QString("write cfg fail"))});
            }
            else
            {
                data = QJsonObject( { qMakePair(QString("result"), QString("OK")) });
            }
        }

        QJsonDocument doc;
        doc.setObject(data);
        socket->write(doc.toJson(QJsonDocument::Indented));
    }
    else
    {
        statusLabel->setText("Error: uncknown command");
    }
}

//-----------------------------------------------------------------------------
static int write_cfg_value(const char* group, const char* section,
                                    const char* option, const char* value)
{
    QString path = QString(group) + QString(".@") +
                    QString(section) + QString("[0].") + QString(option);

    char buffer[80];
    struct  uci_ptr ptr;
    struct  uci_context *c = uci_alloc_context();

    if(!c) return -1;

    if ((uci_lookup_ptr(c, &ptr,
                       (char*)path.toStdString().c_str(), true) != UCI_OK) ||
                       (ptr.o==NULL || ptr.o->v.string==NULL))
    {
      uci_free_context(c);
      return -1;
    }

    if(ptr.flags & uci_ptr::UCI_LOOKUP_COMPLETE)
    {
        strcpy(buffer, ptr.o->v.string);
    }
    //printf("%s\n", buffer);

    ptr.value = value;
    if (uci_set(c, &ptr) != UCI_OK)
    {
        uci_free_context(c);
        printf("uci_set error\n");
        return -1;
    }

    if (uci_commit(c, &ptr.p, false) != UCI_OK)
    {
       uci_free_context(c);
       uci_perror(c, "UCI Error");
       printf("uci commit error\n");
       return -1;
    }

    uci_free_context(c);
    return 0;
}
