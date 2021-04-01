#include <QApplication>
#include "widget.h"

int main(int argc, char *argv[])
{
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication app(argc, argv);
    tcpClient client;
    client.show();
    return app.exec();
}
