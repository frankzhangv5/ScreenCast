#include "MainWindow.h"

#include "App.h"
#include "MessageBox.h"
#include "MirrorWindow.h"
#include "SettingsWindow.h"
#include "TitleBar.h"

#include <QAction>
#include <QApplication>
#include <QCloseEvent>
#include <QDebug>
#include <QDesktopServices>
#include <QFile>
#include <QGraphicsOpacityEffect>
#include <QInputDialog>
#include <QLayoutItem>
#include <QLinearGradient>
#include <QPainter>
#include <QParallelAnimationGroup>
#include <QPropertyAnimation>
#include <QScreen>
#include <QStyle>
#include <QStyleFactory>
#include <QTimer>
#include <QToolBar>
#include <QUrl>
#include <QtAwesome.h>
#include <settings/Settings.h>

MainWindow::MainWindow(QWidget* parent) : FramelessWindow(parent)
{
    // Set window size to 9:16 ratio
    QScreen* screen = QApplication::primaryScreen();
    QRect screenGeometry = screen->geometry();
    int width = UI::WINDOW_WIDTH;
    int height = UI::WINDOW_HEIGHT;
    int x = (screenGeometry.width() - width) / 2;
    int y = (screenGeometry.height() - height) / 2;
    setGeometry(x, y, width, height);

    // Initialize QtAwesome for icon rendering
    m_awesome = new fa::QtAwesome(this);
    m_awesome->initFontAwesome();

    m_noDeviceContainer = nullptr;

    setupUI();
    setupConnections();

    // Start device monitoring
    startDeviceMonitor();

    // Initial device scan
    scanDevices();

    // 添加3秒定时器，确保即使没有设备连接，UI也会更新
    QTimer::singleShot(3000, this, [this]() {
        // 如果设备列表为空且仍然显示扫描标签，则更新UI
        if (m_devices.isEmpty() && m_infoLabel->isVisible())
        {
            updateDeviceList();
        }
    });
}

MainWindow::~MainWindow()
{
    // Stop device monitoring
    stopDeviceMonitor();
}

void MainWindow::setupUI()
{
    setFixedSize(UI::WINDOW_WIDTH, UI::WINDOW_HEIGHT);
    // 设置标题栏
    TitleBar* titleBar = getTitleBar();
    titleBar->setTitle(QIcon(":/icon/logo/#ffffff.svg").pixmap(20, 20));

    // Add settings icon to title bar toolbar
    QVariantMap options;
    options.insert("color", QColor(255, 255, 255));

    QToolBar* toolBar = titleBar->getToolBar();
    toolBar->setProperty("class", "Toolbar");
    QSize iconSize(14, 14);
    toolBar->setVisible(true);
    toolBar->addSeparator();
    toolBar->setIconSize(iconSize);

    // Helper lambda to add toolbar actions
    auto addToolbarAction = [&](const QString& event, uint16_t iconType) {
        QIcon icon = m_awesome->icon(fa::fa_solid, iconType, options).pixmap(iconSize);
        toolBar->addAction(icon, nullptr, this, [this, event]() {
            if (event == "home")
            {
                QDesktopServices::openUrl(QUrl(HOME_PAGE_URL));
            }
            else if (event == "settings")
            {
                onSettingsClicked();
            }
        });
    };

    addToolbarAction("home", fa::fa_home);
    addToolbarAction("settings", fa::fa_cog);

    // 获取页面容器并设置布局
    QWidget* page = getPage();
    m_listContainer = new QVBoxLayout(page);
    // Set content margins for m_listContainer
    m_listContainer->setContentsMargins(5, 5, 5, 5);

    // Device list
    m_deviceList = new QListWidget(page);
    m_deviceList->setProperty("class", "ListWidget");
    m_deviceList->hide();

    // Create device status info label and add to list container
    m_infoLabel = new QLabel(tr("Scanning for devices..."), page);
    m_infoLabel->setProperty("Color", "Primary");
    m_infoLabel->setProperty("class", "infoLabel");
    m_infoLabel->setAlignment(Qt::AlignCenter);

    // Add label to list container
    m_listContainer->addWidget(m_infoLabel, 1);

    // 设置状态栏信息
    StatusBar* statusBar = getStatusBar();
    statusBar->setStatusIcon(StatusType::Warning);
    statusBar->setStatusMessage(tr("Not ready"));

    // 设置右键菜单
    setupContextMenu();
}

