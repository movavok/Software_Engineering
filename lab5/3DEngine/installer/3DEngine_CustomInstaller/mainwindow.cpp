#include "mainwindow.h"
#include "ui_mainwindow.h"

namespace {
constexpr const char* kRegistryBase = "HKEY_LOCAL_MACHINE\\SOFTWARE\\KNTU\\3DEngine";

const QRegularExpression& serialRe() {
    static const QRegularExpression re(QStringLiteral("^[A-Z0-9]{4}-[A-Z0-9]{4}-[A-Z0-9]{4}$"));
    return re;
}

const QRegularExpression& emailRe() {
    static const QRegularExpression re(QStringLiteral(R"(^[^\s@]+@[^\s@]+\.[^\s@]+$)"));
    return re;
}

bool isValidInstallPath(const QString& path) {
    const QString trimmed = path.trimmed();
    if (trimmed.isEmpty())
        return false;

    QFileInfo info(trimmed);
    if (!info.isAbsolute())
        return false;

#ifdef Q_OS_WIN
    const QString native = QDir::toNativeSeparators(trimmed);
    if (native.size() < 3 || native[1] != QLatin1Char(':'))
        return false;
#endif

    return true;
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

    checkIfAppExist();

    updateWindowByPage(ui->stackedWidget->currentIndex());
    ui->l_buildDateValue->setText(QStringLiteral(__DATE__));

    connect(ui->b_back, &QPushButton::clicked, this, &MainWindow::onPageBack);
    connect(ui->b_next, &QPushButton::clicked, this, &MainWindow::onPageNext);

    getLicenseFromFile(":/LICENSE.txt");
    connect(ui->cb_acceptLicense, &QCheckBox::toggled, ui->b_next, &QPushButton::setEnabled);

    ui->le_serialNumber->setValidator(new QRegularExpressionValidator(serialRe(), this));

    if (ui->le_email) {
        ui->le_email->setValidator(new QRegularExpressionValidator(emailRe(), this));
    }

    connect(ui->le_serialNumber, &QLineEdit::textChanged, this, &MainWindow::validateSerialPageInputs);
    if (ui->le_email)
        connect(ui->le_email, &QLineEdit::textChanged, this, &MainWindow::validateSerialPageInputs);

    connect(ui->le_installPath, &QLineEdit::textChanged, this, &MainWindow::validateInstallPath);
    connect(ui->b_browse, &QPushButton::clicked, this, &MainWindow::chooseInstallDirectory);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::checkIfAppExist() {
    QSettings settings(kRegistryBase, QSettings::NativeFormat);

    if (settings.contains("Installsn")) {
        const auto reply = QMessageBox::question(
            this,
            "Already installed",
            "The program is already installed.\nContinue reinstallation?",
            QMessageBox::Yes | QMessageBox::No);

        if (reply == QMessageBox::No)
            close();
    }
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
    case 2:
        if (!checkSerial())
            return;
        break;

    case 4:
        if (!startInstallation())
            return;
        return;

    case 5:
        ui->stackedWidget->setCurrentIndex(4);
        updateWindowByPage(4);
        return;

    case 6:
        close();
        return;

    default:
        break;
    }

    const int next = index + 1;
    if (next >= ui->stackedWidget->count())
        return;

    ui->stackedWidget->setCurrentIndex(next);
    updateWindowByPage(next);
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
        {"Welcome", 290, 270, false, true, "Next"},
        {"License", 600, 600, true, false, "Next"},
        {"Serial", 290, 220, true, false, "Check"},
        {"Directory", 290, 160, true, false, "Next"},
        {"Settings", 400, 250, true, true, "Install"},
        {"Installing", 400, 150, false, false, "Next"},
        {"Finish", 450, 290, false, true, "Finish"},
    };

    ui->b_next->setEnabled(true);
    ui->b_back->setEnabled(true);
    ui->b_next->setText(QStringLiteral("Next"));

