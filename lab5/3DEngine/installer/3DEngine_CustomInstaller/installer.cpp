#include "installer.h"

void Installer::run() {
    const bool ok = startInstallation();
    emit finished(ok, ok ? QString() : m_lastError);
}

void Installer::setProgress(int value) {
    if (value == m_progress)
        return;
    m_progress = value;
    emit progressChanged(m_progress);
}

void Installer::setStatus(const QString& status) {
    if (status == m_status)
        return;
    m_status = status;
    emit statusChanged(m_status);
}

void Installer::setError(const QString& message) {
    m_lastError = message;
}

QString Installer::psQuote(const QString& value) {
    QString escaped = value;
    escaped.replace("'", "''");
    return "'" + escaped + "'";
}

bool Installer::runPowerShellCommand(const QString& command, int timeoutMs) {
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

QString Installer::resolveInstalledExePath() const {
    const QString base = QDir::toNativeSeparators(m_data.installPath);

    const QString direct = QDir(base).filePath(m_data.exeFileName);
    if (QFile::exists(direct))
        return direct;

    const QString inSubfolder = QDir(base).filePath(QStringLiteral("3DEngine") + QDir::separator() + m_data.exeFileName);
    if (QFile::exists(inSubfolder))
        return inSubfolder;

    QDirIterator it(base, QStringList() << m_data.exeFileName, QDir::Files, QDirIterator::Subdirectories);
    if (it.hasNext())
        return it.next();

    return {};
}

bool Installer::startInstallation() {
    setProgress(0);
    setStatus("Preparing...");

    setStatus("Extracting files...");
    if (!extractArchive())
        return false;

    m_installedExePath = resolveInstalledExePath();
    if (m_installedExePath.isEmpty()) {
        setError("Installed executable not found after extraction.");
        return false;
    }

    if (m_data.desktopShortcut) {
        setStatus("Creating desktop shortcut...");
        if (!createDesktopShortcut())
            return false;
    }

    if (m_data.startMenuShortcut) {
        setStatus("Creating Start Menu shortcut...");
        if (!createStartMenuShortcut())
            return false;
    }

    setStatus("Writing registry entries...");
    if (!createRegistry())
        return false;

    setStatus("Creating Hidekey.dat...");
    if (!createHidekeyFile())
        return false;

    setStatus("Checking internet connection...");
    const bool internetOk = hasInternetConnection(3000);
    if (!internetOk)
        setStatus("No internet connection detected");

    if (!sendInstallEmailIfPossible())
        return false;

    if (m_data.runAfterInstall) {
        setStatus("Launching application...");
        QProcess::startDetached(m_installedExePath);
    }

    setProgress(100);
    setStatus("Completed");
    return true;
}

bool Installer::createHidekeyFile() {
    const QString dirPath = QStringLiteral("C:/Temp");
    if (!QDir().mkpath(dirPath)) {
        setError("Failed to create C:/Temp directory.");
        return false;
    }

    QFile file(dirPath + "/Hidekey.dat");
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        setError("Failed to create C:/Temp/Hidekey.dat");
        return false;
    }

    QTextStream out(&file);
    out << "Product=3DEngine\n";
    out << "InstallDate=" << QDate::currentDate().toString("dd.MM.yyyy") << "\n";
    out << "InstallTime=" << QTime::currentTime().toString("HH:mm:ss") << "\n";
    out << "Serial=" << m_data.serialNumber << "\n";
    out << "Email=" << m_data.email << "\n";
    file.close();
    return true;
}

bool Installer::extractArchive() {
    QDir().mkpath(m_data.installPath);

    QString archivePath = QCoreApplication::applicationDirPath() + "/package.zip";

    if (!QFile::exists(archivePath)) {
        setError("package.zip not found next to the installer.");
        return false;
    }

    const QString command =
        "Expand-Archive -LiteralPath " + psQuote(QDir::toNativeSeparators(archivePath)) +
        " -DestinationPath " + psQuote(QDir::toNativeSeparators(m_data.installPath)) +
        " -Force";

    if (!runPowerShellCommand(command, 5 * 60 * 1000))
        return false;

    setProgress(50);
    return true;
}