void MainWindow::setupConnections()
{
    // Connect device list item click signals and slots
    connect(m_deviceList, &QListWidget::itemClicked, this, &MainWindow::onDeviceItemClicked);

    // Connect DeviceManager signals
    DeviceManager& deviceManager = getDeviceManager();
    connect(&deviceManager, &DeviceManager::deviceConnected, this, &MainWindow::onDeviceConnected);
    connect(&deviceManager, &DeviceManager::deviceDisconnected, this, &MainWindow::onDeviceDisconnected);
    connect(&deviceManager, &DeviceManager::deviceListChanged, this, &MainWindow::onDeviceListChanged);
}

void MainWindow::setupContextMenu()
{
    // 启用列表的右键菜单
    m_deviceList->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_deviceList, &QListWidget::customContextMenuRequested, this, &MainWindow::showContextMenu);
}

void MainWindow::onSettingsClicked()
{
    SettingsWindow* settingsWindow = new SettingsWindow();
    settingsWindow->show();
    settingsWindow->activateWindow();

    // 连接关闭信号到删除槽，避免WA_DeleteOnClose导致的崩溃
    connect(settingsWindow, &SettingsWindow::destroyed, settingsWindow, &SettingsWindow::deleteLater);

    // Add debug output
    qDebug() << "SettingsWindow show() called, visible:" << settingsWindow->isVisible();
}

void MainWindow::onDeviceItemClicked(QListWidgetItem* item)
{
    // Handle device item click event
    // Specific logic can be added here as needed
    qDebug() << "Device item clicked: " << item->text();
}

void MainWindow::onDeviceConnected(const QString& serial)
{
    // Handle when device connects
    DeviceInfo* deviceInfo = getDeviceInfo(serial);
    if (deviceInfo)
    {
        getStatusBar()->setStatusIcon(StatusType::Normal);
        getStatusBar()->setStatusMessage(tr("Device connected"));

        // 发送设备连接系统通知
        QString deviceName = deviceInfo->name.isEmpty() ? serial : deviceInfo->name;
        sendSystemNotification(tr("Device Connected"), tr("%1 has been connected successfully").arg(deviceName));
    }
}

void MainWindow::onDeviceDisconnected(const QString& serial)
{
    // Handle when device disconnects
    qDebug() << "Device disconnected:" << serial;

    // Get device name if available
    DeviceInfo* deviceInfo = getDeviceInfo(serial);
    QString deviceName = deviceInfo && !deviceInfo->name.isEmpty() ? deviceInfo->name : serial;

    // Update status bar
    getStatusBar()->setStatusMessage(tr("Device disconnected"));
    // 发送设备断开系统通知
    sendSystemNotification(tr("Device Disconnected"), tr("%1 has been disconnected").arg(deviceName));

    // Check if any devices are still connected
    if (m_devices.isEmpty())
    {
        getStatusBar()->setStatusIcon(StatusType::Error);
    }

    // No need to update m_devices, updateDeviceList will be called when deviceListChanged signal is emitted
}

void MainWindow::onDeviceListChanged(const QVector<DeviceInfo>& devices)
{
    // Handle when device list changes
    m_devices = devices;
    updateDeviceList();
}

