#pragma once

#include <device/DeviceInfo.h>
#include <device/DeviceProxy.h>
#include <manager/DeviceManager.h>
#include <manager/PluginManager.h>

/**
 * @brief Application context class
 *
 * Provides access to core application components, encapsulating access to DeviceManager, PluginManager and other core
 * managers. Other window classes can inherit from this class to simplify calls to core components.
 */
class Context
{
public:
    /**
     * @brief Constructor
     */
    Context() = default;

    /**
     * @brief Destructor
     */
    virtual ~Context() = default;

    /**
     * @brief Get device manager instance
     * @return DeviceManager reference
     */
    DeviceManager& getDeviceManager();

    /**
     * @brief Get plugin manager instance
     * @return PluginManager reference
     */
    PluginManager& getPluginManager();

    /**
     * @brief Get device proxy for specified device type
     * @param type Device type
     * @return DeviceProxy pointer, nullptr if not found
     */
    DeviceProxy* getDeviceProxy(DeviceType type);

    /**
     * @brief Get all supported device types
     * @return List of device types
     */
    QList<DeviceType> getSupportedDeviceTypes();

    /**
     * @brief Check if device type is supported
     * @param type Device type
     * @return Whether supported
     */
    bool isDeviceTypeSupported(DeviceType type);

    /**
     * @brief Get device type icon
     * @param type Device type
     * @return Device type icon
     */
    QIcon getDeviceTypeIcon(DeviceType type);

    /**
     * @brief Get specified device information
     * @param serial Device serial number
     * @return Device information pointer, nullptr if not found
     */
    DeviceInfo* getDeviceInfo(const QString& serial);

    /**
     * @brief Get current device list
     * @return List of device information
     */
    QVector<DeviceInfo> getDeviceList();

    /**
     * @brief Scan devices
     */
    void scanDevices();

    /**
     * @brief Start device monitoring
     */
    void startDeviceMonitor();

    /**
     * @brief Stop device monitoring
     */
    void stopDeviceMonitor();

    /**
     * @brief Prevent copy construction
     */
    Context(const Context&) = delete;

    /**
     * @brief Prevent assignment operation
     */
    Context& operator=(const Context&) = delete;
};