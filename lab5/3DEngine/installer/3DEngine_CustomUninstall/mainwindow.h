#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QCoreApplication>
#include <QDate>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMessageBox>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QSettings>
#include <QTextStream>
#include <QTime>
#include <QThread>

#include "uninstaller.h"

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
    void validateWelcomeInputs();

private:
    Ui::MainWindow *ui;

    bool m_componentsDetected = false;
    UninstallData m_data;

    QThread* m_uninstallThread = nullptr;
    Uninstaller* m_uninstaller = nullptr;

    void updateWindowByPage(int index);
    void detectInstalledComponents();
    void parseHidekeyIfPresent();
    void readRegistryInfo();
    void detectInstallDirFromUninstallRegistry();
    void detectInstallDirHeuristics();
    bool startUninstall();
    void fillFinishPage();
};
#endif // MAINWINDOW_H
