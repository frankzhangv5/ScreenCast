#include "device/DeviceConnector.h"

DeviceConnector::DeviceConnector(QObject* parent) : QObject(parent), m_socket(new QTcpSocket(this))
{
    connect(m_socket, &QTcpSocket::connected, this, &DeviceConnector::onSocketConnected);
    connect(m_socket, &QTcpSocket::disconnected, this, &DeviceConnector::onSocketDisconnected);
    connect(m_socket, &QTcpSocket::readyRead, this, &DeviceConnector::onReadyRead);
    connect(m_socket,
            QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::errorOccurred),
            this,
            &DeviceConnector::onErrorOccurred);
}

void DeviceConnector::connectToDevice(const QString& host, quint16 port)
{
    m_socket->connectToHost(host, port);
}

void DeviceConnector::disconnectFromDevice()
{
    m_socket->disconnectFromHost();
}

void DeviceConnector::sendCommand(CommandType cmd, const QByteArray& payload)
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

void DeviceConnector::onReadyRead()
{
    while (m_socket->bytesAvailable() >= 5)
    {
        if (m_expectedSize == 0)
        {
            // Read packet header
            QDataStream headerStream(m_socket);
            headerStream.setByteOrder(QDataStream::LittleEndian);
            quint8 type;
            quint32 length;
            headerStream >> type >> length;

            m_currentPacketType = static_cast<PacketType>(type);
            m_expectedSize = length;
        }

        if (m_socket->bytesAvailable() < m_expectedSize)
            return;

        // Read packet payload
        QByteArray data = m_socket->read(m_expectedSize);
        m_expectedSize = 0;

        processPacket(m_currentPacketType, data);
    }
}

void DeviceConnector::processPacket(PacketType type, const QByteArray& data)
{
    switch (type)
    {
        case PKT_DEVICE_INFO:
            emit deviceInfoReceived(parseDeviceInfo(data));
            break;
        case PKT_SCREEN_FRAME:
            emit screenFrameReceived(data);
            break;
        case PKT_ERROR:
            emit errorOccurred(QString::fromUtf8(data));
            break;
        default:
            qWarning() << "Unknown packet type:" << type;
    }
}

DeviceInfoFull DeviceConnector::parseDeviceInfo(const QByteArray& data)
{
    QDataStream stream(data);
    stream.setByteOrder(QDataStream::LittleEndian);

    DeviceInfoFull info;

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
    info.brand = QString::fromUtf8(brand).trimmed();
    info.manufacturer = QString::fromUtf8(manufacturer).trimmed();
    info.marketName = QString::fromUtf8(marketName).trimmed();
    info.osVersion = QString::fromUtf8(osVersion).trimmed();
    info.apiVersion = apiVersion;
    info.dpi = dpi;
    info.screenWidth = screenWidth;
    info.screenHeight = screenHeight;
    info.cpuArch = QString::fromUtf8(cpuArch).trimmed();

    return info;
}

void DeviceConnector::onErrorOccurred(QAbstractSocket::SocketError error)
{
    qWarning() << "DeviceConnector:: Socket error:" << error;
    emit errorOccurred(m_socket->errorString());
}

void DeviceConnector::onSocketConnected()
{
    qDebug() << "onSocketConnected to device";
    emit connected();
}

void DeviceConnector::onSocketDisconnected()
{
    qDebug() << "onSocketDisconnected from device";
    emit disconnected();
}

void DeviceConnector::queryDeviceInfo()
{
    sendCommand(CMD_QUERY_DEVICE_INFO);
}

void DeviceConnector::startScreenCapture()
{
    qDebug() << "Start screen capture";
    sendCommand(CMD_START_SCREEN_CAPTURE);
}

void DeviceConnector::stopScreenCapture()
{
    qDebug() << "Stop screen capture";
    sendCommand(CMD_STOP_SCREEN_CAPTURE);
}
