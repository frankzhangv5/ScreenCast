#pragma once

#include <QIcon>
#include <QObject>
#include <QString>
#include <QtPlugin>
#include <device/DeviceProxy.h>

/**
 * @brief Device plugin interface
 *
 * Defines the interface that ScreenMirror device plugins must implement.
 * Each plugin is a dynamic link library that implements this interface to support new device types.
 */
class IDevicePlugin
{
public:
    virtual ~IDevicePlugin() = default;

    /**
     * @brief Get plugin name
     * @return Plugin name string
     */
    virtual QString pluginName() const = 0;

    /**
     * @brief Get plugin version
     * @return Version string
     */
    virtual QString pluginVersion() const = 0;

    /**
     * @brief Get plugin icon
     * @return Plugin icon
     */
    virtual QIcon pluginIcon() const = 0;

    /**
     * @brief Create device proxy instance
     * @param parent Parent object
     * @return Device proxy instance pointer
     */
    virtual DeviceProxy* createDeviceProxy(QObject* parent = nullptr) = 0;

    /**
     * @brief Get device type enum value
     * @return Device type enum
     */
    virtual DeviceType deviceType() const = 0;

    /**
     * @brief Check if dependency tools are available
     * @return Whether tools are available
     */
    virtual bool checkDriverAvailable() const = 0;

    /**
     * @brief Get plugin description
     * @return Description string
     */
    virtual QString description() const = 0;

    /**
     * @brief Get plugin author
     * @return Author information string
     */
    virtual QString author() const = 0;

    /**
     * @brief Get driver command name (e.g., adb / hdc)
     */
    virtual QString getDriverName() const = 0;

    /**
     * @brief Get driver program path
     * @return Absolute path to the driver program
     */
    virtual QString getDriverPath() const = 0;
};

#define IDevicePlugin_iid "com.screencast.IDevicePlugin"

Q_DECLARE_INTERFACE(IDevicePlugin, IDevicePlugin_iid)