#include "device/DeviceManager.h"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMutex>
#include <QObject>
#include <QProcess>
#include <QStandardPaths>
#include <QThread>
#include <QThreadPool>
#include <QTimer>
#include <QVector>
#include <QtConcurrent/QtConcurrent>
#include <list>

DeviceManager::DeviceManager() : QObject(nullptr), m_timer(nullptr)
{
    // Load persistent data
    loadPersistentData();

    // Only register two platform proxies here, can be extended for more platforms
    m_proxies.append(new AndroidDevice(this));
    m_proxies.append(new OHOSDevice(this));
}

DeviceManager::~DeviceManager()
{
    // Save persistent data before destruction
    savePersistentData();
}

void DeviceManager::loadPersistentData()
{
    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dataDir);
    QString filePath = QDir(dataDir).filePath("device_data.json");

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly))
    {
        qDebug() << "No existing device data file found, starting with empty data";
        return;
    }

    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &error);
    if (error.error != QJsonParseError::NoError)
    {
        qWarning() << "Failed to parse device data file:" << error.errorString();
        return;
    }

    QJsonObject root = doc.object();

    // Load device aliases
    if (root.contains("aliases"))
    {
        QJsonObject aliasesObj = root["aliases"].toObject();
        for (auto it = aliasesObj.begin(); it != aliasesObj.end(); ++it)
        {
            m_deviceAliases[it.key()] = it.value().toString();
        }
        qDebug() << "Loaded" << m_deviceAliases.size() << "device aliases";
    }

    // Load pinned devices
    if (root.contains("pinned"))
    {
        QJsonObject pinnedObj = root["pinned"].toObject();
        for (auto it = pinnedObj.begin(); it != pinnedObj.end(); ++it)
        {
            m_pinnedDevices[it.key()] = it.value().toBool();
        }
        qDebug() << "Loaded" << m_pinnedDevices.size() << "pinned devices";
    }

    // Load device cache
    if (root.contains("cache"))
    {
        QJsonArray cacheArray = root["cache"].toArray();
        for (const QJsonValue& value : cacheArray)
        {
            QJsonObject deviceObj = value.toObject();
            DeviceInfo info;
            info.serial = deviceObj["serial"].toString();
            info.name = deviceObj["name"].toString();
            info.type = static_cast<DeviceType>(deviceObj["type"].toInt());
            info.width = deviceObj["width"].toInt();
            info.height = deviceObj["height"].toInt();
            info.forwardPort = deviceObj["forwardPort"].toInt();
            info.scale = deviceObj["scale"].toDouble(1.0);

            if (m_port < info.forwardPort) {
                m_port = info.forwardPort;
            }
            m_deviceCache.put(info.serial, info);
        }
        qDebug() << "Loaded" << cacheArray.size() << "cached devices";
    }
}

void DeviceManager::savePersistentData()
{
    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dataDir);
    QString filePath = QDir(dataDir).filePath("device_data.json");

    QJsonObject root;

    // Save device aliases
    QJsonObject aliasesObj;
    for (auto it = m_deviceAliases.begin(); it != m_deviceAliases.end(); ++it)
    {
        aliasesObj[it.key()] = it.value();
    }
    root["aliases"] = aliasesObj;

    // Save pinned devices
    QJsonObject pinnedObj;
    for (auto it = m_pinnedDevices.begin(); it != m_pinnedDevices.end(); ++it)
    {
        pinnedObj[it.key()] = it.value();
    }
    root["pinned"] = pinnedObj;

    // Save device cache
    QJsonArray cacheArray;
    for (auto it = m_deviceCache.cache.begin(); it != m_deviceCache.cache.end(); ++it)
    {
        QJsonObject deviceObj;
        const DeviceInfo& info = it.value();
        deviceObj["serial"] = info.serial;
        deviceObj["name"] = info.name;
        deviceObj["type"] = static_cast<int>(info.type);
        deviceObj["width"] = info.width;
        deviceObj["height"] = info.height;
        deviceObj["forwardPort"] = info.forwardPort;
        deviceObj["scale"] = info.scale;

        cacheArray.append(deviceObj);
    }
    root["cache"] = cacheArray;

    QJsonDocument doc(root);
    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly))
    {
        file.write(doc.toJson(QJsonDocument::Indented));
        qDebug() << "Saved device data to" << filePath;
    }
    else
    {
        qWarning() << "Failed to save device data to" << filePath;
    }
}

QVector<DeviceInfo> DeviceManager::devices() const
{
    QMutexLocker locker(&m_mutex);
    return getDevicesInternal();
}

