#pragma once

#include "StreamDecoder.h"

/**
 * @brief StreamDecoderFactory
 *
 * A factory class for creating decoder instances based on the specified type.
 * Currently supports creating OpenH264 and FFmpeg decoders for H264 streams.
 */
class StreamDecoderFactory
{
public:
    /**
     * @brief Create a decoder instance
     *
     * @param type The type of decoder to create
     * @param parent Optional parent QObject
     * @return Pointer to the created decoder instance, or nullptr if creation failed
     */
    static StreamDecoder* create(DecoderType type, QObject* parent = nullptr);
};