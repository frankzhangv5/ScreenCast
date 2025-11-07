#pragma once

#include <QMutex>
#include <device/DeviceInfo.h>
#include <device/DeviceProxy.h>

/**
 * @brief Android device proxy implementation
 *
 * Provides connection, control, and screenshot functionality for Android devices.
 * Communicates with Android devices through ADB.
 */
class AndroidDevice : public DeviceProxy
{
    Q_OBJECT

public:
    explicit AndroidDevice(QObject* parent = nullptr);

    // DeviceProxy interface implementation
    QVector<DeviceInfo> listDevices() override;
    QString deviceModel(const QString& serial) override;
    QString deviceName(const QString& serial) override;
    QSize deviceResolution(const QString& serial) override;
    DeviceType deviceType() const override;
    bool queryDeviceInfo(const QString& serial, DeviceInfo& info) override;

    // Mirror server related
    bool setupMirrorServer(const QString& serial, int forwardPort) override;
    bool startMirrorServer(const QString& serial) override;
    bool stopMirrorServer(const QString& serial) override;
    QString getMirrorServerIp(const QString& serial) override;

    // Screen related
    bool isScreenOn(const QString& serial) override;

    // Get device rotation angle
    int deviceRotation(const QString& serial) override;

    // Event sending related
    bool sendEvent(const DeviceInfo& dev, DeviceEvent eventType) override;
    bool sendTouchEvent(const DeviceInfo& dev, QPoint pos) override;
    bool sendTextEvent(const DeviceInfo& dev, const QString& text) override;
    bool sendSwipeEvent(const DeviceInfo& dev, QPoint start, QPoint end, int duration = 300) override;

    // Screenshot related
    bool screenshot(const QString& serial, QByteArray& imageData) override;

    // Function support checking
    bool supportEvent(DeviceEvent eventType) const override;

private:
    bool pushResourceToDevice(const QString& serial, const QString& resourcePath, const QString& devicePath);

    bool m_running = false;
    QMutex m_mutex;
};