bool Installer::createDesktopShortcut() {
    const QString desktopPath = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
    const QString shortcutPath = desktopPath + "/3DEngine.lnk";
    return createShortcut(shortcutPath, 75);
}

bool Installer::createStartMenuShortcut() {
    const QString startMenuPath = QStandardPaths::writableLocation(QStandardPaths::ApplicationsLocation);

    if (startMenuPath.isEmpty()) {
        setError("Cannot determine Start Menu path.");
        return false;
    }

    QDir().mkpath(startMenuPath);
    const QString shortcutPath = startMenuPath + "/3DEngine.lnk";
    const int nextProgress = (m_progress < 85) ? 85 : m_progress;
    return createShortcut(shortcutPath, nextProgress);
}

bool Installer::createShortcut(const QString& shortcutPath, int progressAfter) {
    const QString workingDir = QFileInfo(m_installedExePath).absolutePath();

    const QString command =
        "$WshShell = New-Object -ComObject WScript.Shell;"
        "$Shortcut = $WshShell.CreateShortcut(" + psQuote(QDir::toNativeSeparators(shortcutPath)) + ");"
        "$Shortcut.TargetPath = " + psQuote(QDir::toNativeSeparators(m_installedExePath)) + ";"
        "$Shortcut.WorkingDirectory = " + psQuote(QDir::toNativeSeparators(workingDir)) + ";"
        "$Shortcut.Save();";

    if (!runPowerShellCommand(command, 60 * 1000))
        return false;

    setProgress(progressAfter);
    return true;
}

bool Installer::createRegistry() {
    QSettings settings(kRegistryBase, QSettings::NativeFormat);
    settings.setValue("Installtime", QTime::currentTime().toString("HH:mm:ss"));
    settings.setValue("Installdate", QDate::currentDate().toString("dd.MM.yyyy"));
    settings.setValue("Installsn", m_data.serialNumber);

    if (settings.status() != QSettings::NoError) {
        setError("Failed to write registry keys. Please run the installer as Administrator.");
        return false;
    }

    setProgress(90);
    return true;
}

bool Installer::hasInternetConnection(int timeoutMs) {
    return canConnectToHost(QStringLiteral("8.8.8.8"), 53, timeoutMs);
}

bool Installer::canConnectToHost(const QString& host, quint16 port, int timeoutMs) {
    if (host.trimmed().isEmpty() || port == 0)
        return false;

    QTcpSocket socket;
    socket.connectToHost(host, port);
    const bool ok = socket.waitForConnected(timeoutMs);
    if (ok)
        socket.disconnectFromHost();
    return ok;
}

bool Installer::sendInstallEmailIfPossible() {
    const QString recipient = m_data.email.trimmed();
    if (recipient.isEmpty()) {
        setStatus("Email skipped (no recipient specified)");
        return true;
    }

    const QString appDir = QCoreApplication::applicationDirPath();
    const SmtpConfig cfg = loadSmtpConfig(appDir);
    const QStringList missing = smtpMissingEnvVars(cfg);
    if (!missing.isEmpty()) {
        setStatus(smtpSkippedStatusMessage(cfg, missing));
        return true;
    }

    const quint16 port16 = smtpPortOrDefault(cfg.port);
    if (!canConnectToHost(cfg.server, port16, 8000)) {
        setStatus("Email: SMTP pre-check failed (" + cfg.server + ":" + QString::number(port16) + "), attempting to send...");
    }

    const QString subject = QStringLiteral("3DEngine installed");
    const QString body = buildInstallEmailBody();
    return trySendEmailWithPowerShell(cfg, port16, subject, body);
}

QStringList Installer::smtpCandidateIniPaths(const QString& appDir) const {
    return {
        QDir(appDir).filePath(QStringLiteral("smtp.ini")),
        QDir(QDir::currentPath()).filePath(QStringLiteral("smtp.ini")),
        QDir(appDir).absoluteFilePath(QStringLiteral("../smtp.ini")),
    };
}

