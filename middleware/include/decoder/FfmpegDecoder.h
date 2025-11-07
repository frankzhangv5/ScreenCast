#pragma once

#include <QByteArray>
#include <QImage>
#include <QObject>

// FFmpeg headers
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libavutil/pixfmt.h>
#include <libswscale/swscale.h>
}

#include "StreamDecoder.h"

/**
 * @brief FfmpegDecoder class
 *
 * This class implements the StreamDecoder interface using FFmpeg library
 * to decode video streams, particularly H264 encoded streams.
 */
class FfmpegDecoder : public StreamDecoder
{
    Q_OBJECT
public:
    explicit FfmpegDecoder(QObject* parent = nullptr);
    virtual ~FfmpegDecoder() override;

public slots:
    /**
     * @brief Initialize the FFmpeg decoder
     * @return true if initialization was successful, false otherwise
     */
    virtual bool initialize() override;

    /**
     * @brief Decode the given encoded data
     * @param data The encoded video data to decode
     */
    virtual void decode(const QByteArray& data) override;

    /**
     * @brief Flush any buffered frames in the decoder
     */
    virtual void flush() override;

private:
    /**
     * @brief Clean up decoder resources
     */
    void cleanup();

    /**
     * @brief Convert AVFrame to QImage
     * @param frame The AVFrame to convert
     * @return Converted QImage
     */
    QImage convertFrameToImage(AVFrame* frame);

    /**
     * @brief Create or update SWS context
     * @param srcW Source width
     * @param srcH Source height
     * @param srcFormat Source pixel format
     * @param dstW Destination width
     * @param dstH Destination height
     * @param dstFormat Destination pixel format
     * @return SWS context for conversion
     */
    SwsContext* getSwsContext(int srcW, int srcH, AVPixelFormat srcFormat, int dstW, int dstH, AVPixelFormat dstFormat);

private:
    AVCodecContext* m_codecContext; ///< FFmpeg codec context
    AVFrame* m_frame;               ///< FFmpeg frame for decoded data
    AVPacket* m_packet;             ///< FFmpeg packet for encoded data
    AVCodec* m_codec;               ///< FFmpeg codec
    AVCodecParserContext* m_parser; ///< FFmpeg parser context for H.264
    bool m_initialized;             ///< Initialization flag

    // Optimization members
    SwsContext* m_swsContext;      ///< Cached SWS context
    int m_lastWidth;               ///< Last converted width
    int m_lastHeight;              ///< Last converted height
    AVPixelFormat m_lastSrcFormat; ///< Last source format
    AVPixelFormat m_lastDstFormat; ///< Last destination format
    AVFrame* m_rgbFrame;           ///< Reusable RGB frame
    uint8_t* m_rgbBuffer;          ///< Reusable RGB buffer
};