#include "mainwindow.h"

#include <QApplication>
#include <QMessageBox>

#ifdef Q_OS_WIN
#include <windows.h>
#include <shellapi.h>

static bool isRunningAsAdmin() {
    BOOL isAdmin = FALSE;
    PSID adminGroup = nullptr;
    SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;
    if (AllocateAndInitializeSid(&ntAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS,
                                 0, 0, 0, 0, 0, 0, &adminGroup)) {
        CheckTokenMembership(nullptr, adminGroup, &isAdmin);
        FreeSid(adminGroup);
    }
    return isAdmin;
}

static bool relaunchAsAdmin() {
    wchar_t exePath[MAX_PATH];
    const DWORD len = GetModuleFileNameW(nullptr, exePath, MAX_PATH);
    if (len == 0 || len == MAX_PATH)
        return false;

    HINSTANCE result = ShellExecuteW(nullptr, L"runas", exePath, nullptr, nullptr, SW_SHOWNORMAL);
    return reinterpret_cast<intptr_t>(result) > 32;
}
#endif

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

#ifdef Q_OS_WIN
    if (!isRunningAsAdmin()) {
        const auto reply = QMessageBox::question(
            nullptr,
            "Administrator privileges required",
            "Uninstallation may require administrator privileges (removing files from Program Files).\n\nRestart as Administrator now?",
            QMessageBox::Yes | QMessageBox::No);

        if (reply == QMessageBox::Yes) {
            if (relaunchAsAdmin())
                return 0;
            QMessageBox::information(nullptr, "Uninstaller", "Administrator privileges were not granted. The uninstaller will close.");
            return 0;
        }
        return 0;
    }
#endif

    MainWindow w;
    w.show();
    return a.exec();
}
