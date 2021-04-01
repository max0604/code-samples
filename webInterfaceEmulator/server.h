#ifndef SERVER_H
#define SERVER_H

#include <QDialog>
#include <QLabel>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QTcpServer>
#include <QTcpSocket>
#include <QList>
#include <QTimer>

#include <QTextEdit>

class SimpleTcpServer : public QWidget
{
    Q_OBJECT

    QLabel*           hostLabel;
    QLabel*           portLabel;
    QLabel*           msgLabel;
    QLabel*           statusLabel;
    QComboBox*        hostCombo;
    QLineEdit*        portLineEdit;
    QLineEdit*        msgLineEdit;
    QPushButton*      listenButton;
    QPushButton*      sendMsgButton;
    QPushButton*      closeButton;
    QDialogButtonBox* buttonBox;
    QTextEdit*        dumpTextEdit;
    QTextEdit*        textEdit;    

    QTcpServer* server;

    // таймер для проработкi ситуации с посылкой байт раз в 100 миллисекунд
    QTimer* test_timer;

    QList<QTcpSocket*> sockets;

private slots:

    void listen();
    void readData();
    void sendData();
    void newConnection();
    void closeConnections();
    void acceptError(QAbstractSocket::SocketError err);
    void clientDisconnnected();

signals:
    void emitReadSignal(QTcpSocket* s);

public:
    SimpleTcpServer(QWidget *parent = 0);
    ~SimpleTcpServer();
};

#endif // SERVER_H
