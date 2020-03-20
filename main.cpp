#include "mainwindow.h"
#include "host.h"
#include "datablock.h"
#include <QApplication>
#include <string>

int main(int argc, char *argv[]){
    QApplication a(argc, argv);
    MainWindow window;
    window.show();
    return a.exec();
}