QString Installer::chooseSmtpIniPath(const QString& appDir, const QStringList& candidates) const {
    for (const QString& path : candidates) {
        if (QFile::exists(path))
            return path;
    }
    return QDir(appDir).filePath(QStringLiteral("smtp.ini"));
}

Installer::SmtpConfig Installer::loadSmtpConfig(const QString& appDir) const {
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

QStringList Installer::smtpMissingEnvVars(const SmtpConfig& cfg) const {
    QStringList missing;
    if (cfg.server.trimmed().isEmpty())
        missing << QStringLiteral("INSTALLER_SMTP_SERVER");
    if (cfg.from.trimmed().isEmpty())
        missing << QStringLiteral("INSTALLER_SMTP_FROM");
    if (cfg.haveUser != cfg.havePass)
        missing << (cfg.haveUser ? QStringLiteral("INSTALLER_SMTP_PASS") : QStringLiteral("INSTALLER_SMTP_USER"));
    return missing;
}

QString Installer::smtpSkippedStatusMessage(const SmtpConfig& cfg, const QStringList& missingVars) const {
    QString hint;
    if (cfg.isGmailSender) {
        hint = QStringLiteral(" (Gmail: set INSTALLER_SMTP_USER + INSTALLER_SMTP_PASS (App Password) via env vars or smtp.ini; server defaults to smtp.gmail.com:587)");
    }

    const QString where = QFile::exists(cfg.iniPath)
                              ? (QStringLiteral(" (using ") + cfg.iniPath + QStringLiteral(")"))
                              : (QStringLiteral(" (smtp.ini not found; searched: ") + cfg.searchedIniPaths.join(QStringLiteral("; ")) + QStringLiteral(")"));

    return QStringLiteral("Email skipped (SMTP not configured: missing ") + missingVars.join(QStringLiteral(", ")) + QStringLiteral(")") + hint + where;
}

quint16 Installer::smtpPortOrDefault(const QString& portText) const {
    const QString port = portText.trimmed().isEmpty() ? QStringLiteral("587") : portText.trimmed();
    bool ok = false;
    const uint portNum = port.toUInt(&ok);
    if (!ok || portNum > 65535)
        return static_cast<quint16>(587);
    return static_cast<quint16>(portNum);
}

QString Installer::buildInstallEmailBody() const {
    return QStringLiteral("3DEngine installation report\n\n") +
           QStringLiteral("Install path: ") + m_data.installPath + QStringLiteral("\n") +
           QStringLiteral("Date: ") + QDate::currentDate().toString(QStringLiteral("dd.MM.yyyy")) + QStringLiteral("\n") +
           QStringLiteral("Time: ") + QTime::currentTime().toString(QStringLiteral("HH:mm:ss")) + QStringLiteral("\n") +
           QStringLiteral("Serial: ") + m_data.serialNumber + QStringLiteral("\n");
}

QString Installer::buildSendMailMessageCommand(const SmtpConfig& cfg, quint16 port16, const QString& subject, const QString& body) const {
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
    command += QStringLiteral("$params.To = ") + psQuote(m_data.email.trimmed()) + QStringLiteral(";");
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

bool Installer::trySendEmailWithPowerShell(const SmtpConfig& cfg, quint16 port16, const QString& subject, const QString& body) {
    const QString command = buildSendMailMessageCommand(cfg, port16, subject, body);

    setStatus(QStringLiteral("Email: sending..."));

    const QString prevError = m_lastError;
    if (!runPowerShellCommand(command, 60 * 1000)) {
        QString err = m_lastError;
        if (err.size() > 300)
            err = err.left(300) + QStringLiteral("...");
        setError(prevError);
        setStatus(QStringLiteral("Email sending failed: ") + err);
        return true;
    }

    setStatus(QStringLiteral("Email sent"));
    return true;
}