QVector<DeviceInfo> DeviceManager::getDevicesInternal() const
{
    QVector<DeviceInfo> result;

    // Step 1: Start with currently connected devices (m_devices)
    for (const auto& dev : m_devices)
    {
        // Step 2: Get complete DeviceInfo from cache
        auto cached = m_deviceCache.get(dev.serial);
        DeviceInfo deviceInfo = cached ? *cached : dev;

        // Step 3: Apply custom name from aliases if exists
        if (m_deviceAliases.contains(dev.serial))
        {
            deviceInfo.name = m_deviceAliases[dev.serial];
        }

        result.append(deviceInfo);
    }

    // Step 4: Reorder based on pinned status - pinned devices first
    QVector<DeviceInfo> pinnedDevices;
    QVector<DeviceInfo> nonPinnedDevices;

    for (const auto& deviceInfo : result)
    {
        if (m_pinnedDevices.value(deviceInfo.serial, false))
        {
            pinnedDevices.append(deviceInfo);
        }
        else
        {
            nonPinnedDevices.append(deviceInfo);
        }
    }

    // Combine: pinned devices first, then non-pinned devices
    result.clear();
    result.append(pinnedDevices);
    result.append(nonPinnedDevices);

    return result;
}

DeviceInfo* DeviceManager::device(const QString& serial) const
{
    DeviceInfo* deviceInfo = m_deviceCache.get(serial);
    if (deviceInfo && m_deviceAliases.contains(serial))
    {
        deviceInfo->name = m_deviceAliases[serial];
    }

    return deviceInfo;
}

DeviceProxy* DeviceManager::proxyForType(DeviceType type) const
{
    for (DeviceProxy* proxy : m_proxies)
    {
        if (proxy->deviceType() == type)
        {
            return proxy;
        }
    }
    return nullptr;
}

void DeviceManager::scanDevices()
{
    refreshDevices();
}

void DeviceManager::startMonitor()
{
    if (!m_timer)
    {
        m_timer = new QTimer(this);
        connect(m_timer, &QTimer::timeout, this, &DeviceManager::refreshDevicesAsync);
        m_timer->start(3000); // Refresh every 3 seconds
    }
    refreshDevicesAsync();
}

void DeviceManager::stopMonitor()
{
    if (m_timer)
    {
        m_timer->stop();
        m_timer->deleteLater();
        m_timer = nullptr;
    }
}

// New async refresh method
void DeviceManager::refreshDevicesAsync()
{
    // Avoid multiple concurrent refreshes
    static QAtomicInt running = 0;
    if (running.testAndSetOrdered(0, 1))
    {
        QtConcurrent::run([this]() {
            this->refreshDevices();
            running = 0; // or running.storeRelease(0);
        });
    }
}

void DeviceManager::refreshDevices()
{
    QVector<DeviceInfo> newDevices;
    // Aggregate devices from all proxies
    for (DeviceProxy* proxy : m_proxies)
    {
        if (Settings::instance().isProxyEnabled(proxy->deviceType()))
        {
            newDevices += proxy->queryDevices();
        }
    }

    // Device connect/disconnect detection
    QVector<DeviceInfo> devicesToEmit;
    bool devicesChanged = false;
    {
        QMutexLocker locker(&m_mutex);
        for (const DeviceInfo& dev : newDevices)
        {
            if (!m_devices.contains(dev))
            {
                qDebug() << "DeviceManager:: Device connected:" << dev.serial;
                processNewDevice(dev);
                emit deviceConnected(dev);
                devicesChanged = true;
            }
        }
        for (const DeviceInfo& dev : m_devices)
        {
            if (!newDevices.contains(dev))
            {
                qDebug() << "DeviceManager:: Device disconnected:" << dev.serial;
                // Keep custom data (aliases, pinned status, and cache) for reconnection
                // Only remove from active device list, but preserve user preferences

                emit deviceDisconnected(dev);
                devicesChanged = true;
            }
        }
        if (devicesChanged)
        {
            qDebug() << "DeviceManager:: Devices changed, updating list";
            m_devices = newDevices;
            // Get the current device list while holding the lock
            devicesToEmit = this->getDevicesInternal();
            qDebug() << "DeviceManager:: Emitting" << devicesToEmit.size() << "devices";
        }
    }

    // Emit signal outside of the lock to avoid deadlock
    if (devicesChanged)
    {
        emit deviceListChanged(devicesToEmit);
    }
}

