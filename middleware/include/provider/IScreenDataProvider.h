#pragma once

#include <QByteArray>
#include <QImage>
#include <QObject>
#include <QString>

class IScreenDataProvider : public QObject
{
    Q_OBJECT
public:
    explicit IScreenDataProvider(QObject* parent = nullptr) : QObject(parent) {}
    virtual ~IScreenDataProvider() = default;

    // Start/stop providing data
    virtual void startProvide() = 0;
    virtual void stopProvide() = 0;

signals:
    // Lifecycle
    void onStarted();
    void onStopped();

    // Data and errors
    void errorOccurred(const QString& error);
    void frameDecoded(const QImage& frame);

public slots:
    virtual void handleDataReceived(const QByteArray& data) = 0;
    virtual void handleStartDone() = 0;
    virtual void handleStopDone() = 0;
};
