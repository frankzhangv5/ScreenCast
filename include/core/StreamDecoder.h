#pragma once
#include <QDebug>
#include <QImage>
#include <QObject>
#include <mutex>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

class StreamDecoder : public QObject
{
    Q_OBJECT
public:
    explicit StreamDecoder(QObject* parent = nullptr);
    ~StreamDecoder();

    bool initialize();
    void reset();
    bool isInitialized() const { return m_initialized; }

public slots:
    void decode(const QByteArray& h264Data);

signals:
    void frameDecoded(const QImage& image);

private:
    bool initializeDecoder();
    void cleanup();
    QImage convertFrameToImage(const AVFrame* frame);

    const AVCodec* m_codec = nullptr;
    AVCodecParserContext* m_parser = nullptr;
    AVCodecContext* m_codecContext = nullptr;
    AVFrame* m_frame = nullptr;
    SwsContext* m_swsContext = nullptr;
    AVPacket* m_packet = nullptr;
    std::mutex m_mutex;
    bool m_initialized = false;
    bool m_firstFrame = true;
};
