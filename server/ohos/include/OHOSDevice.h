#pragma once
#include "Device.h"
#include "Log.h"

#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deviceinfo.h>
#include <functional>
#include <getopt.h>
#include <hilog/log.h>
#include <multimedia/player_framework/native_avcodec_videoencoder.h>
#include <multimedia/player_framework/native_avformat.h>
#include <multimedia/player_framework/native_avmemory.h>
#include <multimedia/player_framework/native_avscreen_capture.h>
#include <multimedia/player_framework/native_avscreen_capture_base.h>
#include <string>
#include <thread>
#include <unistd.h>
#include <vector>
#include <window_manager/oh_display_manager.h>

class OHOSDevice : public Device
{
public:
    OHOSDevice();
    ~OHOSDevice() override;
    bool startScreenCapture() override;
    void stopScreenCapture() override;
    void registerDataCallback(DataCallback cb) override;
    void unregisterDataCallback(DataCallback cb) override;
    DeviceInfo getDeviceInfo() override;

private:
    static void OnError(OH_AVScreenCapture* capture, int32_t errorCode, void* userData);
    static void
    OnStateChange(struct OH_AVScreenCapture* capture, OH_AVScreenCaptureStateCode stateCode, void* userData);
    static void OnEncoderFrameAvailable(OH_AVCodec* codec,
                                        uint32_t index,
                                        OH_AVMemory* data,
                                        OH_AVCodecBufferAttr* attr,
                                        void* userData);

    struct OH_AVScreenCapture* capture = nullptr;
    OH_AVCodec* h264Encoder = nullptr;
    std::atomic<bool> capturing;
    std::thread captureThread;
    std::vector<DataCallback> callbacks;
    std::mutex cb_mutex;

    struct
    {
        int32_t frameRate = 30.0; // Default frame rate
        int32_t bitrate = 8 * 1024 * 1024;
    } options;

    DeviceInfo deviceInfo;
};