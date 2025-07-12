#include "ui/MainWindow.h"

#include <QDesktopServices>
#include <QGuiApplication>
#include <QScreen>
#include <QToolBar>
#include <QUrl>
#include <QtAwesome.h>

MainWindow::MainWindow(QWidget* parent) : FramelessWindow(parent)
{
    // Replace page content with DeviceListPage
    DeviceListPage* deviceListPage = new DeviceListPage(this);
    getPage()->setLayout(new QVBoxLayout());
    getPage()->layout()->setContentsMargins(0, 0, 0, 0);
    getPage()->layout()->addWidget(deviceListPage);

    fa::QtAwesome* awesome = new fa::QtAwesome(this);
    awesome->initFontAwesome();
    QVariantMap options;
    options.insert("color", QColor(255, 255, 255));
    QIcon homeIcon = awesome->icon(fa::fa_solid, fa::fa_home, options).pixmap(14, 14);
    QIcon settingsIcon = awesome->icon(fa::fa_solid, fa::fa_cog, options).pixmap(14, 14);

    QToolBar* toolBar = getTitleBar()->getToolBar();
    toolBar->addSeparator();
    toolBar->setIconSize(QSize(14, 14));
    QAction* homeAction = toolBar->addAction(homeIcon, "");
    QAction* settingsAction = toolBar->addAction(settingsIcon, "");

    // Floating control
    toolBar->setFloatable(false); // Don't allow dragging out as independent panel

    // Set click event callbacks
    connect(homeAction, &QAction::triggered, this, [this]() {
        getStatusBar()->setStatus(AppConfig::HOME_PAGE_URL);
        QDesktopServices::openUrl(QUrl(AppConfig::HOME_PAGE_URL));
    });

    connect(settingsAction, &QAction::triggered, this, [this]() {
        // Open settings window, center display in main window center
        SettingsWindow* settingsWin = new SettingsWindow(this);
        settingsWin->setAttribute(Qt::WA_DeleteOnClose); // Auto delete when closed

        // Center display
        QPoint center = this->geometry().center();
        QSize winSize = settingsWin->sizeHint();
        int x = center.x() - winSize.width() / 2;
        int y = center.y() - winSize.height() / 2;

        settingsWin->move(x, y);
        settingsWin->show();
    });

    connect(&DeviceManager::instance(), &DeviceManager::deviceConnected, this, &MainWindow::onDeviceConnected);
    connect(&DeviceManager::instance(), &DeviceManager::deviceDisconnected, this, &MainWindow::onDeviceDisconnected);
}

MainWindow::~MainWindow() {}

void MainWindow::showEvent(QShowEvent* event)
{
    FramelessWindow::showEvent(event);

    // Move window to screen center
    QScreen* screen = QGuiApplication::primaryScreen();
    if (screen)
    {
        QRect screenGeometry = screen->geometry();
        QRect windowGeometry = this->geometry();

        int x = screenGeometry.center().x() - windowGeometry.width() / 2;
        int y = screenGeometry.center().y() - windowGeometry.height() / 2;

        this->move(x, y);
    }
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    FramelessWindow::closeEvent(event);
}

void MainWindow::onDeviceConnected(const DeviceInfo& dev)
{
    qDebug() << "DeviceListPage::onDeviceConnected: " << dev.toString();

    DeviceInfo* info = DeviceManager::instance().device(dev.serial);
    if (info)
    {
        sendSystemNotification(tr("Device connected"), info->name);
        getStatusBar()->setStatus(tr("Device connected: %1").arg(info->name));
    }
}

void MainWindow::onDeviceDisconnected(const DeviceInfo& dev)
{
    qDebug() << "DeviceListPage::onDeviceDisconnected: " << dev.toString();

    DeviceInfo* info = DeviceManager::instance().device(dev.serial);
    if (info)
    {
        sendSystemNotification(tr("Device disconnected"), info->name);
        getStatusBar()->setStatus(tr("Device disconnected: %1").arg(info->name));
    }
}
