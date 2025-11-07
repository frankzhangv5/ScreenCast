#pragma once
#include "IScreenDataProvider.h"
#include "connector/ConnectorFactory.h"
#include "device/DeviceInfo.h"

#include <QImage>
#include <QMutex>
#include <QObject>
#include <QSharedPointer>
#include <QThread>
#include <decoder/StreamDecoder.h>

class StreamScreenProvider : public IScreenDataProvider
{
    Q_OBJECT
public:
    explicit StreamScreenProvider(const DeviceInfo& deviceInfo, QObject* parent = nullptr);
    ~StreamScreenProvider();

    // IScreenDataProvider 接口
    void startProvide() override;

public slots:
    void stopProvide() override;
    void stopCapture();
    void startCapture();

signals:
    void frameDecoded(const QImage& frame);
    void errorOccurred(const QString& msg);

    // IScreenDataProvider 信号
    void onStarted();
    void onStopped();

public slots:
    void handleDataReceived(const QByteArray& data);
    void handleStartDone();
    void handleStopDone();

private slots:
    void onConnectorConnected();

    void onServerStarted(const QString& serial);
    void onServerStopped(const QString& serial);
    void onConnectorError(const QString& msg);
    void handleFrameDecoded(const QImage& frame);
    void handleDecoderError(const QString& error);

private:
    void initializeDecoder();
    void initFrame();

    DeviceInfo m_device;
    QSharedPointer<IConnector> m_connector;
    QMutex m_decoderMutex;
    bool m_isCapturing = false;

    StreamDecoder* m_decoder = nullptr;
    QThread* m_decoderThread = nullptr;
};
