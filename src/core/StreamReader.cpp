#include "core/StreamReader.h"

#include "device/Device.h"

#include <QDebug>
#include <QImageReader>
#include <QObject>
#include <QTimer>

StreamReader::StreamReader(const DeviceInfo& device, QObject* parent)
    : QObject(parent), m_device(device), m_decoder(new StreamDecoder(this))
{
    connect(&m_connector, &DeviceConnector::screenFrameReceived, this, &StreamReader::handleDataReceived);
    connect(&m_connector, &DeviceConnector::connected, this, &StreamReader::onConnectorConnected);
    connect(m_decoder, &StreamDecoder::frameDecoded, this, &StreamReader::onFrameDecoded);
    connect(&m_connector, &DeviceConnector::errorOccurred, this, &StreamReader::onErrorOccurred);
}

void StreamReader::startCapture()
{
    DeviceProxy* proxy = DeviceManager::instance().proxyForType(m_device.type);
    if (proxy)
    {
        proxy->sendEvent(m_device, DeviceEvent::WAKEUP);
        proxy->startDeviceServer(m_device.serial);
        connect(proxy, &DeviceProxy::serverStarted, this, &StreamReader::onServerStarted);
        connect(proxy, &DeviceProxy::serverStopped, this, &StreamReader::onServerStopped);
    }
}

void StreamReader::stopCapture()
{
    if (m_isCapturing)
    {
        m_connector.stopScreenCapture();
        m_connector.disconnectFromDevice();
        m_isCapturing = false;
        DeviceProxy* proxy = DeviceManager::instance().proxyForType(m_device.type);
        if (proxy)
        {
            proxy->stopDeviceServer(m_device.serial);
        }
    }
}

void StreamReader::onServerStarted(const QString& serial)
{
    if (serial == m_device.serial)
    {
        qDebug() << "onServerStarted: " << serial;
        initFrame();
        m_connector.connectToDevice("127.0.0.1", m_device.forwardPort);
    }
}

void StreamReader::onServerStopped(const QString& serial)
{
    if (serial == m_device.serial)
    {
        qDebug() << "onServerStopped: " << serial;
    }
}

void StreamReader::onErrorOccurred(const QString& msg)
{
    qWarning() << "StreamReader:: Error occurred:" << msg;
    emit errorOccurred(msg);
}

void StreamReader::onConnectorConnected()
{
    qDebug() << "onConnectorConnected";
    m_connector.startScreenCapture();
    m_isCapturing = true;
}

void StreamReader::onFrameDecoded(const QImage& frame)
{
    emit frameDecoded(frame);
}

void StreamReader::handleDataReceived(const QByteArray& h264Data)
{
    if (!m_isCapturing)
    {
        return;
    }
    QMutexLocker locker(&m_decoderMutex);
    QMetaObject::invokeMethod(m_decoder, "decode", Qt::QueuedConnection, Q_ARG(QByteArray, h264Data));
}

void StreamReader::initFrame()
{
    DeviceProxy* proxy = DeviceManager::instance().proxyForType(m_device.type);
    if (proxy)
    {
        QByteArray imageData;
        if (proxy->screenshot(m_device.serial, imageData))
        {
            // Check for empty image data
            if (imageData.isEmpty())
            {
                qWarning() << "Received empty image data from device";
                return;
            }

            emit frameDecoded(QImage::fromData(imageData));
        }
    }
}
