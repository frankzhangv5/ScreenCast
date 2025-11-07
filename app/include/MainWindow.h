#pragma once

#include "Context.h"
#include "FramelessWindow.h"
#include "MessageBox.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QtAwesome.h>
#include <device/DeviceInfo.h>
#include <vector>

class MainWindow : public FramelessWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

private slots:
    void onStartMirrorClicked();
    void onSettingsClicked();
    void onDeviceItemClicked(QListWidgetItem* item);
    void onDeviceConnected(const QString& serial);
    void onDeviceDisconnected(const QString& serial);
    void onDeviceListChanged(const QVector<DeviceInfo>& devices);
    // Context menu related slots
    void showContextMenu(const QPoint& pos);
    void renameDevice();
    void toggleTopDevice();
    // Override close event to allow normal window closing behavior
    void closeEvent(QCloseEvent* event) override;

private:
    void setupUI();
    void setupConnections();
    void updateDeviceList();
    void setupContextMenu();    // Set up context menu
    QMenu* createContextMenu(); // Create context menu
    QWidget* createDeviceItem(const DeviceInfo& device, int height = 60);

private:
    QList<DeviceInfo> m_devices;
    QListWidget* m_deviceList;
    QLabel* m_infoLabel;          // Device status information label
    QWidget* m_noDeviceContainer; // Container for no device prompt
    QPushButton* m_refreshButton;
    QVBoxLayout* m_listContainer; // List container
    fa::QtAwesome* m_awesome;
    QListWidgetItem* m_contextItem; // Currently selected item in context menu

    // File log handler
};