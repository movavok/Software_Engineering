#ifndef INSTALLDATA_H
#define INSTALLDATA_H

#include <QString>

struct InstallData {
    QString installPath;
    QString exeFileName;
    bool desktopShortcut = false;
    bool startMenuShortcut = false;
    bool runAfterInstall = false;
    QString serialNumber;
    QString email;
};

#endif
