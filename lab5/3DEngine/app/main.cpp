#include "mainwindow.h"

#include <QApplication>
#include <QStringList>
#include <QFileInfo>
#include <QIcon>

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    a.setWindowIcon(QIcon(":/icons/app_icon.png"));
    MainWindow w;
    QStringList args = a.arguments();
    if(args.size() > 1){ QString sceneFile = args.at(1); if(QFileInfo::exists(sceneFile)) w.loadSceneFile(sceneFile); }
    w.show();
    return a.exec();
}
