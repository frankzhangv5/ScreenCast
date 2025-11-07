#include "decoder/StreamDecoderFactory.h"

#include "decoder/FfmpegDecoder.h"

#include <QDebug>

/**
 * @brief Create a decoder instance based on the specified type
 * @param type The type of decoder to create
 * @param parent Parent QObject
 * @return Created decoder instance, or nullptr if creation failed
 */
StreamDecoder* StreamDecoderFactory::create(DecoderType type, QObject* parent)
{
    switch (type)
    {
        case DecoderType::FFMPEGH264:
            qDebug() << "Creating FFmpeg H264 decoder";
            return new FfmpegDecoder(parent);
        case DecoderType::NONE:
        default:
            qWarning() << "Invalid decoder type specified";
            return nullptr;
    }
}