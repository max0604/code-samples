#include "server.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    SimpleTcpServer w;
    w.show();

    return a.exec();
}
