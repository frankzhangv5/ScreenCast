#include "OHOSDevice.h"

#include <algorithm>
#include <cstring>

OHOSDevice::OHOSDevice() : capturing(false)
{
    std::memset(&deviceInfo, 0, sizeof(deviceInfo));

    // Get device model
    strncpy(deviceInfo.model, OH_GetProductModel(), sizeof(deviceInfo.model));

    // Get device brand
    strncpy(deviceInfo.brand, OH_GetBrand(), sizeof(deviceInfo.brand));

    // Get device manufacturer
    strncpy(deviceInfo.manufacturer, OH_GetManufacture(), sizeof(deviceInfo.manufacturer));

    // Get device market_name
    strncpy(deviceInfo.market_name, OH_GetMarketName(), sizeof(deviceInfo.market_name));

    // Get OS version
    strncpy(deviceInfo.os_version, OH_GetOSFullName(), sizeof(deviceInfo.os_version));

    // Get API version
    deviceInfo.api_version = OH_GetSdkApiVersion();

    // Get display dimensions
    if (OH_NativeDisplayManager_GetDefaultDisplayWidth(&deviceInfo.screen_width) != 0 ||
        OH_NativeDisplayManager_GetDefaultDisplayHeight(&deviceInfo.screen_height) != 0)
    {
        LOGE("Failed to get display dimensions\n");
    }

    // Get DPI
    if (OH_NativeDisplayManager_GetDefaultDisplayDensityDpi(&deviceInfo.dpi) != 0)
    {
        LOGE("Failed to get display DPI\n");
    }
    // Get CPU architecture
    strncpy(deviceInfo.cpu_arch, OH_GetAbiList(), sizeof(deviceInfo.cpu_arch));
}

OHOSDevice::~OHOSDevice()
{
    stopScreenCapture();
}

bool OHOSDevice::startScreenCapture()
{
    if (capturing)
        return true;
    capturing = true;
    captureThread = std::thread([this]() {
        capture = OH_AVScreenCapture_Create();
        OH_AVScreenCapture_SetErrorCallback(capture, OnError, nullptr);
        OH_AVScreenCapture_SetStateCallback(capture, OnStateChange, nullptr);

        OH_VideoCaptureInfo videocapinfo = {.videoFrameWidth = deviceInfo.screen_width,
                                            .videoFrameHeight = deviceInfo.screen_height,
                                            .videoSource = OH_VIDEO_SOURCE_SURFACE_RGBA};
        OH_VideoEncInfo videoEncInfo = {.videoCodec = OH_VideoCodecFormat::OH_H264,
                                        .videoBitrate = options.bitrate,
                                        .videoFrameRate = options.frameRate};
        OH_VideoInfo videoinfo = {.videoCapInfo = videocapinfo, .videoEncInfo = videoEncInfo};
        OH_AVScreenCaptureConfig config = {
            .captureMode = OH_CAPTURE_HOME_SCREEN, .dataType = OH_ORIGINAL_STREAM, .videoInfo = videoinfo};
        OH_AVScreenCapture_Init(capture, config);

        h264Encoder = OH_VideoEncoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
        if (h264Encoder == nullptr)
        {
            LOGE("Failed to create H.264 encoder\n");
            return;
        }

        OH_AVFormat* format = OH_AVFormat_Create();
        OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, deviceInfo.screen_width);
        OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, deviceInfo.screen_height);
        OH_AVFormat_SetIntValue(format, OH_MD_KEY_PIXEL_FORMAT, AV_PIXEL_FORMAT_RGBA);
        OH_AVFormat_SetDoubleValue(format, OH_MD_KEY_FRAME_RATE, static_cast<double>(options.frameRate));
        OH_AVFormat_SetIntValue(format, OH_MD_KEY_I_FRAME_INTERVAL, 2000);
        OH_AVFormat_SetIntValue(format, OH_MD_KEY_PROFILE, AVC_PROFILE_MAIN);
        OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, options.bitrate);
        OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_ENCODE_BITRATE_MODE, CBR);

        OH_AVCodecAsyncCallback cb = {
            .onError =
                [](OH_AVCodec* codec, int32_t errorCode, void* userData) { LOGE("Encoder error: %d\n", errorCode); },
            .onNeedOutputData = OnEncoderFrameAvailable,
        };
        OH_VideoEncoder_SetCallback(h264Encoder, cb, this);

        int ret = OH_VideoEncoder_Configure(h264Encoder, format);
        OH_AVFormat_Destroy(format);
        if (ret != AV_ERR_OK)
        {
            LOGE("Failed to configure encoder\n");
            return;
        }

        OHNativeWindow* window;
        ret = OH_VideoEncoder_GetSurface(h264Encoder, &window);
        if (ret != AV_ERR_OK)
        {
            LOGE("Failed to get surface\n");
            return;
        }

        ret = OH_VideoEncoder_Prepare(h264Encoder);
        if (ret != AV_ERR_OK)
        {
            LOGE("Failed to prepare encoder\n");
            return;
        }

        ret = OH_AVScreenCapture_ShowCursor(capture, true);
        if (ret != AV_SCREEN_CAPTURE_ERR_OK)
        {
            LOGE("Failed to set show cursor\n");
            return;
        }

        ret = OH_AVScreenCapture_StartScreenCaptureWithSurface(capture, window);
        if (ret != AV_SCREEN_CAPTURE_ERR_OK)
        {
            LOGE("Failed to start screen capture\n");
            return;
        }
        LOGI("Screen capture started successfully");

        ret = OH_VideoEncoder_Start(h264Encoder);
        if (ret != AV_ERR_OK)
        {
            LOGE("Failed to start encoder\n");
            return;
        }

        while (capturing)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
    });
    return true;
}

