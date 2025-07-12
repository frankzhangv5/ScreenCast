#pragma once

#include "AppConfig.h"
#include "device/Device.h"
#include "ui/DeviceListPage.h"
#include "ui/FramelessWindow.h"
#include "ui/SettingsWindow.h"

#include <QDesktopServices>
#include <QToolBar>
#include <QUrl>
#include <QtAwesome.h>

class MainWindow : public FramelessWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

protected:
    void showEvent(QShowEvent* event) override;
    void closeEvent(QCloseEvent* event) override;

private slots:
    void onDeviceConnected(const DeviceInfo& dev);
    void onDeviceDisconnected(const DeviceInfo& dev);
};
