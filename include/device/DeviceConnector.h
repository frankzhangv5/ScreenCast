#pragma once

#include <QDataStream>
#include <QObject>
#include <QTcpSocket>

struct DeviceInfoFull
{
    QString model;
    QString brand;
    QString manufacturer;
    QString marketName;
    QString osVersion;
    int apiVersion;
    int dpi;
    int screenWidth;
    int screenHeight;
    QString cpuArch;
};

class DeviceConnector : public QObject
{
    Q_OBJECT

public:
    enum CommandType
    {
        CMD_QUERY_DEVICE_INFO = 1,
        CMD_GET_SCREEN_FRAME = 2,
        CMD_START_SCREEN_CAPTURE = 3,
        CMD_STOP_SCREEN_CAPTURE = 4,
        CMD_EXIT = 5
    };

    enum PacketType
    {
        PKT_DEVICE_INFO = 1,
        PKT_SCREEN_FRAME = 2,
        PKT_ACK = 3,
        PKT_ERROR = 4,
        PKT_UNKNOWN = 255
    };

    explicit DeviceConnector(QObject* parent = nullptr);

    void connectToDevice(const QString& host = "127.0.0.1", quint16 port = 12345);
    void disconnectFromDevice();

    void sendCommand(CommandType cmd, const QByteArray& payload = QByteArray());
    void queryDeviceInfo();
    void startScreenCapture();
    void stopScreenCapture();

signals:
    void deviceInfoReceived(const DeviceInfoFull& info);
    void screenFrameReceived(const QByteArray& frameData);
    void errorOccurred(const QString& error);
    void connected();
    void disconnected();

private slots:
    void onSocketConnected();
    void onSocketDisconnected();
    void onReadyRead();
    void onErrorOccurred(QAbstractSocket::SocketError error);

private:
    void processPacket(PacketType type, const QByteArray& data);
    DeviceInfoFull parseDeviceInfo(const QByteArray& data);

    QTcpSocket* m_socket;
    qint32 m_expectedSize = 0;
    PacketType m_currentPacketType;
};
