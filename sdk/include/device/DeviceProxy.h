#pragma once

#include "DeviceInfo.h"

#include <QIcon>
#include <QImage>
#include <QObject>
#include <QPoint>
#include <QSize>
#include <QVector>

/**
 * @brief DeviceProxy interface class
 *
 * This is a pure interface class that defines standard behaviors that all device proxies must implement.
 * Any device plugin must fully implement all methods of this interface.
 */
class DeviceProxy : public QObject
{
    Q_OBJECT

public:
    explicit DeviceProxy(QObject* parent = nullptr) : QObject(parent) {}
    virtual ~DeviceProxy() = default;
    /**
     * @brief List available devices
     * @return List of device information
     */
    virtual QVector<DeviceInfo> listDevices() = 0;

    /**
     * @brief Get device model
     * @param serial Device serial number
     * @return Device model string
     */
    virtual QString deviceModel(const QString& serial) = 0;

    /**
     * @brief Get device name
     * @param serial Device serial number
     * @return Device name string
     */
    virtual QString deviceName(const QString& serial) = 0;

    /**
     * @brief Get device resolution
     * @param serial Device serial number
     * @return Device screen size
     */
    virtual QSize deviceResolution(const QString& serial) = 0;

    /**
     * @brief Get device type
     * @return Device type enum value
     */
    virtual DeviceType deviceType() const = 0;

    /**
     * @brief Set up mirror server
     * @param serial Device serial number
     * @param forwardPort Forwarding port
     * @return Whether setup succeeded
     */
    virtual bool setupMirrorServer(const QString& serial, int forwardPort) = 0;

    /**
     * @brief Start mirror server
     * @param serial Device serial number
     * @return Whether startup succeeded
     */
    virtual bool startMirrorServer(const QString& serial) = 0;

    /**
     * @brief Stop mirror server
     * @param serial Device serial number
     * @return Whether stop succeeded
     */
    virtual bool stopMirrorServer(const QString& serial) = 0;

    /**
     * @brief Get mirror server IP address
     * @param serial Device serial number
     * @return Mirror server IP address string, empty if cannot be obtained
     */
    virtual QString getMirrorServerIp(const QString& serial) = 0;

    /**
     * @brief Query device detailed information
     * @param serial Device serial number
     * @param info Returned device information
     * @return Whether information was obtained successfully
     */
    virtual bool queryDeviceInfo(const QString& serial, DeviceInfo& info) = 0;

    /**
     * @brief Send device event
     * @param dev Device information
     * @param eventType Event type
     * @return Whether event was sent successfully
     */
    virtual bool sendEvent(const DeviceInfo& dev, DeviceEvent eventType) = 0;

    /**
     * @brief Send touch event
     * @param dev Device information
     * @param pos Touch position
     * @return Whether event was sent successfully
     */
    virtual bool sendTouchEvent(const DeviceInfo& dev, QPoint pos) = 0;

    /**
     * @brief Send text event
     * @param dev Device information
     * @param text Text content
     * @return Whether event was sent successfully
     */
    virtual bool sendTextEvent(const DeviceInfo& dev, const QString& text) = 0;

    /**
     * @brief Send swipe event
     * @param dev Device information
     * @param start Start position
     * @param end End position
     * @param duration Duration in milliseconds
     * @return Whether event was sent successfully
     */
    virtual bool sendSwipeEvent(const DeviceInfo& dev, QPoint start, QPoint end, int duration = 300) = 0;

    /**
     * @brief Get device screen screenshot
     * @param serial Device serial number
     * @param imageData Returned image data
     * @return Whether screenshot was obtained successfully
     */
    virtual bool screenshot(const QString& serial, QByteArray& imageData) = 0;

    /**
     * @brief Check if specific event type is supported
     * @param eventType Event type
     * @return Whether event type is supported
     */
    virtual bool supportEvent(DeviceEvent eventType) const = 0;

    /**
     * @brief Check if device screen is on
     * @param serial Device serial number
     * @return Whether screen is on
     */
    virtual bool isScreenOn(const QString& serial) = 0;

    /**
     * @brief Get device rotation angle
     * @param serial Device serial number
     * @return Device rotation angle (0, 90, 180, 270 degrees)
     */
    virtual int deviceRotation(const QString& serial) = 0;

signals:
    /**
     * @brief Server started signal
     * @param serial Device serial number
     */
    void serverStarted(const QString& serial);

    /**
     * @brief Server stopped signal
     * @param serial Device serial number
     */
    void serverStopped(const QString& serial);
};