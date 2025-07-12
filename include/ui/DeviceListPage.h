#pragma once
#include "AppConfig.h"
#include "DeviceItem.h"
#include "device/Device.h"

#include <QLabel>
#include <QListWidget>
#include <QMenu>
#include <QVBoxLayout>
#include <QWidget>

class DeviceListPage : public QWidget
{
    Q_OBJECT
public:
    explicit DeviceListPage(QWidget* parent = nullptr);

private slots:
    void updateDeviceList(const QVector<DeviceInfo>& devices);
    void showContextMenu(const QPoint& pos);
    void renameDevice();
    void toggleTopDevice();

private:
    void setupContextMenu();
    QMenu* createContextMenu();

    QListWidget* listWidget;
    QMenu* contextMenu;
    QListWidgetItem* contextItem;
};
