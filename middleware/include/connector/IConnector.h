#pragma once

#include "device/DeviceInfo.h"

#include <QByteArray>
#include <QObject>
#include <QString>

// Connector interface class
class IConnector : public QObject
{
    Q_OBJECT

public:
    // Connection state enumeration
    enum ConnectionState
    {
        Disconnected = 0, // Not connected
        Connecting = 1,   // Connecting
        Connected = 2,    // Connected
        Reconnecting = 3  // Reconnecting
    };

    // Command type enumeration
    enum CommandType
    {
        CMD_QUERY_DEVICE_INFO = 1,
        CMD_GET_SCREEN_FRAME = 2,
        CMD_START_SCREEN_CAPTURE = 3,
        CMD_STOP_SCREEN_CAPTURE = 4,
        CMD_SEND_CONTROL = 5,
        CMD_EXIT = 6
    };

    // Packet type enumeration
    enum PacketType
    {
        PKT_DEVICE_INFO = 1,
        PKT_SCREEN_FRAME = 2,
        PKT_CONTROL_DATA = 3,
        PKT_ACK = 4,
        PKT_ERROR = 5,
        PKT_UNKNOWN = 255
    };

    explicit IConnector(QObject* parent = nullptr) : QObject(parent) {}
    virtual ~IConnector() = default;

    // Connection related interfaces
    virtual void connectToDevice(const QString& host = "127.0.0.1", quint16 port = 12345) = 0;
    virtual void disconnectFromDevice() = 0;
    virtual ConnectionState getConnectionState() const = 0;
    virtual bool isConnected() const = 0;

    // Data sending interfaces
    virtual void sendCommand(CommandType cmd, const QByteArray& payload = QByteArray()) = 0;
    virtual void sendControlData(const QByteArray& controlData) = 0;
    virtual void queryDeviceInfo() = 0;
    virtual void startScreenCapture() = 0;
    virtual void stopScreenCapture() = 0;

    // Data receiving interfaces
    virtual QByteArray getReceivedData() = 0;
    virtual bool hasDataAvailable() const = 0;

signals:
    // Connection state signals
    void connectionEstablished();                       // 连接成功
    void connectionFailed(const QString& error);        // 连接失败
    void connectionLost(const QString& reason);         // 连接断开
    void connectionStateChanged(ConnectionState state); // 连接状态改变

    // Data receiving signals
    void dataReceived(const QByteArray& data);               // 数据接收
    void deviceInfoReceived(const DeviceInfo& info);         // 设备信息接收
    void screenFrameReceived(const QByteArray& frameData);   // 屏幕帧数据接收
    void controlDataReceived(const QByteArray& controlData); // 控制数据接收

    // Error signals
    void dataTransmissionError(const QString& error); // 数据传输出错
    void errorOccurred(const QString& error);         // 一般错误
};