    const QString base = QStringLiteral("Installer | ");
    if (index >= 0 && index < static_cast<int>(sizeof(cfg) / sizeof(cfg[0]))) {
        const PageConfig& c = cfg[index];
        setWindowTitle(base + QString::fromLatin1(c.title));
        setFixedSize(c.width, c.height);
        ui->b_back->setEnabled(c.backEnabled);
        ui->b_next->setEnabled(c.nextEnabled);
        ui->b_next->setText(QString::fromLatin1(c.nextText));

        if (index == 5)
            ui->l_installStatus->setText(QStringLiteral("Preparing..."));

        return;
    }

    setFixedSize(600, 600);
}

void MainWindow::getLicenseFromFile(const QString& path) {
    QFile file(path);
    if(file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        ui->tb_licenseText->setText(in.readAll());
        file.close();
    }
}

bool MainWindow::checkSerial() {
    const QString serial = ui->le_serialNumber->text();

    if (!serialRe().match(serial).hasMatch()) {
        QMessageBox::warning(this, "Error", "Invalid serial number format!");
        return false;
    }

    const QString email = ui->le_email ? ui->le_email->text().trimmed() : QString();
    if (!email.isEmpty()) {
        if (!emailRe().match(email).hasMatch()) {
            QMessageBox::warning(this, "Error", "Invalid email format!");
            return false;
        }
    }

    return true;
}

void MainWindow::validateSerialPageInputs() {
    if (ui->stackedWidget->currentIndex() != 2)
        return;

    const QString serial = ui->le_serialNumber->text();
    const bool serialValid = serialRe().match(serial).hasMatch();

    const QString email = ui->le_email ? ui->le_email->text().trimmed() : QString();
    const bool emailValid = email.isEmpty() || emailRe().match(email).hasMatch();

    ui->b_next->setEnabled(serialValid && emailValid);
}

void MainWindow::chooseInstallDirectory() {
    const QString dir = QFileDialog::getExistingDirectory(this, "Select Install Directory", "C:/Program Files");
    if(!dir.isEmpty())
        ui->le_installPath->setText(dir);
}

void MainWindow::validateInstallPath(const QString& path) {
    if(ui->stackedWidget->currentIndex() != 3)
        return;
    ui->b_next->setEnabled(isValidInstallPath(path));
}

bool MainWindow::startInstallation() {
    m_lastEmailStatus.clear();

    InstallData data;
    if (!collectInstallData(data))
        return false;

    m_currentInstallData = data;

    ui->stackedWidget->setCurrentIndex(5);
    updateWindowByPage(5);

    ui->pb_installProgress->setValue(0);
    ui->l_installStatus->setText("Preparing...");

    QTimer::singleShot(0, this, [this, data]() {
        startInstallationAsync(data);
    });

    return true;
}

bool MainWindow::collectInstallData(InstallData& data) {
    const QString installPath = ui->le_installPath->text();

    if (installPath.isEmpty()) {
        QMessageBox::warning(this, "Error", "Please select an installation folder!");
        return false;
    }

    if (!isValidInstallPath(installPath)) {
        QMessageBox::warning(this, "Error", "Invalid installation path!");
        return false;
    }

    QDir dir(installPath);
    if (!dir.exists() && !QDir().mkpath(installPath)) {
        QMessageBox::critical(this, "Error", "Cannot create installation folder!");
        return false;
    }

    data.installPath = installPath;
    data.exeFileName = "3DEngine.exe";
    data.desktopShortcut = ui->cb_desktopShortcut->isChecked();
    data.startMenuShortcut = ui->cb_startMenuShortcut->isChecked();
    data.runAfterInstall = ui->cb_runAfterInstall->isChecked();
    data.serialNumber = ui->le_serialNumber->text();
    data.email = ui->le_email ? ui->le_email->text().trimmed() : QString();

    return true;
}