QWidget* MainWindow::createDeviceItem(const DeviceInfo& device, int height)
{
    QWidget* itemWidget = new QWidget();
    itemWidget->setAttribute(Qt::WA_TranslucentBackground);
    itemWidget->setFixedHeight(height);
    // 存储设备序列号在widget的属性中，以便在右键菜单操作中使用
    itemWidget->setProperty("deviceSerial", device.serial);
    // Set background color to light blue
    QHBoxLayout* layout = new QHBoxLayout(itemWidget);
    layout->setContentsMargins(10, 10, 10, 10);

    // Device icon
    QLabel* iconLabel = new QLabel();
    QPixmap deviceIcon(":/icon/device/#008d4e.svg");
    iconLabel->setPixmap(deviceIcon.scaled(32, 32, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    iconLabel->setProperty("class", "DeviceIcon");

    // Device info
    QVBoxLayout* infoLayout = new QVBoxLayout();

    // Debug output to check device name value
    qDebug() << "Device name for display:" << device.name << "(length:" << device.name.length() << ")";

    // Ensure we have a fallback if name is empty
    QString displayName = device.name.isEmpty() ? tr("Unknown Device") : device.name;
    // Debug output to verify device name
    qDebug() << "Using device name:" << displayName;

    QLabel* nameLabel = new QLabel(displayName);
    nameLabel->setProperty("Color", "Primary");
    nameLabel->setProperty("class", "DeviceName");
    nameLabel->setStyleSheet("color: #333333; font-size: 14px; font-weight: bold;"); // Ensure visible style

    QLabel* serialLabel = new QLabel(device.serial);
    serialLabel->setProperty("Color", "Primary");
    serialLabel->setStyleSheet("color: #666666; font-size: 12px;"); // Ensure visible style

    infoLayout->addWidget(nameLabel);
    infoLayout->addWidget(serialLabel);

    // Start mirroring button with icon
    // 设置图标为白色
    QVariantMap options;
    options.insert("color", QColor(255, 255, 255)); // 白色图标

    QPushButton* mirrorButton = new QPushButton();
    mirrorButton->setIcon(m_awesome->icon(fa::fa_solid, fa::fa_play, options).pixmap(14, 14));
    // 使用Green.qss中定义的样式
    mirrorButton->setProperty("class", "MirrorButton");

    // Store device info in button
    mirrorButton->setProperty("deviceSerial", device.serial);
    connect(mirrorButton, &QPushButton::clicked, this, &MainWindow::onStartMirrorClicked);

    layout->addWidget(iconLabel);
    layout->addLayout(infoLayout);
    layout->addStretch();
    layout->addWidget(mirrorButton);

    itemWidget->setLayout(layout);

    return itemWidget;
}

void MainWindow::updateDeviceList()
{
    // Only update if device list is empty and currently not showing scanningLabel, or device list is not empty and
    // currently not showing deviceList
    if (m_devices.isEmpty())
    {
        getStatusBar()->setStatusIcon(StatusType::Error);
        getStatusBar()->setStatusMessage(tr("No devices detected"));
        getStatusBar()->setNotification("");

        // Update UI if currently showing deviceList or scanningLabel
        if (m_deviceList->isVisible() || m_infoLabel->isVisible())
        {
            m_deviceList->clear();
            m_deviceList->hide();
            m_listContainer->removeWidget(m_deviceList);

            // 创建一个容器widget和垂直布局，优化提示信息的显示
            QWidget* container = new QWidget(this);
            QVBoxLayout* vLayout = new QVBoxLayout(container);
            vLayout->setContentsMargins(20, 40, 20, 40);
            vLayout->setSpacing(0);

            // 使用单个QLabel和HTML格式来精确控制文本间距
            QString htmlContent = QString("<div style='text-align: center; line-height: 1.1;'>") +
                                  QString("<div style='font-weight: bold; font-size: 16px; color: #008D4E;'>%1</div>")
                                      .arg(tr("No Connected Devices")) +
                                  QString("<div style='font-size: 13px; color: #008D4E; margin-top: 12px;'>%1</div>")
                                      .arg(tr("Please connect your phone via USB")) +
                                  QString("<div style='font-size: 13px; color: #008D4E; margin-top: 1px;'>%1</div>")
                                      .arg(tr("Ensure USB debugging is enabled")) +
                                  QString("<div style='font-size: 13px; margin-top: 3px;'>") +
                                  QString(
                                      "<a href='%1' style='color: #0066cc; text-decoration: none; font-weight: "
                                      "bold;'>📖 %2</a>")
                                      .arg(App::HELP_URL)
                                      .arg(tr("Visit Help Website")) +
                                  QString("</div></div>");

            QLabel* infoLabel = new QLabel(htmlContent, container);
            infoLabel->setOpenExternalLinks(true);

            // 添加到布局
            vLayout->addWidget(infoLabel);

            // 将容器添加到列表容器中，设置为顶部居中对齐
            m_listContainer->addWidget(container, 0, Qt::AlignTop | Qt::AlignCenter);
            container->show();

            // 隐藏旧的infoLabel
            m_infoLabel->hide();

            // 保存容器指针到类成员变量中，以便后续清理
            m_noDeviceContainer = container;
        }
    }
    else
    {
        getStatusBar()->setStatusIcon(StatusType::Normal);
        getStatusBar()->setStatusMessage(tr("Ready"));
        getStatusBar()->setNotification(tr("Connected to %1 devices").arg(m_devices.size()));

        // Only update UI if not currently showing deviceList
        if (!m_deviceList->isVisible())
        {
            // 清理无设备时创建的容器
            if (m_noDeviceContainer)
            {
                if (m_listContainer->indexOf(m_noDeviceContainer) != -1)
                {
                    m_listContainer->removeWidget(m_noDeviceContainer);
                }
                m_noDeviceContainer->deleteLater();
                m_noDeviceContainer = nullptr;
            }

            // Hide device status info label
            m_infoLabel->hide();
            if (m_infoLabel->parent() == m_listContainer)
            {
                m_listContainer->removeWidget(m_infoLabel);
            }

            m_listContainer->addWidget(m_deviceList, 1);
            m_deviceList->show();
        }

        // Check if device list content needs to be updated
        if (m_deviceList->count() != m_devices.size())
        {
            // Clear and refill list
            m_deviceList->clear();
            QSize itemSize(0, 64);
            for (const DeviceInfo& device : m_devices)
            {
                QListWidgetItem* item = new QListWidgetItem();
                QWidget* itemWidget = createDeviceItem(device, itemSize.height());
                item->setSizeHint(itemSize);
                m_deviceList->addItem(item);
                m_deviceList->setItemWidget(item, itemWidget);
            }
        }
    }
}

// closeEvent is defined at the end of the file

void MainWindow::onStartMirrorClicked()
{
    QPushButton* button = qobject_cast<QPushButton*>(sender());
    if (button)
    {
        QString deviceSerial = button->property("deviceSerial").toString();
        qDebug() << "Start mirroring clicked for device serial:" << deviceSerial;

        // Find corresponding DeviceInfo struct by serial
        bool deviceFound = false;
        DeviceInfo deviceInfo;
        for (const DeviceInfo& device : m_devices)
        {
            if (device.serial == deviceSerial)
            {
                deviceInfo = device;
                deviceFound = true;
                break;
            }
        }

        if (deviceFound)
        {
            qDebug() << "Device found, creating mirror window for:" << deviceInfo.toString();

            // Create mirroring window
            MirrorWindow* mirrorWindow = new MirrorWindow(deviceInfo);
            mirrorWindow->show();
            mirrorWindow->activateWindow();

            // 连接关闭信号到删除槽
            connect(mirrorWindow, &MirrorWindow::destroyed, mirrorWindow, &MirrorWindow::deleteLater);
        }
        else
        {
            qWarning() << "Device not found for serial:" << deviceSerial;
        }
    }
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    // 隐藏窗口而不是销毁它，这样应用程序可以在后台继续运行
    FramelessWindow::closeEvent(event);
}

void MainWindow::showContextMenu(const QPoint& pos)
{
    QListWidgetItem* item = m_deviceList->itemAt(pos);
    if (!item)
        return;

    // 保存当前右键菜单选中的项目
    m_contextItem = item;
    QMenu* menu = createContextMenu();
    menu->exec(m_deviceList->mapToGlobal(pos));
    menu->deleteLater();
}

QMenu* MainWindow::createContextMenu()
{
    QMenu* menu = new QMenu(this);

    // 创建FontAwesome图标
    QVariantMap options;
    options.insert("color", QColor(255, 255, 255));

    // 重命名动作
    QAction* renameAction = new QAction(tr("Rename"), menu);
    QIcon renameIcon = m_awesome->icon(fa::fa_solid, fa::fa_edit, options).pixmap(12, 12);
    renameAction->setIcon(renameIcon);
    connect(renameAction, &QAction::triggered, this, &MainWindow::renameDevice);
    menu->addAction(renameAction);

    menu->addSeparator();

    // 切换置顶动作 - 根据置顶状态动态显示文本
    bool isPinned = false;
    if (m_contextItem)
    {
        QWidget* itemWidget = m_deviceList->itemWidget(m_contextItem);
        if (itemWidget)
        {
            // 从widget的属性中获取设备序列号
            QString deviceSerial = itemWidget->property("deviceSerial").toString();
            if (!deviceSerial.isEmpty())
            {
                isPinned = getDeviceManager().isDevicePinned(deviceSerial);
            }
        }
    }

    QString topActionText = isPinned ? tr("UnPin") : tr("Pin to Top");
    QAction* topAction = new QAction(topActionText, menu);
    QIcon topIcon = m_awesome->icon(fa::fa_solid, fa::fa_thumbtack, options).pixmap(12, 12);
    topAction->setIcon(topIcon);
    topAction->setCheckable(true);
    topAction->setChecked(isPinned);

    connect(topAction, &QAction::triggered, this, &MainWindow::toggleTopDevice);
    menu->addAction(topAction);

    return menu;
}

void MainWindow::renameDevice()
{
    if (!m_contextItem)
        return;

    QWidget* itemWidget = m_deviceList->itemWidget(m_contextItem);
    if (!itemWidget)
        return;

    // 从widget的属性中获取设备序列号
    QString deviceSerial = itemWidget->property("deviceSerial").toString();
    if (deviceSerial.isEmpty())
        return;

    // 查找对应的设备信息
    DeviceInfo deviceInfo;
    bool deviceFound = false;
    for (const DeviceInfo& device : m_devices)
    {
        if (device.serial == deviceSerial)
        {
            deviceInfo = device;
            deviceFound = true;
            break;
        }
    }

    if (!deviceFound)
        return;

    QString currentName = deviceInfo.name.isEmpty() ? tr("Unknown Device") : deviceInfo.name;

    bool ok;
    QString newName = QInputDialog::getText(
        this, tr("Rename Device"), tr("Enter new name for device:"), QLineEdit::Normal, currentName, &ok);

    if (ok && !newName.trimmed().isEmpty() && newName != currentName)
    {
        bool success = getDeviceManager().renameDevice(deviceSerial, newName);
        if (success)
        {
            qDebug() << "Device renamed successfully from" << currentName << "to" << newName;

            // 更新UI中的设备名称标签
            QLabel* nameLabel = itemWidget->findChild<QLabel*>(QString(), Qt::FindDirectChildrenOnly);
            if (nameLabel && nameLabel->property("class").toString() == "DeviceName")
            {
                nameLabel->setText(newName);
            }
            else
            {
                // 如果找不到直接子标签，尝试查找所有子标签
                QList<QLabel*> labels = itemWidget->findChildren<QLabel*>();
                for (QLabel* label : labels)
                {
                    if (label->property("class").toString() == "DeviceName")
                    {
                        label->setText(newName);
                        break;
                    }
                }
            }
        }
        else
        {
            qWarning() << "Failed to rename device:" << deviceSerial;
        }
    }
}

void MainWindow::toggleTopDevice()
{
    if (!m_contextItem)
        return;

    QWidget* itemWidget = m_deviceList->itemWidget(m_contextItem);
    if (!itemWidget)
        return;

    // 从widget的属性中获取设备序列号
    QString deviceSerial = itemWidget->property("deviceSerial").toString();
    if (deviceSerial.isEmpty())
        return;

    bool success = getDeviceManager().toggleTopDevice(deviceSerial);
    if (success)
    {
        bool isPinned = getDeviceManager().isDevicePinned(deviceSerial);
        // 查找设备名称用于日志输出
        QString deviceName = deviceSerial;
        for (const DeviceInfo& device : m_devices)
        {
            if (device.serial == deviceSerial)
            {
                deviceName = device.name.isEmpty() ? deviceSerial : device.name;
                break;
            }
        }
        qDebug() << "Device pin status toggled successfully:" << deviceName << "->"
                 << (isPinned ? "pinned" : "unpinned");
    }
    else
    {
        qWarning() << "Failed to toggle pin status for device:" << deviceSerial;
    }
}