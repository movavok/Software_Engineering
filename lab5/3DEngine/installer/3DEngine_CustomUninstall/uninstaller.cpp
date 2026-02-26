#include "uninstaller.h"

Uninstaller::Uninstaller(const UninstallData& data, QObject* parent)
    : QObject(parent)
    , m_data(data) {}

void Uninstaller::run() {
    const bool ok = performUninstall();
    emit finished(ok, ok ? QString() : m_lastError);
}

void Uninstaller::setProgress(int value) {
    const int clamped = qBound(0, value, 100);
    if (clamped == m_progress)
        return;
    m_progress = clamped;
    emit progressChanged(m_progress);
}

void Uninstaller::setStatus(const QString& status) {
    if (status == m_status)
        return;
    m_status = status;
    emit statusChanged(m_status);
}

void Uninstaller::addLog(const QString& line) {
    emit logLine(line);
}

void Uninstaller::setError(const QString& message) {
    m_lastError = message;
}

QString Uninstaller::psQuote(const QString& value) {
    QString escaped = value;
    escaped.replace("'", "''");
    return "'" + escaped + "'";
}

bool Uninstaller::runPowerShellCommand(const QString& command, int timeoutMs) {
    QProcess process;
    process.setProcessChannelMode(QProcess::MergedChannels);

    QStringList args;
    args << "-NoProfile" << "-NonInteractive" << "-ExecutionPolicy" << "Bypass" << "-Command" << command;
    process.start("powershell.exe", args);

    if (!process.waitForStarted(10'000)) {
        setError("Failed to start PowerShell.");
        return false;
    }

    if (!process.waitForFinished(timeoutMs)) {
        process.kill();
        process.waitForFinished(3'000);
        setError("PowerShell command timed out.");
        return false;
    }

    const int exitCode = process.exitCode();
    if (process.exitStatus() != QProcess::NormalExit || exitCode != 0) {
        const QString output = QString::fromLocal8Bit(process.readAll());
        setError(QString("PowerShell failed (exit %1): %2").arg(exitCode).arg(output.trimmed()));
        return false;
    }

    return true;
}

bool Uninstaller::performUninstall() {
    setProgress(0);
    setStatus(QStringLiteral("Preparing..."));
    addLog(QStringLiteral("Preparing..."));

    setStatus(QStringLiteral("Removing shortcuts..."));
    addLog(QStringLiteral("Step 1/6: Removing shortcuts"));
    removeShortcuts();
    setProgress(15);

    setStatus(QStringLiteral("Removing installed files..."));
    addLog(QStringLiteral("Step 2/6: Removing installed files"));
    if (!removeInstalledFiles()) {
        addLog(QStringLiteral("Installed files removal skipped/failed: ") + (m_lastError.isEmpty() ? QStringLiteral("unknown") : m_lastError));
    }
    setProgress(55);

    setStatus(QStringLiteral("Deleting C:/Temp/Hidekey.dat..."));
    addLog(QStringLiteral("Step 3/6: Deleting C:/Temp/Hidekey.dat"));
    if (!removeHidekeyFile()) {
        addLog(QStringLiteral("Hidekey.dat deletion skipped/failed: ") + (m_lastError.isEmpty() ? QStringLiteral("unknown") : m_lastError));
    }
    setProgress(70);

    setStatus(QStringLiteral("Preserving registry keys..."));
    addLog(QStringLiteral("Step 4/6: Preserving registry keys (no changes)"));
    preserveRegistryKeys();
    setProgress(75);

    setStatus(QStringLiteral("Checking internet connection..."));
    addLog(QStringLiteral("Step 5/6: Checking internet connection"));
    const bool internetOk = hasInternetConnection(3000);
    addLog(internetOk ? QStringLiteral("Internet connection detected") : QStringLiteral("No internet connection detected"));
    setProgress(80);

    setStatus(QStringLiteral("Sending uninstall report (if possible)..."));
    addLog(QStringLiteral("Step 6/6: Sending email report (silent)"));
    sendUninstallEmailIfPossible(internetOk);
    setProgress(100);

    setStatus(QStringLiteral("Completed"));
    addLog(QStringLiteral("Completed"));
    return true;
}

bool Uninstaller::removeShortcuts() {
    bool anyRemoved = false;

    const auto removeIfExists = [&](const QString& path, const QString& okMsg, const QString& failMsg) {
        if (!QFile::exists(path))
            return;
        if (QFile::remove(path)) {
            addLog(okMsg);
            anyRemoved = true;
        } else {
            addLog(failMsg);
        }
    };

    const QString desktopPath = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
    removeIfExists(
        QDir(desktopPath).filePath(UninstallConstants::kShortcutName),
        QStringLiteral("Removed desktop shortcut"),
        QStringLiteral("Failed to remove desktop shortcut"));

    const QString startMenuPath = QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation);
    if (!startMenuPath.isEmpty()) {
        removeIfExists(
            QDir(startMenuPath).filePath(UninstallConstants::kShortcutName),
            QStringLiteral("Removed Start Menu shortcut"),
            QStringLiteral("Failed to remove Start Menu shortcut"));
    }

    return anyRemoved;
}

bool Uninstaller::removeInstalledFiles() {
    m_lastError.clear();

    QString dir = m_data.installDir.trimmed();
    if (dir.isEmpty() && !m_data.installedExePath.trimmed().isEmpty())
        dir = QFileInfo(m_data.installedExePath).absolutePath();

    if (dir.isEmpty()) {
        addLog(QStringLiteral("Install directory not detected"));
        return false;
    }

    QFileInfo dirInfo(dir);
    if (!dirInfo.exists() || !dirInfo.isDir()) {
        setError("Install directory does not exist: " + dir);
        return false;
    }

    const QString appDir = QCoreApplication::applicationDirPath();
    if (QDir::cleanPath(dir).compare(QDir::cleanPath(appDir), Qt::CaseInsensitive) == 0) {
        addLog(QStringLiteral("Install dir equals uninstaller dir; skipping recursive folder delete"));
        return true;
    }

    const QString exe1 = QDir(dir).filePath(UninstallConstants::kExeName);
    const QString exe2 = QDir(dir).filePath(QStringLiteral("3DEngine/") + UninstallConstants::kExeName);
    if (!QFile::exists(exe1) && !QFile::exists(exe2)) {
        setError("Safety check failed: 3DEngine.exe not found inside " + dir);
        return false;
    }

    addLog(QStringLiteral("Removing directory: ") + QDir::toNativeSeparators(dir));
    const QString command =
        QStringLiteral("Remove-Item -LiteralPath ") + psQuote(QDir::toNativeSeparators(dir)) + QStringLiteral(" -Recurse -Force -ErrorAction Stop");

    if (!runPowerShellCommand(command, 2 * 60 * 1000))
        return false;

    addLog(QStringLiteral("Directory removed"));
    return true;
}

bool Uninstaller::removeHidekeyFile() {
    m_lastError.clear();
    if (!QFile::exists(UninstallConstants::kHidekeyPath)) {
        addLog(QStringLiteral("Hidekey.dat not found"));
        return true;
    }
    if (!QFile::remove(UninstallConstants::kHidekeyPath)) {
        setError(QStringLiteral("Failed to delete ") + UninstallConstants::kHidekeyPath);
        return false;
    }
    addLog(QStringLiteral("Hidekey.dat removed"));
    return true;
}

bool Uninstaller::preserveRegistryKeys() {
    // Requirement: keep registry keys. Nothing to do.
    QSettings settings(kRegistryBase, QSettings::NativeFormat);
    Q_UNUSED(settings);
    return true;
}

bool Uninstaller::hasInternetConnection(int timeoutMs) {
    return canConnectToHost(QStringLiteral("8.8.8.8"), 53, timeoutMs);
}

bool Uninstaller::canConnectToHost(const QString& host, quint16 port, int timeoutMs) {
    if (host.trimmed().isEmpty() || port == 0)
        return false;

    QTcpSocket socket;
    socket.connectToHost(host, port);
    const bool ok = socket.waitForConnected(timeoutMs);
    if (ok)
        socket.disconnectFromHost();
    return ok;
}

QStringList Uninstaller::smtpCandidateIniPaths(const QString& appDir) const {
    return {
        QDir(appDir).filePath(QStringLiteral("smtp.ini")),
        QDir(QDir::currentPath()).filePath(QStringLiteral("smtp.ini")),
        QDir(appDir).absoluteFilePath(QStringLiteral("../smtp.ini")),
    };
}

QString Uninstaller::chooseSmtpIniPath(const QString& appDir, const QStringList& candidates) const {
    for (const QString& path : candidates) {
        if (QFile::exists(path))
            return path;
    }
    return QDir(appDir).filePath(QStringLiteral("smtp.ini"));
}

Uninstaller::SmtpConfig Uninstaller::loadSmtpConfig(const QString& appDir) const {
    const QStringList candidates = smtpCandidateIniPaths(appDir);
    const QString iniPath = chooseSmtpIniPath(appDir, candidates);

    SmtpConfig cfg;
    cfg.iniPath = iniPath;
    cfg.searchedIniPaths = candidates;

    QSettings ini(iniPath, QSettings::IniFormat);
    ini.beginGroup(QStringLiteral("smtp"));

    const auto readValue = [&](const char* envName, const char* iniKey) -> QString {
        const QString envVal = qEnvironmentVariable(envName);
        if (!envVal.trimmed().isEmpty())
            return envVal;
        return ini.value(QString::fromLatin1(iniKey)).toString();
    };

    // Reuse same env vars as the installer to keep config simple.
    cfg.server = readValue("INSTALLER_SMTP_SERVER", "server");
    cfg.port = readValue("INSTALLER_SMTP_PORT", "port");
    cfg.user = readValue("INSTALLER_SMTP_USER", "user");
    cfg.pass = readValue("INSTALLER_SMTP_PASS", "pass");
    cfg.from = readValue("INSTALLER_SMTP_FROM", "from");
    if (cfg.from.trimmed().isEmpty())
        cfg.from = cfg.user;

    const QString sslRaw = readValue("INSTALLER_SMTP_USESSL", "useSsl");
    cfg.useSsl = sslRaw.trimmed().isEmpty() ? true : (sslRaw.trimmed() != QStringLiteral("0"));

    ini.endGroup();

    const QString userLower = cfg.user.trimmed().toLower();
    cfg.isGmailSender = userLower.endsWith(QStringLiteral("@gmail.com")) || userLower.endsWith(QStringLiteral("@googlemail.com"));
    if (cfg.server.trimmed().isEmpty() && cfg.isGmailSender)
        cfg.server = QStringLiteral("smtp.gmail.com");
    if (cfg.port.trimmed().isEmpty() && cfg.isGmailSender)
        cfg.port = QStringLiteral("587");

    cfg.haveUser = !cfg.user.trimmed().isEmpty();
    cfg.havePass = !cfg.pass.trimmed().isEmpty();
    if (cfg.from.trimmed().isEmpty())
        cfg.from = cfg.user;

    return cfg;
}

QStringList Uninstaller::smtpMissingEnvVars(const SmtpConfig& cfg) const {
    QStringList missing;
    if (cfg.server.trimmed().isEmpty())
        missing << QStringLiteral("INSTALLER_SMTP_SERVER");
    if (cfg.from.trimmed().isEmpty())
        missing << QStringLiteral("INSTALLER_SMTP_FROM");
    if (cfg.haveUser != cfg.havePass)
        missing << (cfg.haveUser ? QStringLiteral("INSTALLER_SMTP_PASS") : QStringLiteral("INSTALLER_SMTP_USER"));
    return missing;
}

QString Uninstaller::smtpSkippedStatusMessage(const SmtpConfig& cfg, const QStringList& missingVars) const {
    QString hint;
    if (cfg.isGmailSender) {
        hint = QStringLiteral(" (Gmail: set INSTALLER_SMTP_USER + INSTALLER_SMTP_PASS (App Password) via env vars or smtp.ini; server defaults to smtp.gmail.com:587)");
    }

    const QString where = QFile::exists(cfg.iniPath)
                              ? (QStringLiteral(" (using ") + cfg.iniPath + QStringLiteral(")"))
                              : (QStringLiteral(" (smtp.ini not found; searched: ") + cfg.searchedIniPaths.join(QStringLiteral("; ")) + QStringLiteral(")"));

    return QStringLiteral("Email skipped (SMTP not configured: missing ") + missingVars.join(QStringLiteral(", ")) + QStringLiteral(")") + hint + where;
}

quint16 Uninstaller::smtpPortOrDefault(const QString& portText) const {
    const QString port = portText.trimmed().isEmpty() ? QStringLiteral("587") : portText.trimmed();
    bool ok = false;
    const uint portNum = port.toUInt(&ok);
    if (!ok || portNum > 65535)
        return static_cast<quint16>(587);
    return static_cast<quint16>(portNum);
}

QString Uninstaller::buildUninstallEmailBody() const {
    QString body;
    body += QStringLiteral("3DEngine uninstall report\n\n");
    if (!m_data.installDir.trimmed().isEmpty())
        body += QStringLiteral("Install dir: ") + m_data.installDir + QStringLiteral("\n");
    if (!m_data.installedExePath.trimmed().isEmpty())
        body += QStringLiteral("Exe: ") + m_data.installedExePath + QStringLiteral("\n");
    if (!m_data.installSerial.trimmed().isEmpty())
        body += QStringLiteral("Serial: ") + m_data.installSerial + QStringLiteral("\n");
    if (!m_data.installEmail.trimmed().isEmpty())
        body += QStringLiteral("Email (from Hidekey/registry): ") + m_data.installEmail + QStringLiteral("\n");
    body += QStringLiteral("Uninstall date: ") + QDate::currentDate().toString(QStringLiteral("dd.MM.yyyy")) + QStringLiteral("\n");
    body += QStringLiteral("Uninstall time: ") + QTime::currentTime().toString(QStringLiteral("HH:mm:ss")) + QStringLiteral("\n");
    body += QStringLiteral("Registry preserved: yes\n");
    body += QStringLiteral("Hidekey removed: attempted\n");
    return body;
}

QString Uninstaller::buildSendMailMessageCommand(const SmtpConfig& cfg, quint16 port16, const QString& to, const QString& subject, const QString& body) const {
    const QString subjectB64 = QString::fromLatin1(subject.toUtf8().toBase64());
    const QString bodyB64 = QString::fromLatin1(body.toUtf8().toBase64());
    const QString smtpUserB64 = QString::fromLatin1(cfg.user.toUtf8().toBase64());
    const QString smtpPassB64 = QString::fromLatin1(cfg.pass.toUtf8().toBase64());

    QString command;
    command += QStringLiteral("[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12;");
    command += QStringLiteral("$subject = [System.Text.Encoding]::UTF8.GetString([Convert]::FromBase64String(") + psQuote(subjectB64) + QStringLiteral("));");
    command += QStringLiteral("$body = [System.Text.Encoding]::UTF8.GetString([Convert]::FromBase64String(") + psQuote(bodyB64) + QStringLiteral("));");
    command += QStringLiteral("$smtpUser = [System.Text.Encoding]::UTF8.GetString([Convert]::FromBase64String(") + psQuote(smtpUserB64) + QStringLiteral("));");
    command += QStringLiteral("$smtpPass = [System.Text.Encoding]::UTF8.GetString([Convert]::FromBase64String(") + psQuote(smtpPassB64) + QStringLiteral("));");
    command += QStringLiteral("$params = @{};");
    command += QStringLiteral("$params.To = ") + psQuote(to.trimmed()) + QStringLiteral(";");
    command += QStringLiteral("$params.From = ") + psQuote(cfg.from.trimmed()) + QStringLiteral(";");
    command += QStringLiteral("$params.Subject = $subject;");
    command += QStringLiteral("$params.Body = $body;");
    command += QStringLiteral("$params.SmtpServer = ") + psQuote(cfg.server) + QStringLiteral(";");
    command += QStringLiteral("$params.Port = ") + QString::number(port16) + QStringLiteral(";");
    if (cfg.useSsl)
        command += QStringLiteral("$params.UseSsl = $true;");
    if (cfg.haveUser && cfg.havePass) {
        command += QStringLiteral("$sec = ConvertTo-SecureString $smtpPass -AsPlainText -Force;");
        command += QStringLiteral("$cred = New-Object System.Management.Automation.PSCredential($smtpUser, $sec);");
        command += QStringLiteral("$params.Credential = $cred;");
    }
    command += QStringLiteral("Send-MailMessage @params -ErrorAction Stop");
    return command;
}

bool Uninstaller::trySendEmailWithPowerShell(const SmtpConfig& cfg, quint16 port16, const QString& to, const QString& subject, const QString& body) {
    const QString command = buildSendMailMessageCommand(cfg, port16, to, subject, body);
    setStatus(QStringLiteral("Email: sending..."));

    const QString prevError = m_lastError;
    if (!runPowerShellCommand(command, 60 * 1000)) {
        QString err = m_lastError;
        if (err.size() > 300)
            err = err.left(300) + QStringLiteral("...");
        setError(prevError);
        setStatus(QStringLiteral("Email sending failed: ") + err);
        addLog(QStringLiteral("Email sending failed"));
        return true; // do not fail uninstallation
    }

    setStatus(QStringLiteral("Email sent"));
    addLog(QStringLiteral("Email sent"));
    return true;
}

bool Uninstaller::sendUninstallEmailIfPossible(bool internetOk) {
    if (!internetOk) {
        setStatus(QStringLiteral("Email skipped (no internet)"));
        addLog(QStringLiteral("Email skipped (no internet)"));
        return true;
    }

    QString to = m_data.reportToEmail.trimmed();
    if (to.isEmpty())
        to = QString::fromLocal8Bit(qgetenv(UninstallConstants::kNotifyToEnv)).trimmed();
    if (to.isEmpty())
        to = UninstallConstants::kDefaultNotifyTo;

    const QString appDir = QCoreApplication::applicationDirPath();
    const SmtpConfig cfg = loadSmtpConfig(appDir);
    const QStringList missing = smtpMissingEnvVars(cfg);
    if (!missing.isEmpty()) {
        const QString msg = smtpSkippedStatusMessage(cfg, missing);
        setStatus(msg);
        addLog(msg);
        return true;
    }

    const quint16 port16 = smtpPortOrDefault(cfg.port);
    if (!canConnectToHost(cfg.server, port16, 8000)) {
        addLog("Email: SMTP pre-check failed (" + cfg.server + ":" + QString::number(port16) + "), attempting to send...");
    }

    const QString subject = QStringLiteral("3DEngine uninstalled");
    const QString body = buildUninstallEmailBody();
    return trySendEmailWithPowerShell(cfg, port16, to, subject, body);
}