void MainWindow::startInstallationAsync(const InstallData& data) {
    if (m_installThread)
        return;

    m_installThread = new QThread(this);
    m_installer = new Installer(data);
    m_installer->moveToThread(m_installThread);

    connect(m_installThread, &QThread::started, m_installer, &Installer::run);
    connect(m_installer, &Installer::progressChanged, ui->pb_installProgress, &QProgressBar::setValue);
    connect(m_installer, &Installer::statusChanged, ui->l_installStatus, &QLabel::setText);
    connect(m_installer, &Installer::statusChanged, this, [this](const QString& status) {
        if (status.startsWith("Email", Qt::CaseInsensitive))
            m_lastEmailStatus = status;
    });

    connect(m_installer, &Installer::finished, this, [this](bool ok, const QString& errorMessage) {
        if (ok) {
            ui->pb_installProgress->setValue(100);
            ui->l_installStatus->setText("Completed");

            fillFinishPage(m_currentInstallData);
            ui->stackedWidget->setCurrentIndex(6);
            updateWindowByPage(6);

            if (!m_currentInstallData.email.trimmed().isEmpty() && !m_lastEmailStatus.trimmed().isEmpty()) {
                if (m_lastEmailStatus.contains("failed", Qt::CaseInsensitive) ||
                    m_lastEmailStatus.contains("not sent", Qt::CaseInsensitive) ||
                    m_lastEmailStatus.contains("skipped", Qt::CaseInsensitive)) {
                    QMessageBox::warning(this, "Email", m_lastEmailStatus);
                }
            }
        } else {
            ui->b_next->setEnabled(true);
            ui->b_next->setText("Back");
            ui->b_back->setEnabled(true);
            ui->l_installStatus->setText("Error: " + (errorMessage.isEmpty() ? "Unknown error" : errorMessage));
            QMessageBox::critical(this, "Installation failed", errorMessage.isEmpty() ? "Unknown error" : errorMessage);
        }

        if (m_installThread)
            m_installThread->quit();
    });

    connect(m_installThread, &QThread::finished, m_installer, &QObject::deleteLater);
    connect(m_installThread, &QThread::finished, m_installThread, &QObject::deleteLater);
    connect(m_installThread, &QThread::finished, this, [this]() {
        m_installer = nullptr;
        m_installThread = nullptr;
    });

    m_installThread->start();
}

void MainWindow::fillFinishPage(const InstallData& data) {
    if (ui->l_finishPathValue)
        ui->l_finishPathValue->setText(data.installPath);

    QSettings settings(kRegistryBase, QSettings::NativeFormat);
    const QString installDate = settings.value("Installdate").toString();
    const QString installTime = settings.value("Installtime").toString();
    const QString installSn = settings.value("Installsn").toString();

    if (ui->l_finishDateValue)
        ui->l_finishDateValue->setText(installDate.isEmpty() ? QDate::currentDate().toString("dd.MM.yyyy") : installDate);
    if (ui->l_finishTimeValue)
        ui->l_finishTimeValue->setText(installTime.isEmpty() ? QTime::currentTime().toString("HH:mm:ss") : installTime);
    if (ui->l_finishSerialValue)
        ui->l_finishSerialValue->setText(installSn.isEmpty() ? data.serialNumber : installSn);

    if (ui->l_finishEmailValue) {
        const QString email = data.email.trimmed();
        ui->l_finishEmailValue->setText(email.isEmpty() ? QStringLiteral("-") : email);
    }

    if (ui->l_finishDesktopValue)
        ui->l_finishDesktopValue->setText(data.desktopShortcut ? QStringLiteral("Yes") : QStringLiteral("No"));
    if (ui->l_finishStartMenuValue)
        ui->l_finishStartMenuValue->setText(data.startMenuShortcut ? QStringLiteral("Yes") : QStringLiteral("No"));
    if (ui->l_finishRunValue)
        ui->l_finishRunValue->setText(data.runAfterInstall ? QStringLiteral("Yes") : QStringLiteral("No"));
}
