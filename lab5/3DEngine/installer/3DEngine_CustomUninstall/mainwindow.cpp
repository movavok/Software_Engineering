#include "mainwindow.h"
#include "ui_mainwindow.h"

namespace {
constexpr const char* kRegistryBase = "HKEY_LOCAL_MACHINE\\SOFTWARE\\KNTU\\3DEngine";

const QRegularExpression& emailRe() {
    static const QRegularExpression re(QStringLiteral(R"(^[^\s@]+@[^\s@]+\.[^\s@]+$)"));
    return re;
}

bool isValidOptionalEmail(const QString& email) {
    const QString trimmed = email.trimmed();
    return trimmed.isEmpty() || emailRe().match(trimmed).hasMatch();
}
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    if (ui->gridLayout_finish) {
        ui->gridLayout_finish->setColumnStretch(0, 0);
        ui->gridLayout_finish->setColumnStretch(1, 1);
    }

    detectInstalledComponents();
    updateWindowByPage(ui->stackedWidget->currentIndex());

    if (ui->l_buildDateValue)
        ui->l_buildDateValue->setText(QStringLiteral(__DATE__));

    connect(ui->b_back, &QPushButton::clicked, this, &MainWindow::onPageBack);
    connect(ui->b_next, &QPushButton::clicked, this, &MainWindow::onPageNext);

    if (ui->le_reportEmail) {
        ui->le_reportEmail->setValidator(new QRegularExpressionValidator(emailRe(), this));
        connect(ui->le_reportEmail, &QLineEdit::textChanged, this, &MainWindow::validateWelcomeInputs);
    }

