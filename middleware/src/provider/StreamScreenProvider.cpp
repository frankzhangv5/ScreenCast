#include "provider/StreamScreenProvider.h"

#include "connector/ConnectorFactory.h"
#include "decoder/StreamDecoderFactory.h"
#include "device/DeviceInfo.h"
#include "device/DeviceProxy.h"
#include "manager/DeviceManager.h"

#include <QDebug>
#include <QImageReader>
#include <QObject>
#include <QTimer>

StreamScreenProvider::StreamScreenProvider(const DeviceInfo& device, QObject* parent)
    : IScreenDataProvider(parent), m_device(device)
{
    // Create decoder thread
    m_decoderThread = new QThread(this);

    // Create decoder and move to child thread (hold concrete implementation with interface pointer)
    m_decoder = StreamDecoderFactory::create(DecoderType::FFMPEGH264);
    m_decoder->moveToThread(m_decoderThread);

    // Connect signals and slots (interface signals)
    connect(m_decoder, &StreamDecoder::frameDecoded, this, &StreamScreenProvider::handleFrameDecoded);
    connect(m_decoder, &StreamDecoder::errorOccurred, this, &StreamScreenProvider::handleDecoderError);

    // Initialize decoder (in the correct thread)
    QMetaObject::invokeMethod(m_decoder, "initialize", Qt::QueuedConnection);

    // Start thread
    m_decoderThread->start();

    // Create connector through factory
    m_connector = ConnectorFactory::create(ConnectorType::SOCKET, this);

    // 连接IConnector信号
    connect(m_connector.data(), &IConnector::screenFrameReceived, this, &StreamScreenProvider::handleDataReceived);
    connect(m_connector.data(), &IConnector::connectionEstablished, this, &StreamScreenProvider::onConnectorConnected);
    connect(m_connector.data(), &IConnector::errorOccurred, this, &StreamScreenProvider::onConnectorError);
}

StreamScreenProvider::~StreamScreenProvider()
{
    // Stop capturing
    stopCapture();

    // Disconnect all signal connections to connector
    if (m_connector)
    {
        disconnect(
            m_connector.data(), &IConnector::screenFrameReceived, this, &StreamScreenProvider::handleDataReceived);
        disconnect(
            m_connector.data(), &IConnector::connectionEstablished, this, &StreamScreenProvider::onConnectorConnected);
        disconnect(m_connector.data(), &IConnector::errorOccurred, this, &StreamScreenProvider::onConnectorError);

        // Ensure connector stops and disconnects
        m_connector->disconnectFromDevice();
        m_connector.clear(); // Release smart pointer
    }

    // Safely stop decoder thread
    if (m_decoderThread && m_decoderThread->isRunning())
    {
        m_decoderThread->quit();
        m_decoderThread->wait();
    }

    // Clean up decoder resources
    if (m_decoder)
    {
        delete m_decoder;
        m_decoder = nullptr;
    }

    // Clean up thread resources
    if (m_decoderThread)
    {
        delete m_decoderThread;
        m_decoderThread = nullptr;
    }
}

void StreamScreenProvider::startProvide()
{
    startCapture();
}

void StreamScreenProvider::stopProvide()
{
    stopCapture();
}

void StreamScreenProvider::handleFrameDecoded(const QImage& frame)
{
    // Display image in UI thread
    if (!frame.isNull())
    {
        emit frameDecoded(frame);
    }
}

void StreamScreenProvider::handleDecoderError(const QString& error)
{
    qWarning() << "Decoder error:" << error;
    emit errorOccurred(error);
}

void StreamScreenProvider::startCapture()
{
    DeviceProxy* proxy = DeviceManager::instance().proxyForType(m_device.type);
    if (proxy)
    {
        proxy->sendEvent(m_device, DeviceEvent::WAKEUP);
        proxy->startMirrorServer(m_device.serial);
        connect(proxy, &DeviceProxy::serverStarted, this, &StreamScreenProvider::onServerStarted);
        connect(proxy, &DeviceProxy::serverStopped, this, &StreamScreenProvider::onServerStopped);
    }
}

void StreamScreenProvider::stopCapture()
{
    if (m_isCapturing)
    {
        // Before stopping screen capture, flush all frames in decoder buffer
        if (m_decoder && m_decoderThread->isRunning())
        {
            QMetaObject::invokeMethod(m_decoder, "flush", Qt::QueuedConnection);
        }

        m_connector->stopScreenCapture();
        m_connector->disconnectFromDevice();
        m_isCapturing = false;
        emit onStopped();
        DeviceProxy* proxy = DeviceManager::instance().proxyForType(m_device.type);
        if (proxy)
        {
            proxy->stopMirrorServer(m_device.serial);
            // Disconnect signal/slot connections with DeviceProxy to avoid resource leaks and duplicate callbacks
            disconnect(proxy, &DeviceProxy::serverStarted, this, &StreamScreenProvider::onServerStarted);
            disconnect(proxy, &DeviceProxy::serverStopped, this, &StreamScreenProvider::onServerStopped);
        }
    }
}

void StreamScreenProvider::onServerStarted(const QString& serial)
{
    if (serial == m_device.serial)
    {
        qDebug() << "onServerStarted: " << serial;
        // Get device IP address from proxy
        DeviceProxy* proxy = DeviceManager::instance().proxyForType(m_device.type);
        if (proxy)
        {
            QString mirrorServerIp = proxy->getMirrorServerIp(serial);
            if (!mirrorServerIp.isEmpty())
            {
                if (m_connector)
                {
                    m_connector->connectToDevice(mirrorServerIp, m_device.forwardPort);
                }
                return;
            }
        }
        qWarning() << "onServerStarted: " << serial << " mirrorServerIp is empty";
    }
}

void StreamScreenProvider::onServerStopped(const QString& serial)
{
    if (serial == m_device.serial)
    {
        qDebug() << "onServerStopped: " << serial;
    }
}

void StreamScreenProvider::onConnectorError(const QString& msg)
{
    qWarning() << "StreamScreenProvider:: Error occurred:" << msg;
    emit errorOccurred(msg);
}

void StreamScreenProvider::onConnectorConnected()
{
    qDebug() << "onConnectorConnected";

    // Get and display initial screenshot after successful connection
    initFrame();

    m_connector->startScreenCapture();
    m_isCapturing = true;
    emit onStarted();
}

void StreamScreenProvider::handleDataReceived(const QByteArray& h264Data)
{
    if (!m_isCapturing)
    {
        return;
    }

    QMutexLocker locker(&m_decoderMutex);

    if (m_decoder && m_decoderThread->isRunning())
    {
        // Ensure decoding is performed in the decoder thread
        QMetaObject::invokeMethod(m_decoder, "decode", Qt::QueuedConnection, Q_ARG(QByteArray, h264Data));
    }
}

void StreamScreenProvider::handleStartDone()
{
    qDebug() << "StreamScreenProvider::handleStartDone";
    m_isCapturing = true;
    emit onStarted();
}

void StreamScreenProvider::handleStopDone()
{
    qDebug() << "StreamScreenProvider::handleStopDone";
    m_isCapturing = false;
    emit onStopped();
}

void StreamScreenProvider::initFrame()
{
    qDebug() << "StreamScreenProvider::initFrame";
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
            QImage screenshot = QImage::fromData(imageData);
            if (screenshot.isNull())
            {
                qWarning() << "Failed to create image from data";
                return;
            }
            emit frameDecoded(screenshot);
        }
    }
}
