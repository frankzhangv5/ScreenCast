#pragma once
#include "StreamDecoder.h"
#include "device/Device.h"

#include <QImage>
#include <QMutex>
#include <QObject>

class StreamReader : public QObject
{
    Q_OBJECT
public:
    explicit StreamReader(const DeviceInfo& deviceInfo, QObject* parent = nullptr);

public slots:
    void stopCapture();
    void startCapture();

signals:
    void frameDecoded(const QImage& frame);
    void errorOccurred(const QString& msg);

private slots:
    void handleDataReceived(const QByteArray& frameData);
    void onConnectorConnected();
    void onFrameDecoded(const QImage& frame);
    void onServerStarted(const QString& serial);
    void onServerStopped(const QString& serial);
    void onErrorOccurred(const QString& msg);

private:
    void initializeDecoder();
    void initFrame();

    DeviceInfo m_device;
    DeviceConnector m_connector;
    QMutex m_decoderMutex;
    bool m_isCapturing = false;
    StreamDecoder* m_decoder;
};
