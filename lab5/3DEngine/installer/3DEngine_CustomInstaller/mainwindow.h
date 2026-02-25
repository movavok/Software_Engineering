#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QRegularExpressionValidator>
#include <QMessageBox>
#include <QFileDialog>
#include <QThread>
#include <QTimer>

#include "installer.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onPageBack();
    void onPageNext();

    void validateSerialPageInputs();
    void validateInstallPath(const QString& path);
    void chooseInstallDirectory();

private:
    Ui::MainWindow *ui;

    QThread* m_installThread = nullptr;
    Installer* m_installer = nullptr;
    InstallData m_currentInstallData;
    QString m_lastEmailStatus;

    void checkIfAppExist();
    void updateWindowByPage(int index);

    void getLicenseFromFile(const QString& path);

    bool checkSerial();

    bool startInstallation();
    bool collectInstallData(InstallData& data);
    void startInstallationAsync(const InstallData& data);
    void fillFinishPage(const InstallData& data);
};
#endif
