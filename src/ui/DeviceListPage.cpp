#include "ui/DeviceListPage.h"

#include <QAction>
#include <QInputDialog>
#include <QLabel>
#include <QListWidget>
#include <QMenu>
#include <QVBoxLayout>
#include <QWidget>
#include <QtAwesome.h>

DeviceListPage::DeviceListPage(QWidget* parent) : QWidget(parent)
{
    setFixedHeight(AppConfig::PAGE_HEIGHT);
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // Use QListWidget as device list
    listWidget = new QListWidget(this);
    listWidget->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    listWidget->setFrameShape(QFrame::NoFrame);
    listWidget->setStyleSheet(R"(
        QListWidget {
            background-color: #f0f0f0;
            border: none;
            padding: 10px;
        }
        QListWidget::item {
            padding: 4px;
            margin: 1px 0px;
            border-radius: 4px;
            background: #008D4E;
            border: none;
        }
        QListWidget::item:hover {
            background: #00804E;
        }
        QScrollBar:vertical {
            width: 4px;
            background: none;
            margin: 2px 0 2px 0;
            border-radius: 5px;
            border: none;
        }
        QScrollBar::handle:vertical {
            background: #b2dfdb;
            min-height: 10px;
            border-radius: 5px;
        }
        QScrollBar::handle:vertical:hover {
            background: #26a69a;
        }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
            height: 0;
        }
        QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {
            background: none;
        }
    )");

    mainLayout->addWidget(listWidget, 1);
    setLayout(mainLayout);
    setStyleSheet("background-color: #f0f0f0;");

    // Initial load devices
    updateDeviceList(DeviceManager::instance().devices());

    // Listen for device change signals
    connect(&DeviceManager::instance(), &DeviceManager::deviceListChanged, this, &DeviceListPage::updateDeviceList);

    // Setup context menu
    setupContextMenu();
}

void DeviceListPage::updateDeviceList(const QVector<DeviceInfo>& devices)
{
    qDebug() << "DeviceListPage::updateDeviceList called with" << devices.size() << "devices";
    listWidget->clear();
    if (devices.isEmpty())
    {
        qDebug() << "DeviceListPage:: No devices, showing empty state";
        QWidget* container = new QWidget(this);
        QVBoxLayout* vLayout = new QVBoxLayout(container);
        vLayout->setContentsMargins(20, 16, 20, 16);
        vLayout->setSpacing(8);

        // Title with icon
        QLabel* titleLabel = new QLabel(tr("No Connected Devices"), container);
        titleLabel->setStyleSheet("font-weight: bold; font-size: 16px; color: white;");
        titleLabel->setAlignment(Qt::AlignCenter);

        // Subtitle
        QLabel* subtitleLabel = new QLabel(tr("Please connect your device via USB"), container);
        subtitleLabel->setStyleSheet("font-size: 13px; color: white;");
        subtitleLabel->setAlignment(Qt::AlignCenter);

        // Tips section
        QLabel* tipsTitleLabel = new QLabel(tr("Troubleshooting:"), container);
        tipsTitleLabel->setStyleSheet("font-weight: bold; font-size: 13px; color: white; padding-top: 12px;");

        QLabel* tipsLabel = new QLabel(tr("• Enable USB debugging on your device\n"
                                          "• Check if adb/hdc is properly installed\n"
                                          "• Try reconnecting the USB cable"),
                                       container);
        tipsLabel->setStyleSheet("font-size: 12px; color: white; line-height: 1.4;");

        // Help link
        QLabel* helpLabel =
            new QLabel(tr("<a href='%1' style='color: white; text-decoration: none; font-weight: bold;'>"
                          "📖 Visit Help Website</a>")
                           .arg(AppConfig::HELP_URL),
                       container);
        helpLabel->setOpenExternalLinks(true);
        helpLabel->setAlignment(Qt::AlignCenter);
        helpLabel->setStyleSheet("font-size: 13px; padding-bottom: 8px;");

        vLayout->addWidget(titleLabel);
        vLayout->addWidget(subtitleLabel);
        vLayout->addStretch();
        vLayout->addWidget(tipsTitleLabel);
        vLayout->addWidget(tipsLabel);
        vLayout->addStretch();
        vLayout->addWidget(helpLabel);

        container->setStyleSheet("background: #008D4E; border: 1px solid #008D4E; border-radius: 8px;");
        QListWidgetItem* item = new QListWidgetItem(listWidget);
        item->setSizeHint(QSize(0, 220));
        listWidget->addItem(item);
        listWidget->setItemWidget(item, container);
        return;
    }

    qDebug() << "DeviceListPage:: Adding" << devices.size() << "devices to list";
    for (const DeviceInfo& dev : devices)
    {
        qDebug() << "DeviceListPage::updateDeviceList: " << dev.toString();
        QListWidgetItem* item = new QListWidgetItem(listWidget);
        DeviceItem* widget = new DeviceItem(dev, listWidget);
        item->setSizeHint(QSize(0, 72));
        listWidget->addItem(item);
        listWidget->setItemWidget(item, widget);
    }
}

