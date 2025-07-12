#pragma once
#include "device/Device.h"
#include "ui/ScreenWindow.h"

#include <QApplication>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

class DeviceItem : public QWidget
{
    Q_OBJECT
public:
    explicit DeviceItem(const DeviceInfo& dev, QWidget* parent = nullptr);

    // Get icon based on device type
    static QIcon iconForType(DeviceType type);

    // Get device info
    const DeviceInfo& deviceInfo() const { return m_info; }

private:
    DeviceInfo m_info; // Device info member
};
