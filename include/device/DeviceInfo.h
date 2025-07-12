#pragma once
#include <QString>

// Device type enumeration
enum class DeviceType
{
    INVALID = 0,
    Android,
    OHOS
};

// Device information structure
struct DeviceInfo
{
    DeviceInfo();
    DeviceType type;
    QString name;
    QString serial;
    int width;
    int height;
    int forwardPort;
    qreal scale;

    bool operator==(const DeviceInfo& other) const;
    QString toString() const;
};
