#include "decoder/FfmpegDecoder.h"

#include <QCoreApplication>
#include <QDebug>
#include <QElapsedTimer>
#include <QScopedPointer>
#include <QString>
#include <QThread>

// 辅助宏用于错误处理和日志
#define LOG_INFO(msg)  qDebug() << "[FfmpegDecoder]" << msg
#define LOG_ERROR(msg) qWarning() << "[FfmpegDecoder] Error:" << msg
#define LOG_DEBUG(msg) qDebug() << "[FfmpegDecoder] Debug:" << msg
#define RETURN_IF_ERROR(condition, errorMsg)                                                                           \
    do                                                                                                                 \
    {                                                                                                                  \
        if (condition)                                                                                                 \
        {                                                                                                              \
            LOG_ERROR(errorMsg);                                                                                       \
            emit errorOccurred(errorMsg);                                                                              \
            return false;                                                                                              \
        }                                                                                                              \
    } while (false)
#define BREAK_IF_ERROR(condition, errorMsg)                                                                            \
    do                                                                                                                 \
    {                                                                                                                  \
        if (condition)                                                                                                 \
        {                                                                                                              \
            LOG_ERROR(errorMsg);                                                                                       \
            emit errorOccurred(errorMsg);                                                                              \
            break;                                                                                                     \
        }                                                                                                              \
    } while (false)
#define RETURN_IF_NOT_INITIALIZED()                                                                                    \
    do                                                                                                                 \
    {                                                                                                                  \
        if (!m_initialized)                                                                                            \
        {                                                                                                              \
            LOG_ERROR("Decoder not initialized");                                                                      \
            emit errorOccurred("Decoder not initialized");                                                             \
            return;                                                                                                    \
        }                                                                                                              \
    } while (false)

// FFmpeg headers
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
}

FfmpegDecoder::FfmpegDecoder(QObject* parent)
    : StreamDecoder(parent),
      m_codecContext(nullptr),
      m_frame(nullptr),
      m_packet(nullptr),
      m_codec(nullptr),
      m_parser(nullptr),
      m_initialized(false),
      m_swsContext(nullptr),
      m_lastWidth(-1),
      m_lastHeight(-1),
      m_lastSrcFormat(AV_PIX_FMT_NONE),
      m_lastDstFormat(AV_PIX_FMT_NONE),
      m_rgbFrame(nullptr),
      m_rgbBuffer(nullptr)
{
}

FfmpegDecoder::~FfmpegDecoder()
{
    cleanup();
}

bool FfmpegDecoder::initialize()
{
    if (m_initialized)
    {
        LOG_INFO("Already initialized");
        return true;
    }

    LOG_INFO("Initializing FFmpeg decoder...");

    // Clean up any existing resources
    cleanup();

    // Find the H264 codec
    m_codec = const_cast<AVCodec*>(avcodec_find_decoder(AV_CODEC_ID_H264));
    RETURN_IF_ERROR(!m_codec, "Could not find H264 codec");

    LOG_INFO(QString("Found codec: %1").arg(m_codec->name));

    // Create H.264 parser
    m_parser = av_parser_init(AV_CODEC_ID_H264);
    RETURN_IF_ERROR(!m_parser, "Could not initialize H.264 parser");
    LOG_INFO("H.264 parser initialized");

    // Allocate codec context
    m_codecContext = avcodec_alloc_context3(m_codec);
    RETURN_IF_ERROR(!m_codecContext, "Could not allocate codec context");

    // Thread configuration optimization
    int threadCount = qMax(1, QThread::idealThreadCount() - 1);
    m_codecContext->thread_count = threadCount;
    m_codecContext->thread_type = FF_THREAD_FRAME | FF_THREAD_SLICE;

    // Enable fast decoding
    m_codecContext->flags2 |= AV_CODEC_FLAG2_FAST;

    // Set optimized decoding parameters
    // Enable low latency decoding
    m_codecContext->flags |= AV_CODEC_FLAG_LOW_DELAY;

    // Set maximum cached frames to 1 to reduce latency
    av_opt_set_int(m_codecContext, "refcounted_frames", 1, 0);

    // Disable B-frames to reduce latency
    av_opt_set_int(m_codecContext, "tune", 2, 0); // tune=zerolatency
    LOG_INFO(QString("Configured with %1 threads, fast decoding enabled").arg(threadCount));

    // Open codec
    int ret = avcodec_open2(m_codecContext, m_codec, nullptr);
    RETURN_IF_ERROR(ret < 0, QString("Could not open codec: %1").arg(ret));

    // Allocate frame and packet with error checking
    m_frame = av_frame_alloc();
    RETURN_IF_ERROR(!m_frame, "Could not allocate frame");

    m_packet = av_packet_alloc();
    RETURN_IF_ERROR(!m_packet, "Could not allocate packet");

    m_initialized = true;
    LOG_INFO("FFmpeg decoder initialized successfully");
    return true;
}

