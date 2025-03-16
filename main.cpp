#include "mainwindow.h"
#include <QApplication>
#include <QDebug>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    // Connect aboutToQuit to cleanup
    QObject::connect(&a, &QCoreApplication::aboutToQuit, [&w]() {
        qDebug() << "Main: Application is about to quit, cleaning up...";
        // No explicit cleanup needed here if destructors are correct
    });

    return a.exec();
}
