#ifndef DIALOG_H
#define DIALOG_H

#include <QWidget>
#include <QDialog>
#include <QTcpSocket>
#include <QLabel>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QSplitter>

#include <QTextEdit>
#include <QAbstractSocket>

class tcpClient : public QWidget
{
    Q_OBJECT

public:
    tcpClient(QWidget *parent = 0);
    ~tcpClient();

private:

    QLabel*           hostLabel;
    QLabel*           portLabel;
    QLabel*           statusLabel;
    QLabel*           image;
    QComboBox*        hostCombo;
    QLineEdit*        portLineEdit;
    QPushButton*      connectButton;
    QPushButton*      disconnectButton;
    QDialogButtonBox* buttonBox;
    QTextEdit*        dumpTextEdit;
    QSplitter*        splitter;

    QTcpSocket* socket;

private slots:
    void displayError(QAbstractSocket::SocketError socketError);
    void activateConnectButton();
    void connectToServer();
    void disconnectFromServer();
    void showConnectedStatus();
    void showDisconnectedStatus();
    void readData();

signals:
    void setCommand(uint8_t cmd);
};

enum {
    STATUS = 0,
    REBOOT = 1,
    SHUTDOWN = 2,
    READ_CONFIG = 3,
    WRITE_CONFIG = 4
};


#endif // DIALOG_H