void FfmpegDecoder::decode(const QByteArray& data)
{
    RETURN_IF_NOT_INITIALIZED();

    // Check for empty data
    if (data.isEmpty())
    {
        LOG_ERROR("Empty data received for decoding");
        return;
    }

    const uint8_t* buffer = reinterpret_cast<const uint8_t*>(data.constData());
    int bufferSize = data.size();
    int remainingSize = bufferSize;
    int offset = 0;

    // Parse H.264 data using av_parser_parse2
    while (remainingSize > 0)
    {
        // Parse data
        int parsedSize = av_parser_parse2(m_parser,
                                          m_codecContext,
                                          &m_packet->data,
                                          &m_packet->size,
                                          buffer + offset,
                                          remainingSize,
                                          AV_NOPTS_VALUE,
                                          AV_NOPTS_VALUE,
                                          0);

        if (parsedSize < 0)
        {
            LOG_ERROR(QString("Error parsing H.264 data: %1").arg(parsedSize));
            emit errorOccurred(QString("Error parsing H.264 data: %1").arg(parsedSize));
            break;
        }

        // Update offset and remaining size
        offset += parsedSize;
        remainingSize -= parsedSize;

        // Decode if complete packet is parsed
        if (m_packet->size > 0)
        {
            // Send packet to decoder
            int ret = avcodec_send_packet(m_codecContext, m_packet);
            if (ret < 0)
            {
                // Some errors can be ignored, such as partial frame data
                if (ret != AVERROR(EAGAIN))
                {
                    LOG_ERROR(QString("Error sending packet to decoder: %1").arg(ret));
                    emit errorOccurred(QString("Error sending packet to decoder: %1").arg(ret));
                }
                // Continue with next packet
                av_packet_unref(m_packet);
                continue;
            }

            // Receive decoded frames
            while (true)
            {
                ret = avcodec_receive_frame(m_codecContext, m_frame);
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                {
                    break;
                }
                else if (ret < 0)
                {
                    // More detailed error information
                    char errbuf[AV_ERROR_MAX_STRING_SIZE];
                    av_strerror(ret, errbuf, sizeof(errbuf));
                    LOG_ERROR(QString("Error receiving frame from decoder: %1 (%2)").arg(ret).arg(errbuf));
                    emit errorOccurred(QString("Error receiving frame from decoder: %1").arg(errbuf));
                    break;
                }

                // Convert frame to QImage and emit signal
                QImage image = convertFrameToImage(m_frame);
                if (!image.isNull())
                {
                    emit frameDecoded(image);
                }

                // Release frame data
                av_frame_unref(m_frame);
            }

            // Reset packet to avoid potential issues
            av_packet_unref(m_packet);
        }
    }
}

void FfmpegDecoder::flush()
{
    RETURN_IF_NOT_INITIALIZED();

    LOG_INFO("Flushing decoder...");

    // Send NULL packet to flush decoder
    int ret = avcodec_send_packet(m_codecContext, nullptr);
    if (ret < 0 && ret != AVERROR_EOF)
    {
        LOG_ERROR(QString("Error sending flush packet: %1").arg(ret));
        emit errorOccurred(QString("Error flushing decoder: %1").arg(ret));
        return;
    }

    // Receive remaining frames
    int frameCount = 0;
    while (true)
    {
        ret = avcodec_receive_frame(m_codecContext, m_frame);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
        {
            break;
        }
        else if (ret < 0)
        {
            LOG_ERROR(QString("Error receiving frame during flush: %1").arg(ret));
            emit errorOccurred(QString("Error flushing decoder: %1").arg(ret));
            break;
        }

        // Convert frame and emit signal
        QImage image = convertFrameToImage(m_frame);
        if (!image.isNull())
        {
            emit frameDecoded(image);
            frameCount++;
        }

        // Release frame data
        av_frame_unref(m_frame);
    }

    LOG_INFO(QString("Decoder flushed, processed %1 remaining frames").arg(frameCount));
}