    validateWelcomeInputs();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::detectInstalledComponents() {
    m_data = UninstallData{};
    parseHidekeyIfPresent();
    readRegistryInfo();
    detectInstallDirFromUninstallRegistry();
    detectInstallDirHeuristics();

    const bool hasRegistry = !m_data.installSerial.trimmed().isEmpty();
    const bool hasHidekey = QFile::exists(UninstallConstants::kHidekeyPath) || !m_data.installEmail.trimmed().isEmpty();
    const bool hasExe = !m_data.installedExePath.trimmed().isEmpty();
    const bool hasDir = !m_data.installDir.trimmed().isEmpty();

    m_componentsDetected = hasRegistry || hasHidekey || hasExe || hasDir;

    if (ui->l_installedStatus) {
        ui->l_installedStatus->setText(m_componentsDetected ? QStringLiteral("Installed components detected: Yes")
                                                            : QStringLiteral("Installed components detected: No"));
    }

    validateWelcomeInputs();
}

void MainWindow::validateWelcomeInputs() {
    if (!ui->stackedWidget || ui->stackedWidget->currentIndex() != 0)
        return;
    const QString email = ui->le_reportEmail ? ui->le_reportEmail->text() : QString();
    ui->b_next->setEnabled(m_componentsDetected && isValidOptionalEmail(email));
}

void MainWindow::parseHidekeyIfPresent() {
    QFile f(UninstallConstants::kHidekeyPath);
    if (!f.exists())
        return;

    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    QTextStream in(&f);
    while (!in.atEnd()) {
        const QString line = in.readLine().trimmed();
        if (line.startsWith(QStringLiteral("Serial=")))
            m_data.installSerial = line.mid(QStringLiteral("Serial=").size()).trimmed();
        if (line.startsWith(QStringLiteral("Email=")))
            m_data.installEmail = line.mid(QStringLiteral("Email=").size()).trimmed();
    }
}

void MainWindow::readRegistryInfo() {
    QSettings settings(kRegistryBase, QSettings::NativeFormat);
    const QString serial = settings.value("Installsn").toString().trimmed();
    if (!serial.isEmpty())
        m_data.installSerial = serial;

    m_data.installDate = settings.value("Installdate").toString().trimmed();
    m_data.installTime = settings.value("Installtime").toString().trimmed();
}

void MainWindow::detectInstallDirFromUninstallRegistry() {
    if (!m_data.installDir.trimmed().isEmpty())
        return;

    const QStringList bases = {
        QStringLiteral("HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall"),
        QStringLiteral("HKEY_LOCAL_MACHINE\\SOFTWARE\\WOW6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall"),
        QStringLiteral("HKEY_CURRENT_USER\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall"),
    };

    auto tryBase = [&](const QString& baseKey) {
        QSettings s(baseKey, QSettings::NativeFormat);
        const QStringList groups = s.childGroups();
        for (const QString& g : groups) {
            s.beginGroup(g);
            const QString name = s.value("DisplayName").toString();
            if (name.contains(QStringLiteral("3DEngine"), Qt::CaseInsensitive)) {
                const QString loc = s.value("InstallLocation").toString().trimmed();
                if (!loc.isEmpty()) {
                    m_data.installDir = loc;
                    s.endGroup();
                    return true;
                }
            }
            s.endGroup();
        }
        return false;
    };

    for (const QString& base : bases) {
        if (tryBase(base))
            return;
    }
}

void MainWindow::detectInstallDirHeuristics() {
    const QString appDir = QCoreApplication::applicationDirPath();

    const auto considerExe = [&](const QString& exePath) {
        if (!QFile::exists(exePath))
            return;
        if (m_data.installedExePath.trimmed().isEmpty())
            m_data.installedExePath = QDir::toNativeSeparators(exePath);
        if (m_data.installDir.trimmed().isEmpty())
            m_data.installDir = QFileInfo(exePath).absolutePath();
    };

    considerExe(QDir(appDir).filePath(UninstallConstants::kExeName));
    considerExe(QDir(appDir).filePath(QStringLiteral("3DEngine/") + UninstallConstants::kExeName));

#ifdef Q_OS_WIN
    const QString pf = qEnvironmentVariable("ProgramFiles");
    const QString pfx86 = qEnvironmentVariable("ProgramFiles(x86)");
    if (!pf.trimmed().isEmpty()) {
        considerExe(QDir(pf).filePath(QStringLiteral("3DEngine/") + UninstallConstants::kExeName));
        considerExe(QDir(pf).filePath(QStringLiteral("KNTU/3DEngine/") + UninstallConstants::kExeName));
    }
    if (!pfx86.trimmed().isEmpty()) {
        considerExe(QDir(pfx86).filePath(QStringLiteral("3DEngine/") + UninstallConstants::kExeName));
        considerExe(QDir(pfx86).filePath(QStringLiteral("KNTU/3DEngine/") + UninstallConstants::kExeName));
    }
#endif
}

void MainWindow::onPageBack() {
    const int index = ui->stackedWidget->currentIndex();
    if (index <= 0)
        return;
    const int prev = index - 1;
    ui->stackedWidget->setCurrentIndex(prev);
    updateWindowByPage(prev);
}

void MainWindow::onPageNext() {
    const int index = ui->stackedWidget->currentIndex();

    switch (index) {
    case 0: {
        if (!m_componentsDetected) {
            close();
            return;
        }

        if (ui->le_reportEmail) {
            const QString email = ui->le_reportEmail->text();
            if (!isValidOptionalEmail(email)) {
                QMessageBox::warning(this, "Email", "Invalid email format.");
                return;
            }
        }

        const auto reply = QMessageBox::warning(
            this,
            "Warning",
            "Увага! Деінсталяція може вилучити файли програми та пов'язану інформацію з цього ПК.\n\nПродовжити?",
            QMessageBox::Ok | QMessageBox::Cancel);
        if (reply != QMessageBox::Ok)
            return;

        if (ui->le_reportEmail)
            m_data.reportToEmail = ui->le_reportEmail->text().trimmed();

        startUninstall();
        return;
    }

    case 2:
        close();
        return;

    default:
        break;
    }
}

bool MainWindow::startUninstall() {
    if (m_uninstallThread)
        return false;

    ui->stackedWidget->setCurrentIndex(1);
    updateWindowByPage(1);

    ui->pb_uninstallProgress->setValue(0);
    ui->l_uninstallStatus->setText(QStringLiteral("Preparing..."));
    ui->tb_steps->clear();

    m_uninstallThread = new QThread(this);
    m_uninstaller = new Uninstaller(m_data);
    m_uninstaller->moveToThread(m_uninstallThread);

    connect(m_uninstallThread, &QThread::started, m_uninstaller, &Uninstaller::run);
    connect(m_uninstaller, &Uninstaller::progressChanged, ui->pb_uninstallProgress, &QProgressBar::setValue);
    connect(m_uninstaller, &Uninstaller::statusChanged, ui->l_uninstallStatus, &QLabel::setText);
    connect(m_uninstaller, &Uninstaller::logLine, this, [this](const QString& line) {
        ui->tb_steps->append(line);
    });

    connect(m_uninstaller, &Uninstaller::finished, this, [this](bool ok, const QString& errorMessage) {
        if (ok) {
            ui->pb_uninstallProgress->setValue(100);
            ui->l_uninstallStatus->setText(QStringLiteral("Completed"));

            fillFinishPage();
            ui->stackedWidget->setCurrentIndex(2);
            updateWindowByPage(2);
        } else {
            ui->b_next->setEnabled(true);
            ui->b_back->setEnabled(true);
            ui->l_uninstallStatus->setText("Error: " + (errorMessage.isEmpty() ? QStringLiteral("Unknown error") : errorMessage));
            QMessageBox::critical(this, "Uninstallation failed", errorMessage.isEmpty() ? "Unknown error" : errorMessage);
        }

        if (m_uninstallThread)
            m_uninstallThread->quit();
    });

    connect(m_uninstallThread, &QThread::finished, m_uninstaller, &QObject::deleteLater);
    connect(m_uninstallThread, &QThread::finished, m_uninstallThread, &QObject::deleteLater);
    connect(m_uninstallThread, &QThread::finished, this, [this]() {
        m_uninstaller = nullptr;
        m_uninstallThread = nullptr;
    });

    m_uninstallThread->start();
    return true;
}

void MainWindow::fillFinishPage() {
    if (ui->l_finishPathValue)
        ui->l_finishPathValue->setText(m_data.installDir.trimmed().isEmpty() ? QStringLiteral("-") : m_data.installDir.trimmed());

    if (ui->l_finishDateValue) {
        const QString d = m_data.installDate.trimmed();
        ui->l_finishDateValue->setText(d.isEmpty() ? QDate::currentDate().toString("dd.MM.yyyy") : d);
    }
    if (ui->l_finishTimeValue) {
        const QString t = m_data.installTime.trimmed();
        ui->l_finishTimeValue->setText(t.isEmpty() ? QTime::currentTime().toString("HH:mm:ss") : t);
    }
    if (ui->l_finishSerialValue)
        ui->l_finishSerialValue->setText(m_data.installSerial.trimmed().isEmpty() ? QStringLiteral("-") : m_data.installSerial.trimmed());
}

void MainWindow::updateWindowByPage(int index) {
    struct PageConfig {
        const char* title;
        int width;
        int height;
        bool backEnabled;
        bool nextEnabled;
        const char* nextText;
    };

    static const PageConfig cfg[] = {
        {"Welcome", 320, 290, false, true, "Next"},
        {"Uninstalling", 520, 380, false, false, "Next"},
        {"Finish", 450, 280, false, true, "Finish"},
    };

    ui->b_next->setEnabled(true);
    ui->b_back->setEnabled(true);
    ui->b_next->setText(QStringLiteral("Next"));

    const QString base = QStringLiteral("Uninstaller | ");
    if (index >= 0 && index < static_cast<int>(sizeof(cfg) / sizeof(cfg[0]))) {
        const PageConfig& c = cfg[index];
        setWindowTitle(base + QString::fromLatin1(c.title));
        setFixedSize(c.width, c.height);
        ui->b_back->setEnabled(c.backEnabled);
        ui->b_next->setEnabled(c.nextEnabled);
        ui->b_next->setText(QString::fromLatin1(c.nextText));

        if (index == 0)
            validateWelcomeInputs();

        return;
    }

    setFixedSize(600, 600);
}
