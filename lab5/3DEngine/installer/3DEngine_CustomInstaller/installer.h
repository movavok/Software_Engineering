#ifndef INSTALLER_H
#define INSTALLER_H

#include <QObject>
#include <QtGlobal>
#include <QCoreApplication>
#include <QDate>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QSettings>
#include <QStandardPaths>
#include <QStringList>
#include <QTcpSocket>
#include <QTextStream>

#include "installdata.h"

class Installer : public QObject
{
    Q_OBJECT
public:
    explicit Installer(const InstallData& data, QObject* parent = nullptr) : QObject(parent), m_data(data) {}

    void run();

signals:
    void progressChanged(int value);
    void statusChanged(const QString& status);
    void finished(bool ok, const QString& errorMessage);

private:
    static constexpr const char* kRegistryBase = "HKEY_LOCAL_MACHINE\\SOFTWARE\\KNTU\\3DEngine";

    struct SmtpConfig {
        QString iniPath;
        QStringList searchedIniPaths;
        QString server;
        QString port;
        QString user;
        QString pass;
        QString from;
        bool useSsl = true;
        bool isGmailSender = false;
        bool haveUser = false;
        bool havePass = false;
    };

    InstallData m_data;
    int m_progress = 0;
    QString m_status;
    QString m_lastError;
    QString m_installedExePath;

    void setProgress(int value);
    void setStatus(const QString& status);
    void setError(const QString& message);

    static QString psQuote(const QString& value);
    bool runPowerShellCommand(const QString& command, int timeoutMs);
    QString resolveInstalledExePath() const;

    bool startInstallation();
    bool extractArchive();
    bool createDesktopShortcut();
    bool createStartMenuShortcut();
    bool createShortcut(const QString& shortcutPath, int progressAfter);
    bool createHidekeyFile();
    bool createRegistry();
    bool hasInternetConnection(int timeoutMs);
    bool canConnectToHost(const QString& host, quint16 port, int timeoutMs);
    bool sendInstallEmailIfPossible();

    QStringList smtpCandidateIniPaths(const QString& appDir) const;
    QString chooseSmtpIniPath(const QString& appDir, const QStringList& candidates) const;
    SmtpConfig loadSmtpConfig(const QString& appDir) const;
    QStringList smtpMissingEnvVars(const SmtpConfig& cfg) const;
    QString smtpSkippedStatusMessage(const SmtpConfig& cfg, const QStringList& missingVars) const;
    quint16 smtpPortOrDefault(const QString& portText) const;
    QString buildInstallEmailBody() const;
    QString buildSendMailMessageCommand(const SmtpConfig& cfg, quint16 port16, const QString& subject, const QString& body) const;
    bool trySendEmailWithPowerShell(const SmtpConfig& cfg, quint16 port16, const QString& subject, const QString& body);
};

#endif
