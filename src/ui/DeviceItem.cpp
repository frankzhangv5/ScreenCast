#include "ui/DeviceItem.h"

#include <QApplication>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

DeviceItem::DeviceItem(const DeviceInfo& dev, QWidget* parent) : QWidget(parent), m_info(dev)
{
    QHBoxLayout* itemLayout = new QHBoxLayout(this);
    itemLayout->setContentsMargins(4, 2, 4, 2);
    itemLayout->setSpacing(5);

    // Device icon
    QLabel* iconLabel = new QLabel(this);
    iconLabel->setPixmap(iconForType(dev.type).pixmap(32, 32));
    iconLabel->setFixedSize(32, 32);
    iconLabel->setStyleSheet("background: none; border-radius: 16px; border: none;");
    iconLabel->setAlignment(Qt::AlignCenter);
    itemLayout->addWidget(iconLabel);

    // Device name and serial number
    QVBoxLayout* infoLayout = new QVBoxLayout();
    QLabel* nameLabel = new QLabel(dev.name, this);
    nameLabel->setStyleSheet("background: none; color: white; font: bold 14px; padding: 2px; border: none;");
    QLabel* serialLabel = new QLabel(dev.serial, this);
    serialLabel->setStyleSheet("background: none; color: white; font: 12px; padding: 2px; border: none;");
    infoLayout->addWidget(nameLabel);
    infoLayout->addWidget(serialLabel);
    itemLayout->addLayout(infoLayout);

    itemLayout->addStretch();

    // Start button
    QPushButton* startBtn = new QPushButton(QIcon(":icons/play"), "", this);
    startBtn->setIconSize(QSize(32, 32));
    startBtn->setFixedSize(32, 32);
    startBtn->setStyleSheet(R"(
        QPushButton {
            background-color: white;
            border: none;
            border-radius: 16px;
        }
        QPushButton:hover {
            background: rgba(255,255,255,0.8)
        }
        QPushButton:pressed {
            background: rgba(255,255,255,0.9)
        }
    )");
    itemLayout->addWidget(startBtn);

    // Connect button click signal
    connect(startBtn, &QPushButton::clicked, this, [this]() {
        ScreenWindow* win = new ScreenWindow(m_info);
        win->setAttribute(Qt::WA_DeleteOnClose); // Auto delete when closed
        win->show();

        // Center display on main screen
        QRect screenRect = QApplication::primaryScreen()->availableGeometry();
        int x = screenRect.x() + (screenRect.width() - win->width()) / 2;
        int y = screenRect.y() + (screenRect.height() - win->height()) / 2;
        win->move(x, y);
    });

    setLayout(itemLayout);
}

// New: Get icon based on device type
QIcon DeviceItem::iconForType(DeviceType type)
{
    switch (type)
    {
        case DeviceType::Android:
            return QIcon(":/icons/android");
        case DeviceType::OHOS:
            return QIcon(":/icons/ohos");
        default:
            return QIcon();
    }
}
