#include "ui/TrayManager.h"

#include "device/Device.h"

#include <QApplication>
#include <QFile>
#include <QIcon>
#include <QtAwesome.h>

TrayManager* TrayManager::instance()
{
    static TrayManager manager;
    return &manager;
}

TrayManager::TrayManager(QObject* parent) : QObject(parent)
{
    // Cross-platform icon selection
    QIcon icon;

#ifdef Q_OS_WIN
    // Windows: prefer ICO format for best tray and notification support
    icon = QIcon(":/app_icons/windows_icon");
#elif defined(Q_OS_MAC)
    // macOS: prefer ICNS format
    icon = QIcon(":/icons/tray");
#else
    // Linux and other platforms: use PNG
    icon = QIcon(":/icons/tray");
#endif

    trayIcon = new QSystemTrayIcon(icon, this);
    trayIcon->setToolTip(qApp->applicationName());
    setupTrayMenu();
    trayIcon->setContextMenu(trayMenu);
    trayIcon->setVisible(true);
    trayIcon->show();
    connect(trayIcon, &QSystemTrayIcon::activated, this, &TrayManager::onTrayIconActivated);
}

void TrayManager::setMainWindow(QWidget* window)
{
    mainWindow = window;
}

void TrayManager::showNotification(const QString& title, const QString& message, int timeout)
{
    trayIcon->showMessage(title, message, QSystemTrayIcon::Information, timeout);
}

void TrayManager::setupTrayMenu()
{
    trayMenu = new QMenu();

    fa::QtAwesome* awesome = new fa::QtAwesome(this);
    awesome->initFontAwesome();
    QVariantMap opts;
#ifdef Q_OS_MAC
    opts.insert("color", QColor("black"));
#else
    opts.insert("color", QColor("white"));
#endif
    QIcon showIcon = awesome->icon(fa::fa_solid, fa::fa_home, opts).pixmap(14, 14);
    QAction* showAction = trayMenu->addAction(showIcon, tr("Show"));

    QIcon exitIcon = awesome->icon(fa::fa_solid, fa::fa_sign_out, opts).pixmap(14, 14);
    QAction* quitAction = trayMenu->addAction(exitIcon, tr("Exit"));
    connect(showAction, &QAction::triggered, this, &TrayManager::onShowMainWindow);
    connect(quitAction, &QAction::triggered, this, &TrayManager::onQuit);

    trayMenu->setStyleSheet(R"(
                    QMenu {
                        background-color: #008D4E;
                        color: white;
                        border-radius: 6px;
                        padding: 4px 0;
                    }
                    QMenu::item {
                        background-color: transparent;
                        padding: 4px 0px;
                        margin: 2px 4px;
                        font: bold 12px;
                        min-width: 70px;
                    }
                    QMenu::item:selected {
                        background-color: #00804E;
                    }
                    QMenu::item:disabled {
                        color: #6e7681;
                    }
                    QMenu::separator {
                        height: 1px;
                        background: #444c56;
                        margin: 4px 8px;
                    }
                    QMenu::icon {
                        margin: 0px 4px;
                    }
    )");
}

void TrayManager::onTrayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::Trigger)
    {
        emit trayIconLeftClicked();
        onShowMainWindow();
    }
}

void TrayManager::onShowMainWindow()
{
    if (mainWindow)
    {
        mainWindow->show();
        mainWindow->raise();
        mainWindow->activateWindow();
    }
}

void TrayManager::onQuit()
{
    DeviceManager::instance().stopMonitor();
    qApp->quit();
}
