#include "connector/SocketConnector.h"

#include <QDebug>
#include <QHostAddress>

SocketConnector::SocketConnector(QObject* parent)
    : IConnector(parent),
      m_socket(nullptr),
      m_host("127.0.0.1"),
      m_port(12345),
      m_connectionState(Disconnected),
      m_expectedPacketSize(0),
      m_currentPacketType(PKT_UNKNOWN)
{
    m_socket = new QTcpSocket(this);

    // 连接信号槽
    connect(m_socket, &QTcpSocket::connected, this, &SocketConnector::onSocketConnected);
    connect(m_socket, &QTcpSocket::disconnected, this, &SocketConnector::onSocketDisconnected);
    connect(m_socket,
            QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::errorOccurred),
            this,
            &SocketConnector::onSocketError);
    connect(m_socket, &QTcpSocket::readyRead, this, &SocketConnector::onReadyRead);
}

SocketConnector::~SocketConnector()
{
    disconnectFromDevice();
}

void SocketConnector::connectToDevice(const QString& host, quint16 port)
{
    if (m_connectionState == Connected || m_connectionState == Connecting)
    {
        return;
    }

    m_host = host;
    m_port = port;

    updateConnectionState(Connecting);
    m_socket->connectToHost(host, port);
}

void SocketConnector::disconnectFromDevice()
{
    if (m_socket->state() != QAbstractSocket::UnconnectedState)
    {
        m_socket->disconnectFromHost();
    }

    updateConnectionState(Disconnected);
}

IConnector::ConnectionState SocketConnector::getConnectionState() const
{
    return m_connectionState;
}

bool SocketConnector::isConnected() const
{
    return m_connectionState == Connected;
}

void SocketConnector::sendCommand(CommandType cmd, const QByteArray& payload)
{
    if (m_socket->state() != QAbstractSocket::ConnectedState)
    {
        qWarning() << "Socket not connected, cannot send command";
        return;
    }

    QByteArray header;
    QDataStream headerStream(&header, QIODevice::WriteOnly);
    headerStream.setByteOrder(QDataStream::LittleEndian);
    headerStream << static_cast<quint8>(cmd) << static_cast<quint32>(payload.size());

    m_socket->write(header + payload);
}

void SocketConnector::sendControlData(const QByteArray& controlData)
{
    sendCommand(CMD_SEND_CONTROL, controlData);
}

void SocketConnector::queryDeviceInfo()
{
    sendCommand(CMD_QUERY_DEVICE_INFO);
}

void SocketConnector::startScreenCapture()
{
    sendCommand(CMD_START_SCREEN_CAPTURE);
}

void SocketConnector::stopScreenCapture()
{
    sendCommand(CMD_STOP_SCREEN_CAPTURE);
}

QByteArray SocketConnector::getReceivedData()
{
    QMutexLocker locker(&m_dataMutex);
    if (!m_receivedDataQueue.isEmpty())
    {
        return m_receivedDataQueue.dequeue();
    }
    return QByteArray();
}

bool SocketConnector::hasDataAvailable() const
{
    QMutexLocker locker(&m_dataMutex);
    return !m_receivedDataQueue.isEmpty();
}

// 重连相关方法已移除

void SocketConnector::onSocketConnected()
{
    updateConnectionState(Connected);
    emit connectionEstablished();
    qDebug() << "SocketConnector: Connected to" << m_host << ":" << m_port;
}

void SocketConnector::onSocketDisconnected()
{
    updateConnectionState(Disconnected);
    emit connectionLost("Connection disconnected");
    qDebug() << "SocketConnector: Connection disconnected";
}

void SocketConnector::onSocketError(QAbstractSocket::SocketError error)
{
    Q_UNUSED(error);
    QString errorString = m_socket->errorString();
    updateConnectionState(Disconnected);
    emit connectionFailed(errorString);
    emit errorOccurred(errorString);
    qDebug() << "SocketConnector: Connection error" << errorString;
}

void SocketConnector::onReadyRead()
{
    processIncomingData();
}

void SocketConnector::processIncomingData()
{
    while (m_socket->bytesAvailable() >= 5)
    {
        if (m_expectedPacketSize == 0)
        {
            // Read packet header
            QDataStream headerStream(m_socket);
            headerStream.setByteOrder(QDataStream::LittleEndian);
            quint8 type;
            quint32 length;
            headerStream >> type >> length;

            m_currentPacketType = static_cast<PacketType>(type);
            m_expectedPacketSize = length;
        }

        if (m_socket->bytesAvailable() < m_expectedPacketSize)
            return;

        // Read packet payload
        QByteArray data = m_socket->read(m_expectedPacketSize);
        m_expectedPacketSize = 0;

        processPacket(m_currentPacketType, data);
    }
}

void SocketConnector::processPacket(PacketType type, const QByteArray& data)
{
    switch (type)
    {
        case PKT_DEVICE_INFO: {
            DeviceInfo info = parseDeviceInfo(data);
            emit deviceInfoReceived(info);
            break;
        }
        case PKT_SCREEN_FRAME:
            emit screenFrameReceived(data);
            break;
        case PKT_CONTROL_DATA:
            emit controlDataReceived(data);
            break;
        case PKT_ACK:
            // 处理确认包
            break;
        case PKT_ERROR:
            emit dataTransmissionError(QString::fromUtf8(data));
            break;
        default:
            qDebug() << "SocketConnector: Unknown packet type" << type;
            break;
    }

    // 将数据添加到队列
    {
        QMutexLocker locker(&m_dataMutex);
        m_receivedDataQueue.enqueue(data);
    }
    emit dataReceived(data);
}

DeviceInfo SocketConnector::parseDeviceInfo(const QByteArray& data)
{
    QDataStream stream(data);
    stream.setByteOrder(QDataStream::LittleEndian);

    DeviceInfo info;

    // Parsed DeviceInfo defined in server：
    // char model[32], brand[32], os_version[32], int api_version,
    // int dpi, int screen_width, int screen_height, char cpu_arch[16]
    char model[32], brand[32], manufacturer[32], marketName[32], osVersion[32];
    int apiVersion, dpi, screenWidth, screenHeight;
    char cpuArch[16];

    stream.readRawData(model, 32);
    stream.readRawData(brand, 32);
    stream.readRawData(manufacturer, 32);
    stream.readRawData(marketName, 32);
    stream.readRawData(osVersion, 32);
    stream >> apiVersion >> dpi >> screenWidth >> screenHeight;
    stream.readRawData(cpuArch, 16);

    info.model = QString::fromUtf8(model).trimmed();
    info.name = QString::fromUtf8(marketName).trimmed();
    info.width = screenWidth;
    info.height = screenHeight;
    return info;
}

void SocketConnector::updateConnectionState(ConnectionState newState)
{
    if (m_connectionState != newState)
    {
        m_connectionState = newState;
        emit connectionStateChanged(newState);
    }
}
