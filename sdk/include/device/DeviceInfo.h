#pragma once

#include <QIcon>
#include <QString>

/**
 * @brief Device type enumeration
 */
enum class DeviceType
{
    Unknown,
    Android,
    OHOS,
    iOS,
    Windows,
    Linux,
    MacOS,
};

/**
 * @brief Device event type
 */
enum class DeviceEvent
{
    INVALID = 0,
    BACK,
    HOME,
    MENU,
    WAKEUP,
    SLEEP,
    ROTATE,
    ROTATE_LOCK,
    UNLOCK,
    SHUTDOWN,
    REBOOT,
    TOUCH,
    KEY,
    TEXT,
    VOLUME_UP,
    VOLUME_DOWN,
    MUTE,
    POWER,
    VOLUME_MUTE,
    SWIPE
};

/**
 * @brief Device information structure
 */
struct DeviceInfo
{
    QString serial;  // Device serial number
    QString model;   // Device model
    QString name;    // Device name
    DeviceType type; // Device type
    int forwardPort; // Port forwarding port
    int width;       // Screen width
    int height;      // Screen height
    int rotation;

    DeviceInfo() : serial(""), model(""), name(""), type(DeviceType::Unknown), forwardPort(0), width(320), height(480)
    {
    }

    bool operator==(const DeviceInfo& other) const { return serial == other.serial; }

    bool operator!=(const DeviceInfo& other) const { return !(*this == other); }

    QString toString() const
    {
        return QString("type=%1, serial='%2', name='%3', width=%4, height=%5, forwardPort=%6, rotation=%7")
            .arg(static_cast<int>(type))
            .arg(serial)
            .arg(name)
            .arg(width)
            .arg(height)
            .arg(forwardPort)
            .arg(rotation);
    }
};