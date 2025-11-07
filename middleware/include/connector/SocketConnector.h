#pragma once

#include "IConnector.h"

#include <QDataStream>
#include <QMutex>
#include <QQueue>
#include <QTcpSocket>

class SocketConnector : public IConnector
{
    Q_OBJECT

public:
    explicit SocketConnector(QObject* parent = nullptr);
    virtual ~SocketConnector();

    // 实现IConnector接口
    void connectToDevice(const QString& host = "127.0.0.1", quint16 port = 12345) override;
    void disconnectFromDevice() override;
    ConnectionState getConnectionState() const override;
    bool isConnected() const override;

    // 数据发送接口
    void sendCommand(CommandType cmd, const QByteArray& payload = QByteArray()) override;
    void sendControlData(const QByteArray& controlData) override;
    void queryDeviceInfo() override;
    void startScreenCapture() override;
    void stopScreenCapture() override;

    // 数据接收接口
    QByteArray getReceivedData() override;
    bool hasDataAvailable() const override;

private slots:
    void onSocketConnected();
    void onSocketDisconnected();
    void onSocketError(QAbstractSocket::SocketError error);
    void onReadyRead();

private:
    // 数据包处理
    void processIncomingData();
    void processPacket(PacketType type, const QByteArray& data);
    DeviceInfo parseDeviceInfo(const QByteArray& data);

    // 连接管理
    void updateConnectionState(ConnectionState newState);

private:
    QTcpSocket* m_socket;
    mutable QMutex m_dataMutex;
    QQueue<QByteArray> m_receivedDataQueue;

    // 连接配置
    QString m_host;
    quint16 m_port;
    ConnectionState m_connectionState;

    // 数据包解析状态
    qint32 m_expectedPacketSize;
    PacketType m_currentPacketType;
};
