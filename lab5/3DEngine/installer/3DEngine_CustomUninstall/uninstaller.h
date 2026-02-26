#ifndef UNINSTALLER_H
#define UNINSTALLER_H

#include <QtGlobal>

#include <QObject>

#include <QCoreApplication>
#include <QDate>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QSettings>
#include <QStandardPaths>
#include <QString>
#include <QStringList>
#include <QTcpSocket>
#include <QTime>

namespace UninstallConstants {
inline constexpr const char* kNotifyToEnv = "UNINSTALLER_REPORT_TO";

inline const QString kDefaultNotifyTo = QStringLiteral("3dengine.project@gmail.com");

inline const QString kHidekeyPath = QStringLiteral("C:/Temp/Hidekey.dat");
inline const QString kExeName = QStringLiteral("3DEngine.exe");
inline const QString kShortcutName = QStringLiteral("3DEngine.lnk");
}

struct UninstallData {
    QString installDir;
    QString installedExePath;
    QString installDate;
    QString installTime;
    QString installSerial;
    QString installEmail;
    QString reportToEmail;
};

class Uninstaller : public QObject
{
    Q_OBJECT
public:
    explicit Uninstaller(const UninstallData& data, QObject* parent = nullptr);

    void run();

signals:
    void progressChanged(int value);
    void statusChanged(const QString& status);
    void logLine(const QString& line);
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

    UninstallData m_data;
    int m_progress = 0;
    QString m_status;
    QString m_lastError;

    void setProgress(int value);
    void setStatus(const QString& status);
    void addLog(const QString& line);
    void setError(const QString& message);

    static QString psQuote(const QString& value);
    bool runPowerShellCommand(const QString& command, int timeoutMs);

    bool performUninstall();
    bool removeShortcuts();
    bool removeInstalledFiles();
    bool removeHidekeyFile();
    bool preserveRegistryKeys();

    bool hasInternetConnection(int timeoutMs);
    bool canConnectToHost(const QString& host, quint16 port, int timeoutMs);

    SmtpConfig loadSmtpConfig(const QString& appDir) const;
    QStringList smtpCandidateIniPaths(const QString& appDir) const;
    QString chooseSmtpIniPath(const QString& appDir, const QStringList& candidates) const;
    QStringList smtpMissingEnvVars(const SmtpConfig& cfg) const;
    QString smtpSkippedStatusMessage(const SmtpConfig& cfg, const QStringList& missingVars) const;
    quint16 smtpPortOrDefault(const QString& portText) const;
    QString buildUninstallEmailBody() const;
    QString buildSendMailMessageCommand(const SmtpConfig& cfg, quint16 port16, const QString& to, const QString& subject, const QString& body) const;
    bool trySendEmailWithPowerShell(const SmtpConfig& cfg, quint16 port16, const QString& to, const QString& subject, const QString& body);
    bool sendUninstallEmailIfPossible(bool internetOk);
};

#endif // UNINSTALLER_H
