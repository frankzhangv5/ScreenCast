#include "Context.h"

// Context class implementation file

DeviceManager& Context::getDeviceManager()
{
    return DeviceManager::instance();
}

PluginManager& Context::getPluginManager()
{
    return PluginManager::instance();
}

DeviceProxy* Context::getDeviceProxy(DeviceType type)
{
    return getDeviceManager().proxyForType(type);
}

QList<DeviceType> Context::getSupportedDeviceTypes()
{
    return getPluginManager().getSupportedDeviceTypes();
}

bool Context::isDeviceTypeSupported(DeviceType type)
{
    return getDeviceManager().isDeviceTypeSupported(type);
}

QIcon Context::getDeviceTypeIcon(DeviceType type)
{
    return getDeviceManager().getDeviceTypeIcon(type);
}

DeviceInfo* Context::getDeviceInfo(const QString& serial)
{
    return getDeviceManager().device(serial);
}

QVector<DeviceInfo> Context::getDeviceList()
{
    return getDeviceManager().devices();
}

void Context::scanDevices()
{
    getDeviceManager().scanDevices();
}

void Context::startDeviceMonitor()
{
    getDeviceManager().startMonitor();
}

void Context::stopDeviceMonitor()
{
    getDeviceManager().stopMonitor();
}