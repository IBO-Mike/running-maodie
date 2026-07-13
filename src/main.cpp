#include "app/mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QApplication::setOrganizationName(QStringLiteral("CSU"));
    QApplication::setApplicationName(QStringLiteral("RunningMaodie"));

    MainWindow w;
    w.show();
    return QApplication::exec();
}