void OHOSDevice::stopScreenCapture()
{
    capturing = false;
    if (captureThread.joinable())
    {
        captureThread.join();
    }
    std::lock_guard<std::mutex> lock(cb_mutex);
    LOGI("Cleaning up...");

    if (capture)
    {
        OH_AVScreenCapture_StopScreenCapture(capture);
        OH_AVScreenCapture_Release(capture);
    }

    if (h264Encoder)
    {
        OH_VideoEncoder_Stop(h264Encoder);
        OH_VideoEncoder_Destroy(h264Encoder);
    }
    LOGI("Recording finished");
}

void OHOSDevice::registerDataCallback(DataCallback cb)
{
    std::lock_guard<std::mutex> lock(cb_mutex);
    callbacks.push_back(cb);
}

void OHOSDevice::unregisterDataCallback(DataCallback cb)
{
    std::lock_guard<std::mutex> lock(cb_mutex);
    callbacks.erase(std::remove_if(callbacks.begin(),
                                   callbacks.end(),
                                   [&](const DataCallback& item) {
                                       // Only compare target type, use other methods for more precise comparison if
                                       // needed
                                       return item.target_type() == cb.target_type();
                                   }),
                    callbacks.end());
}

DeviceInfo OHOSDevice::getDeviceInfo()
{
    return deviceInfo;
}

void OHOSDevice::OnError(OH_AVScreenCapture* capture, int32_t errorCode, void* userData)
{
    LOGE("screenrecord OnError: %d\n", errorCode);
}

void OHOSDevice::OnStateChange(struct OH_AVScreenCapture* capture,
                               OH_AVScreenCaptureStateCode stateCode,
                               void* userData)
{
    if (stateCode == OH_SCREEN_CAPTURE_STATE_STARTED)
    {
        LOGI("Screen capture started");
    }
}

void OHOSDevice::OnEncoderFrameAvailable(OH_AVCodec* codec,
                                         uint32_t index,
                                         OH_AVMemory* data,
                                         OH_AVCodecBufferAttr* attr,
                                         void* userData)
{
    if (!userData)
        return;
    auto* self = static_cast<OHOSDevice*>(userData);
    if (attr && attr->size > 0)
    {
        uint8_t* buffer = static_cast<uint8_t*>(OH_AVMemory_GetAddr(data));
        std::lock_guard<std::mutex> lock(self->cb_mutex);
        for (auto& cb : self->callbacks)
        {
            if (cb)
                cb(std::vector<uint8_t>(buffer, buffer + attr->size));
        }
        LOGI("Encoded frame: index: %{public}d Size: %{public}d PTS: %{public}lld", index, attr->size, attr->pts);
    }
    OH_VideoEncoder_FreeOutputData(codec, index);
}