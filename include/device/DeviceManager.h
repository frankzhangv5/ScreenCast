#pragma once
#include "AndroidDevice.h"
#include "DeviceInfo.h"
#include "DeviceProxy.h"
#include "OHOSDevice.h"
#include "core/Settings.h"

#include <QMutex>
#include <QObject>
#include <QProcess>
#include <QThread>
#include <QThreadPool>
#include <QTimer>
#include <QVector>
#include <QtConcurrent/QtConcurrent>
#include <list>

class DeviceManager : public QObject
{
    Q_OBJECT

public:
    static DeviceManager& instance()
    {
        static DeviceManager inst;
        return inst;
    }

    // Get current device list
    QVector<DeviceInfo> devices() const;
    DeviceInfo* device(const QString& serial) const;
    DeviceProxy* proxyForType(DeviceType type) const;
    void scanDevices();

    // Device management functions
    bool renameDevice(const QString& serial, const QString& newName);
    bool toggleTopDevice(const QString& serial);
    bool isDevicePinned(const QString& serial) const;

    // Cache management
    void clearCache();

signals:
    void deviceConnected(const DeviceInfo& info);
    void deviceDisconnected(const DeviceInfo& info);
    void deviceListChanged(const QVector<DeviceInfo>& devices);

public slots:
    void startMonitor();
    void stopMonitor();

private:
    DeviceManager();
    ~DeviceManager();
    void refreshDevicesAsync();
    void refreshDevices();
    void processNewDevice(const DeviceInfo& dev);
    int nextForwardPort();
    QVector<DeviceInfo> getDevicesInternal() const;
    void loadPersistentData();
    void savePersistentData();

    struct LRUCache
    {
        QHash<QString, DeviceInfo> cache;
        std::list<QString> accessOrder;
        int capacity {100};

        void put(const QString& key, DeviceInfo& value);
        DeviceInfo* get(const QString& key);
        void remove(const QString& key);
    };

    int m_port = 9413;
    mutable LRUCache m_deviceCache;
    QVector<DeviceInfo> m_devices;
    QTimer* m_timer;
    mutable QMutex m_mutex;
    mutable QMutex m_portMutex;
    QVector<DeviceProxy*> m_proxies;

    // Device customization data
    QHash<QString, QString> m_deviceAliases; // serial -> custom name
    QHash<QString, bool> m_pinnedDevices;    // serial -> pinned status
};