void FfmpegDecoder::cleanup()
{
    LOG_INFO("Starting decoder cleanup process");

    // Reset state flag to prevent other threads from using during cleanup
    m_initialized = false;

    // Ensure resources dependent on codec context are released first
    // Send NULL packet to flush decoder internal buffer
    if (m_codecContext && avcodec_is_open(m_codecContext))
    {
        avcodec_send_packet(m_codecContext, nullptr);
        LOG_INFO("Sent NULL packet to flush decoder context");
    }

    // Release parser resources
    if (m_parser)
    {
        av_parser_close(m_parser);
        m_parser = nullptr;
        LOG_INFO("Parser resources freed");
    }

    // Release AVFrame
    if (m_frame)
    {
        av_frame_unref(m_frame); // Release reference resources in the frame first
        av_frame_free(&m_frame);
        m_frame = nullptr;
        LOG_INFO("AVFrame freed");
    }

    // Release Packet
    if (m_packet)
    {
        av_packet_unref(m_packet); // Release reference resources in the packet first
        av_packet_free(&m_packet);
        m_packet = nullptr;
        LOG_INFO("AVPacket freed");
    }

    // Release codec context (critical resource, should be released after frames and packets)
    if (m_codecContext)
    {
        avcodec_free_context(&m_codecContext);
        m_codecContext = nullptr;
        LOG_INFO("Codec context freed");
    }

    // Clear codec reference
    m_codec = nullptr;

    // Release RGB frame resources
    if (m_rgbFrame)
    {
        av_frame_free(&m_rgbFrame);
        m_rgbFrame = nullptr;
        LOG_INFO("RGB frame freed");
    }

    if (m_rgbBuffer)
    {
        av_freep(&m_rgbBuffer);
        m_rgbBuffer = nullptr;
        LOG_INFO("RGB buffer freed");
    }

    // Release SWS context (should be released after all frame processing using it is complete)
    if (m_swsContext)
    {
        sws_freeContext(m_swsContext);
        m_swsContext = nullptr;
        LOG_INFO("SWS context freed");
    }

    // Reset state variables
    m_lastWidth = -1;
    m_lastHeight = -1;
    m_lastSrcFormat = AV_PIX_FMT_NONE;
    m_lastDstFormat = AV_PIX_FMT_NONE;

    LOG_INFO("Decoder resources cleaned up completely");
}

SwsContext*
FfmpegDecoder::getSwsContext(int srcW, int srcH, AVPixelFormat srcFormat, int dstW, int dstH, AVPixelFormat dstFormat)
{
    // 参数有效性检查
    if (srcW <= 0 || srcH <= 0 || dstW <= 0 || dstH <= 0)
    {
        LOG_ERROR("Invalid dimensions for SWS context");
        return nullptr;
    }

    // Check if we need to create a new context
    bool needsNewContext = !m_swsContext || m_lastWidth != srcW || m_lastHeight != srcH ||
                           m_lastSrcFormat != srcFormat || m_lastDstFormat != dstFormat;

    if (needsNewContext)
    {
        // Free old context if exists
        if (m_swsContext)
        {
            sws_freeContext(m_swsContext);
            m_swsContext = nullptr;
        }

        // Create new context with optimized settings
        // 使用更快的算法进行转换
        int flags = SWS_FAST_BILINEAR;

        // 对于相同尺寸的转换，可以使用更简单的算法
        if (srcW == dstW && srcH == dstH)
        {
            flags = SWS_POINT; // 点采样，速度最快
        }

        m_swsContext = sws_getContext(srcW, srcH, srcFormat, dstW, dstH, dstFormat, flags, nullptr, nullptr, nullptr);

        if (m_swsContext)
        {
            // Update last parameters
            m_lastWidth = srcW;
            m_lastHeight = srcH;
            m_lastSrcFormat = srcFormat;
            m_lastDstFormat = dstFormat;

            LOG_INFO(QString("Created new SWS context: %1x%2 -> %3x%4, format: %5 -> %6")
                         .arg(srcW)
                         .arg(srcH)
                         .arg(dstW)
                         .arg(dstH)
                         .arg(av_get_pix_fmt_name(srcFormat))
                         .arg(av_get_pix_fmt_name(dstFormat)));
        }
        else
        {
            LOG_ERROR("Failed to create SWS context");
        }
    }

    return m_swsContext;
}

