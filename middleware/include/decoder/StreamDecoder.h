#pragma once

#include <QByteArray>
#include <QImage>
#include <QObject>

// Common decoder type enum shared across the project
enum class DecoderType
{
    NONE,
    OPENH264,
    FFMPEGH264,
};

class StreamDecoder : public QObject
{
    Q_OBJECT
public:
    explicit StreamDecoder(QObject* parent = nullptr) : QObject(parent) {}
    virtual ~StreamDecoder() {}

public slots:
    virtual bool initialize() = 0;
    virtual void decode(const QByteArray& data) = 0;
    virtual void flush() = 0;

signals:
    void frameDecoded(const QImage& frame);
    void errorOccurred(const QString& error);
};