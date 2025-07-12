#include "device/DeviceInfo.h"

#include <QString>

DeviceInfo::DeviceInfo()
    : type(DeviceType::INVALID), serial(""), name(""), width(0), height(0), forwardPort(0), scale(1.0)
{
}

bool DeviceInfo::operator==(const DeviceInfo& other) const
{
    return type == other.type && serial == other.serial;
}

QString DeviceInfo::toString() const
{
    return QString("type=%1, serial='%2', name='%3', width=%4, height=%5, forwardPort=%6, scale=%7")
        .arg(static_cast<int>(type))
        .arg(serial)
        .arg(name)
        .arg(width)
        .arg(height)
        .arg(forwardPort)
        .arg(scale);
}
