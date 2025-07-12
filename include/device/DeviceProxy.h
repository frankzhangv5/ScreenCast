#pragma once
#include "DeviceInfo.h"

#include <QDebug>
#include <QObject>
#include <QPoint>
#include <QProcess>
#include <QSize>
#include <QString>
#include <QVector>

enum DeviceEvent
{
    INVALID = 0,
    BACK,
    HOME,
    MENU,
    WAKEUP,
    SLEEP,
    ROTATE,
    UNLOCK,
    SHUTDOWN,
    REBOOT
};

class DeviceProxy : public QObject
{
    Q_OBJECT
public:
    explicit DeviceProxy(QObject* parent = nullptr);
    virtual ~DeviceProxy() = default;

    // Query device list
    virtual QVector<DeviceInfo> queryDevices() = 0;
    virtual QString deviceModel(const QString& serial) = 0;
    virtual QString deviceName(const QString& serial) = 0;
    virtual QSize deviceResolution(const QString& serial) = 0;
    virtual DeviceType deviceType() const = 0;

    // Connect device
    virtual bool setupDeviceServer(const QString& serial, int forwardPort) = 0;
    virtual bool startDeviceServer(const QString& serial) = 0;
    virtual bool stopDeviceServer(const QString& serial) = 0;
    virtual bool queryDeviceInfo(const QString& serial, DeviceInfo& info) = 0;

    // Send events
    virtual bool sendEvent(const DeviceInfo& dev, DeviceEvent eventType) = 0;
    // Send coordinate events
    virtual bool sendTouchEvent(const DeviceInfo& dev, QPoint pos) = 0;
    // Send text events
    virtual bool sendTextEvent(const DeviceInfo& dev, const QString& text) = 0;
    // Send swipe events
    virtual bool sendSwipeEvent(const DeviceInfo& dev, QPoint start, QPoint end, int duration = 300) = 0;

    virtual bool screenshot(const QString& serial, QByteArray& imageData) = 0;

    virtual bool supportEvent(DeviceEvent eventType) const = 0;

signals:
    void serverStarted(const QString& serial);
    void serverStopped(const QString& serial);

protected:
    // Execute command and return standard output text
    static QString execCommand(const QString& program, const QStringList& arguments, int timeout = 100000);
    qreal getScale(QSize resolution, int max_size = 800) const;
    QPoint scalePoint(QPoint pos) const;
    QSize scaleSize(QSize resolution) const;
    QSize restoreSize(QSize scaledSize, qreal scale) const;
    QPoint restorePoint(QPoint scaledPoint, qreal scale) const;
};