QImage FfmpegDecoder::convertFrameToImage(AVFrame* frame)
{
    if (!frame || frame->width <= 0 || frame->height <= 0)
    {
        LOG_ERROR("Invalid frame for conversion");
        return QImage();
    }

    // Quick check for frame validity
    if (!frame->data[0] || frame->linesize[0] <= 0)
    {
        LOG_ERROR("Invalid frame data pointers or line size");
        return QImage();
    }

    QImage::Format format;
    int bytesPerLine;

    // Fast path: Check if already in RGB/RGBA format
    switch (frame->format)
    {
        case AV_PIX_FMT_RGBA:
            format = QImage::Format_RGBA8888;
            bytesPerLine = frame->linesize[0];
            break;
        case AV_PIX_FMT_RGB24:
            format = QImage::Format_RGB888;
            bytesPerLine = frame->linesize[0];
            break;
        case AV_PIX_FMT_YUV420P:
        default:
            // For YUV420P and other formats, conversion to RGB is needed
            {
                // Ensure RGB frame resources are allocated and dimensions match
                bool needsReallocation =
                    !m_rgbFrame || m_rgbFrame->width != frame->width || m_rgbFrame->height != frame->height;

                if (needsReallocation)
                {
                    // Release old resources
                    if (m_rgbFrame)
                    {
                        av_frame_free(&m_rgbFrame);
                        m_rgbFrame = nullptr;
                    }
                    if (m_rgbBuffer)
                    {
                        av_freep(&m_rgbBuffer);
                        m_rgbBuffer = nullptr;
                    }

                    // Allocate new RGB frame
                    m_rgbFrame = av_frame_alloc();
                    if (!m_rgbFrame)
                    {
                        LOG_ERROR("Failed to allocate RGB frame");
                        return QImage();
                    }

                    m_rgbFrame->format = AV_PIX_FMT_RGBA;
                    m_rgbFrame->width = frame->width;
                    m_rgbFrame->height = frame->height;

                    // Calculate required buffer size with larger alignment for better performance
                    int numBytes =
                        av_image_get_buffer_size(AV_PIX_FMT_RGBA, frame->width, frame->height, 16); // 16字节对齐

                    // Allocate buffer
                    m_rgbBuffer = (uint8_t*) av_malloc(numBytes * sizeof(uint8_t));
                    if (!m_rgbBuffer)
                    {
                        LOG_ERROR("Failed to allocate RGB buffer");
                        av_frame_free(&m_rgbFrame);
                        m_rgbFrame = nullptr;
                        return QImage();
                    }

                    // Assign buffer to frame
                    int ret = av_image_fill_arrays(m_rgbFrame->data,
                                                   m_rgbFrame->linesize,
                                                   m_rgbBuffer,
                                                   AV_PIX_FMT_RGBA,
                                                   frame->width,
                                                   frame->height,
                                                   16);

                    if (ret < 0)
                    {
                        LOG_ERROR(QString("Failed to fill RGB frame arrays: %1").arg(ret));
                        av_freep(&m_rgbBuffer);
                        av_frame_free(&m_rgbFrame);
                        m_rgbBuffer = nullptr;
                        m_rgbFrame = nullptr;
                        return QImage();
                    }

                    LOG_INFO(QString("Allocated RGB frame: %1x%2").arg(frame->width).arg(frame->height));
                }

                // Get (possibly cached) SWS context
                SwsContext* swsContext = getSwsContext(frame->width,
                                                       frame->height,
                                                       (AVPixelFormat) frame->format,
                                                       frame->width,
                                                       frame->height,
                                                       AV_PIX_FMT_RGBA);

                if (!swsContext)
                {
                    LOG_ERROR("Failed to get SWS context");
                    return QImage();
                }

                try
                {
                    // Perform conversion
                    int result = sws_scale(swsContext,
                                           frame->data,
                                           frame->linesize,
                                           0,
                                           frame->height,
                                           m_rgbFrame->data,
                                           m_rgbFrame->linesize);

                    if (result <= 0)
                    {
                        LOG_ERROR(QString("Failed to scale image: %1").arg(result));
                        return QImage();
                    }
                }
                catch (const std::exception& e)
                {
                    LOG_ERROR(QString("Exception during image scaling: %1").arg(e.what()));
                    return QImage();
                }
                catch (...)
                {
                    LOG_ERROR("Unknown exception during image scaling");
                    return QImage();
                }

                // Create QImage
                format = QImage::Format_RGBA8888;
                bytesPerLine = m_rgbFrame->linesize[0];

                // Create QImage and copy data
                try
                {
                    QImage image(m_rgbFrame->data[0], frame->width, frame->height, bytesPerLine, format);
                    return image.copy();
                }
                catch (const std::exception& e)
                {
                    LOG_ERROR(QString("Exception during QImage creation: %1").arg(e.what()));
                    return QImage();
                }
                catch (...)
                {
                    LOG_ERROR("Unknown exception during QImage creation");
                    return QImage();
                }
            }
    }

    // For frames already in RGB/RGBA format, create QImage directly
    // Note: Deep copy is needed because frame data may be overwritten
    try
    {
        QImage image(frame->data[0], frame->width, frame->height, bytesPerLine, format);
        return image.copy();
    }
    catch (const std::exception& e)
    {
        LOG_ERROR(QString("Exception during image creation: %1").arg(e.what()));
        return QImage();
    }
    catch (...)
    {
        LOG_ERROR("Unknown exception during image creation");
        return QImage();
    }
}