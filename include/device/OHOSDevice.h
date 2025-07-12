#pragma once
#include "DeviceProxy.h"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QMutex>
#include <QProcess>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QTemporaryFile>
#include <QtConcurrent/QtConcurrent>

// OpenHarmony device proxy implementation
class OHOSDevice : public DeviceProxy
{
public:
    explicit OHOSDevice(QObject* parent = nullptr);

    QVector<DeviceInfo> queryDevices() override;
    QString deviceModel(const QString& serial) override;
    QString deviceName(const QString& serial) override;
    QSize deviceResolution(const QString& serial) override;
    bool setupDeviceServer(const QString& serial, int forwardPort) override;
    bool startDeviceServer(const QString& serial) override;
    bool stopDeviceServer(const QString& serial) override;
    bool queryDeviceInfo(const QString& serial, DeviceInfo& info) override;
    DeviceType deviceType() const override;

    // Send events
    bool sendEvent(const DeviceInfo& dev, DeviceEvent eventType) override;
    bool sendTouchEvent(const DeviceInfo& dev, QPoint pos) override;
    bool sendTextEvent(const DeviceInfo& dev, const QString& text) override;
    bool sendSwipeEvent(const DeviceInfo& dev, QPoint start, QPoint end, int duration = 300) override;
    bool screenshot(const QString& serial, QByteArray& imageData) override;

    bool supportEvent(DeviceEvent eventType) const override;

private:
    bool isScreenOn(const QString& serial);

    QMutex m_mutex;
    bool m_running = false;
};