void DeviceListPage::setupContextMenu()
{
    // Enable context menu for list widget
    listWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(listWidget, &QListWidget::customContextMenuRequested, this, &DeviceListPage::showContextMenu);
}

void DeviceListPage::showContextMenu(const QPoint& pos)
{
    QListWidgetItem* item = listWidget->itemAt(pos);
    if (!item)
        return;

    // Check if it's a device item (not the empty state item)
    DeviceItem* deviceWidget = qobject_cast<DeviceItem*>(listWidget->itemWidget(item));
    if (!deviceWidget)
        return;

    contextItem = item;
    QMenu* menu = createContextMenu();
    menu->exec(listWidget->mapToGlobal(pos));
    menu->deleteLater();
}

QMenu* DeviceListPage::createContextMenu()
{
    QMenu* menu = new QMenu(this);

    // Create FontAwesome icons
    fa::QtAwesome* awesome = new fa::QtAwesome(this);
    awesome->initFontAwesome();
    QVariantMap options;
    options.insert("color", QColor(255, 255, 255));

    // Rename action
    QAction* renameAction = new QAction(tr("Rename"), menu);
    QIcon renameIcon = awesome->icon(fa::fa_solid, fa::fa_edit, options).pixmap(12, 12);
    renameAction->setIcon(renameIcon);
    connect(renameAction, &QAction::triggered, this, &DeviceListPage::renameDevice);
    menu->addAction(renameAction);

    menu->addSeparator();

    // Toggle top action - dynamic text based on pinned status
    bool isPinned = false;
    if (contextItem)
    {
        DeviceItem* deviceWidget = qobject_cast<DeviceItem*>(listWidget->itemWidget(contextItem));
        if (deviceWidget)
        {
            DeviceInfo device = deviceWidget->deviceInfo();
            isPinned = DeviceManager::instance().isDevicePinned(device.serial);
        }
    }

    QString topActionText = isPinned ? tr("Unpin") : tr("Pin to Top");
    QAction* topAction = new QAction(topActionText, menu);
    QIcon topIcon = awesome->icon(fa::fa_solid, fa::fa_thumbtack, options).pixmap(12, 12);
    topAction->setIcon(topIcon);
    topAction->setCheckable(true);
    topAction->setChecked(isPinned);

    connect(topAction, &QAction::triggered, this, &DeviceListPage::toggleTopDevice);
    menu->addAction(topAction);

    // Apply styling - same as ScreenWindow
    menu->setStyleSheet(R"(
        QMenu {
            background-color: #008D4E;
            border-radius: 6px;
            padding: 4px 0;
            color: white;
        }
        QMenu::item {
            background-color: transparent;
            padding: 4px 0px;
            margin: 2px 4px;
            font-size: 12px;
        }
        QMenu::item:selected {
            background-color: #00804E;
        }
        QMenu::item:disabled {
            color: #6e7681;
        }
        QMenu::separator {
            height: 1px;
            background: white;
            margin: 4px 8px;
        }
        QMenu::icon {
            padding-left: 6px;
        }
    )");

    return menu;
}

void DeviceListPage::renameDevice()
{
    if (!contextItem)
        return;

    DeviceItem* deviceWidget = qobject_cast<DeviceItem*>(listWidget->itemWidget(contextItem));
    if (!deviceWidget)
        return;

    DeviceInfo device = deviceWidget->deviceInfo();
    QString currentName = device.name;

    bool ok;
    QString newName = QInputDialog::getText(
        this, tr("Rename Device"), tr("Enter new name for device:"), QLineEdit::Normal, currentName, &ok);

    if (ok && !newName.trimmed().isEmpty() && newName != currentName)
    {
        bool success = DeviceManager::instance().renameDevice(device.serial, newName);
        if (success)
        {
            qDebug() << "Device renamed successfully from" << currentName << "to" << newName;
        }
        else
        {
            qWarning() << "Failed to rename device:" << device.serial;
        }
    }
}

void DeviceListPage::toggleTopDevice()
{
    if (!contextItem)
        return;

    DeviceItem* deviceWidget = qobject_cast<DeviceItem*>(listWidget->itemWidget(contextItem));
    if (!deviceWidget)
        return;

    DeviceInfo device = deviceWidget->deviceInfo();

    bool success = DeviceManager::instance().toggleTopDevice(device.serial);
    if (success)
    {
        bool isPinned = DeviceManager::instance().isDevicePinned(device.serial);
        qDebug() << "Device pin status toggled successfully:" << device.name << "->"
                 << (isPinned ? "pinned" : "unpinned");
    }
    else
    {
        qWarning() << "Failed to toggle pin status for device:" << device.serial;
    }
}
