#include "core/StreamDecoder.h"

StreamDecoder::StreamDecoder(QObject* parent) : QObject(parent)
{
    av_log_set_level(AV_LOG_ERROR);
}

StreamDecoder::~StreamDecoder()
{
    cleanup();
}

bool StreamDecoder::initialize()
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_initialized)
    {
        return true;
    }

    m_parser = av_parser_init(AV_CODEC_ID_H264);
    if (!m_parser)
    {
        qWarning() << "Failed to initialize H264 parser";
        return false;
    }

    return initializeDecoder();
}

bool StreamDecoder::initializeDecoder()
{
    m_codec = avcodec_find_decoder(AV_CODEC_ID_H264);
    if (!m_codec)
    {
        qWarning() << "H264 decoder not found";
        return false;
    }

    m_codecContext = avcodec_alloc_context3(m_codec);
    if (!m_codecContext)
    {
        qWarning() << "Failed to allocate codec context";
        return false;
    }

    m_codecContext->thread_count = 4;

    if (avcodec_open2(m_codecContext, m_codec, nullptr) < 0)
    {
        qWarning() << "Failed to open H264 codec";
        return false;
    }

    m_frame = av_frame_alloc();
    if (!m_frame)
    {
        qWarning() << "Failed to allocate frame";
        return false;
    }

    m_packet = av_packet_alloc();
    if (!m_packet)
    {
        qWarning() << "Failed to allocate packet";
        return false;
    }

    m_initialized = true;
    m_firstFrame = true;
    return true;
}

void StreamDecoder::cleanup()
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_parser)
    {
        av_parser_close(m_parser);
        m_parser = nullptr;
    }

    if (m_codecContext)
    {
        avcodec_free_context(&m_codecContext);
        m_codecContext = nullptr;
    }

    if (m_frame)
    {
        av_frame_free(&m_frame);
        m_frame = nullptr;
    }

    if (m_swsContext)
    {
        sws_freeContext(m_swsContext);
        m_swsContext = nullptr;
    }

    if (m_packet)
    {
        av_packet_free(&m_packet);
        m_packet = nullptr;
    }

    m_initialized = false;
}

void StreamDecoder::reset()
{
    cleanup();
    initialize();
}

void StreamDecoder::decode(const QByteArray& h264Data)
{
    if (!m_initialized && !initialize())
    {
        qWarning() << "Decoder not initialized";
        return;
    }

    std::lock_guard<std::mutex> lock(m_mutex);

    const uint8_t* data = reinterpret_cast<const uint8_t*>(h264Data.constData());
    int size = h264Data.size();

    while (size > 0)
    {
        int bytesUsed = av_parser_parse2(
            m_parser, m_codecContext, &m_packet->data, &m_packet->size, data, size, AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);

        if (bytesUsed < 0)
        {
            qWarning() << "Parser error: " << bytesUsed;
            break;
        }

        data += bytesUsed;
        size -= bytesUsed;

        if (m_packet->size == 0)
        {
            continue;
        }

        int ret = avcodec_send_packet(m_codecContext, m_packet);
        if (ret < 0)
        {
            qWarning() << "Error sending packet to decoder: " << ret;
            continue;
        }

        while (ret >= 0)
        {
            ret = avcodec_receive_frame(m_codecContext, m_frame);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
            {
                break;
            }

            if (ret < 0)
            {
                qWarning() << "Error during decoding: " << ret;
                break;
            }

            // skip first frame（SPS/PPS）
            if (m_firstFrame)
            {
                m_firstFrame = false;
                continue;
            }

            QImage img = convertFrameToImage(m_frame);
            if (!img.isNull())
            {
                emit frameDecoded(img);
            }

            av_frame_unref(m_frame);
        }
    }
}

QImage StreamDecoder::convertFrameToImage(const AVFrame* frame)
{
    if (!frame)
    {
        return QImage();
    }

    int outWidth = frame->width;
    int outHeight = frame->height;

    if (outWidth <= 0 || outHeight <= 0)
    {
        qWarning() << "Invalid frame dimensions: " << outWidth << "x" << outHeight;
        return QImage();
    }

    m_swsContext = sws_getCachedContext(m_swsContext,
                                        frame->width,
                                        frame->height,
                                        static_cast<AVPixelFormat>(frame->format),
                                        outWidth,
                                        outHeight,
                                        AV_PIX_FMT_RGB32,
                                        SWS_BILINEAR,
                                        nullptr,
                                        nullptr,
                                        nullptr);

    if (!m_swsContext)
    {
        qWarning() << "Failed to create sws context";
        return QImage();
    }

    QImage image(outWidth, outHeight, QImage::Format_RGB32);
    uint8_t* dst[] = {image.bits()};
    int dstStride[] = {static_cast<int>(image.bytesPerLine())};

    sws_scale(m_swsContext, frame->data, frame->linesize, 0, frame->height, dst, dstStride);

    return image;
}
