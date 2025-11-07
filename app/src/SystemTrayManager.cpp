#include "SystemTrayManager.h"

#include "App.h"

#include <QApplication>
#include <QDebug>
#include <QStyle>

SystemTrayManager::SystemTrayManager(QObject* parent) : QObject(parent)
{
    // 初始化系统托盘图标和菜单
    m_awesome = new fa::QtAwesome(this);
    m_awesome->initFontAwesome();
    setupSystemTray();
}

SystemTrayManager::~SystemTrayManager()
{
    // 清理系统托盘资源
    if (m_trayIcon)
    {
        m_trayIcon->hide();
        delete m_trayIcon;
        m_trayIcon = nullptr;
    }

    // 清理菜单和动作资源
    if (m_trayMenu)
    {
        delete m_trayMenu;
        m_trayMenu = nullptr;
    }
    if (m_showMainWindowAction)
    {
        delete m_showMainWindowAction;
        m_showMainWindowAction = nullptr;
    }
    if (m_quitAction)
    {
        delete m_quitAction;
        m_quitAction = nullptr;
    }
    if (m_awesome)
    {
        delete m_awesome;
        m_awesome = nullptr;
    }
}

void SystemTrayManager::setupSystemTray()
{
    // 创建系统托盘图标
    m_trayIcon = new QSystemTrayIcon(this);

    // 设置托盘图标
#ifdef Q_OS_WIN
    // Windows: prefer ICO format for best tray and notification support
    QIcon appIcon = QIcon(":/app_icons/windows_icon.ico");
#else
    // Linux and other platforms: use PNG
    QIcon appIcon = QIcon(":/icon/tray/tray.png");
#endif

    if (appIcon.isNull())
    {
        // 如果图标加载失败，使用默认图标
        appIcon = QApplication::style()->standardIcon(QStyle::SP_ComputerIcon);
    }
    m_trayIcon->setIcon(appIcon);

    // 设置托盘图标提示文本
    m_trayIcon->setToolTip(
        QString("%1 %2").arg(QApplication::applicationName()).arg(QApplication::applicationVersion()));

    // 创建托盘菜单
    m_trayMenu = new QMenu();

    QVariantMap opts;
#ifdef Q_OS_MAC
    opts.insert("color", QColor("black"));
#else
    opts.insert("color", QColor("white"));
#endif
    QIcon showIcon = m_awesome->icon(fa::fa_solid, fa::fa_home, opts).pixmap(14, 14);
    // 创建菜单动作
    m_showMainWindowAction = new QAction(showIcon, tr("Show Window"), this);
    m_showMainWindowAction->setIconVisibleInMenu(true);

    QIcon quitIcon = m_awesome->icon(fa::fa_solid, fa::fa_sign_out, opts).pixmap(14, 14);
    m_quitAction = new QAction(quitIcon, tr("Quit"), this);
    m_quitAction->setIconVisibleInMenu(true);

    // 将动作添加到菜单
    m_trayMenu->addAction(m_showMainWindowAction);
    m_trayMenu->addSeparator();
    m_trayMenu->addAction(m_quitAction);

    // 设置托盘菜单
    m_trayIcon->setContextMenu(m_trayMenu);
    m_trayIcon->setVisible(true);

    // 连接信号槽
    connect(m_trayIcon, &QSystemTrayIcon::activated, this, &SystemTrayManager::onTrayIconActivated);
    connect(m_showMainWindowAction, &QAction::triggered, this, &SystemTrayManager::showMainWindow);
    connect(m_quitAction, &QAction::triggered, this, &SystemTrayManager::quitApplication);

    // 显示托盘图标
    m_trayIcon->show();
}

void SystemTrayManager::showNotification(const QString& title, const QString& message, int timeout)
{
    if (m_trayIcon && m_trayIcon->isVisible())
    {
        m_trayIcon->showMessage(title, message, QSystemTrayIcon::Information, timeout);
    }
}

bool SystemTrayManager::isVisible() const
{
    return m_trayIcon && m_trayIcon->isVisible();
}

void SystemTrayManager::showMainWindow()
{
    emit showMainWindowRequested();
}

void SystemTrayManager::quitApplication()
{
    emit quitRequested();
}

void SystemTrayManager::onTrayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
    switch (reason)
    {
        case QSystemTrayIcon::Trigger:     // 单击
        case QSystemTrayIcon::DoubleClick: // 双击
            showMainWindow();
            break;
        case QSystemTrayIcon::MiddleClick: // 中键点击
            // 可以添加自定义操作
            break;
        default:
            break;
    }
}