void DeviceManager::processNewDevice(const DeviceInfo& dev)
{
    DeviceInfo* cached = m_deviceCache.get(dev.serial);
    int forwardPort = nextForwardPort();
    if (!cached)
    {
        DeviceInfo info;
        DeviceProxy* proxy = proxyForType(dev.type);
        if (proxy && proxy->queryDeviceInfo(dev.serial, info) && proxy->setupDeviceServer(dev.serial, forwardPort))
        {
            info.forwardPort = forwardPort;
            qDebug() << "New device connected: " << info.toString();
            m_deviceCache.put(dev.serial, info);

            // Save persistent data when new device is added to cache
            savePersistentData();
        }
    }
    else
    {
        forwardPort = cached->forwardPort;
        DeviceProxy* proxy = proxyForType(dev.type);
        if (proxy)
        {
            proxy->setupDeviceServer(dev.serial, forwardPort);
        }
    }
}

void DeviceManager::LRUCache::put(const QString& key, DeviceInfo& value)
{
    if (cache.contains(key))
    {
        accessOrder.remove(key);
    }
    else if (cache.size() >= capacity)
    {
        QString oldest = accessOrder.back();
        cache.remove(oldest);
        accessOrder.pop_back();
    }
    cache.insert(key, value);
    accessOrder.push_front(key);
}

DeviceInfo* DeviceManager::LRUCache::get(const QString& key)
{
    // Keep original implementation
    if (cache.contains(key))
    {
        accessOrder.remove(key);
        accessOrder.push_front(key);
        return &cache[key];
    }
    return nullptr;
}

void DeviceManager::LRUCache::remove(const QString& key)
{
    if (cache.contains(key))
    {
        cache.remove(key);
        accessOrder.remove(key);
    }
}

int DeviceManager::nextForwardPort()
{
    QMutexLocker locker(&m_portMutex);
    return ++m_port;
}

bool DeviceManager::renameDevice(const QString& serial, const QString& newName)
{
    QVector<DeviceInfo> devicesToEmit;
    {
        QMutexLocker locker(&m_mutex);

        // Check if device exists
        DeviceInfo* device = m_deviceCache.get(serial);
        if (!device)
        {
            qWarning() << "Device not found for renaming:" << serial;
            return false;
        }

        // Update alias
        if (newName.trimmed().isEmpty())
        {
            m_deviceAliases.remove(serial);
        }
        else
        {
            m_deviceAliases[serial] = newName.trimmed();
        }

        qDebug() << "Device renamed:" << serial << "->" << newName;

        // Get the current device list while holding the lock
        devicesToEmit = this->getDevicesInternal();
    }

    // Emit signal outside of the lock to avoid deadlock
    emit deviceListChanged(devicesToEmit);

    // Save persistent data
    savePersistentData();
    return true;
}

bool DeviceManager::toggleTopDevice(const QString& serial)
{
    QVector<DeviceInfo> devicesToEmit;
    {
        QMutexLocker locker(&m_mutex);

        // Check if device exists
        DeviceInfo* device = m_deviceCache.get(serial);
        if (!device)
        {
            qWarning() << "Device not found for pinning:" << serial;
            return false;
        }

        // Toggle pinned status
        bool currentStatus = m_pinnedDevices.value(serial, false);
        m_pinnedDevices[serial] = !currentStatus;

        qDebug() << "Device pin status toggled:" << serial << "->" << (!currentStatus ? "pinned" : "unpinned");

        // Get the current device list while holding the lock
        devicesToEmit = this->getDevicesInternal();
    }

    // Emit signal outside of the lock to avoid deadlock
    emit deviceListChanged(devicesToEmit);

    // Save persistent data
    savePersistentData();
    return true;
}

bool DeviceManager::isDevicePinned(const QString& serial) const
{
    QMutexLocker locker(&m_mutex);
    return m_pinnedDevices.value(serial, false);
}

void DeviceManager::clearCache()
{
    QMutexLocker locker(&m_mutex);

    // Delete device_data.json file
    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QString deviceDataPath = QDir(dataDir).filePath("device_data.json");
    QFile deviceDataFile(deviceDataPath);
    if (deviceDataFile.exists())
    {
        if (deviceDataFile.remove())
        {
            qDebug() << "Successfully deleted device_data.json";
        }
        else
        {
            qWarning() << "Failed to delete device_data.json";
        }
    }

    // Clear device cache
    m_deviceCache.cache.clear();
    m_deviceCache.accessOrder.clear();

    // Clear device aliases
    m_deviceAliases.clear();

    // Clear pinned devices
    m_pinnedDevices.clear();

    // Clear current devices list
    m_devices.clear();

    // Reset port counter
    m_port = 9413;

    qDebug() << "DeviceManager cache cleared";
}
