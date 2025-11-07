#include "AndroidDevice.h"

#include <algorithm>
#include <chrono>
#include <cstdlib>

AndroidDevice::AndroidDevice() : capturing(false), fp(nullptr)
{
    // Get basic device information
    __system_property_get("ro.product.model", deviceInfo.model);
    __system_property_get("ro.product.brand", deviceInfo.brand);
    __system_property_get("ro.product.manufacturer", deviceInfo.manufacturer);
    __system_property_get("ro.product.nickname", deviceInfo.market_name);
    __system_property_get("ro.build.version.release", deviceInfo.os_version);

    char api_buf[PROP_VALUE_MAX] = {0};
    if (__system_property_get("ro.build.version.sdk", api_buf) > 0)
    {
        deviceInfo.api_version = atoi(api_buf);
    }
    else
    {
        deviceInfo.api_version = 0;
    }

    __system_property_get("ro.product.cpu.abi", deviceInfo.cpu_arch);

    // Get screen dpi
    FILE* fp = popen("wm density", "r");
    deviceInfo.dpi = 0;
    if (fp)
    {
        char buf[128] = {0};
        while (fgets(buf, sizeof(buf), fp))
        {
            // Example: Physical density: 440
            char* p = strstr(buf, "Physical density:");
            if (p)
            {
                int dpi = 0;
                if (sscanf(p, "Physical density: %d", &dpi) == 1)
                {
                    deviceInfo.dpi = dpi;
                }
                break;
            }
        }
        pclose(fp);
    }

    // Get screen resolution
    fp = popen("wm size", "r");
    deviceInfo.screen_width = 0;
    deviceInfo.screen_height = 0;
    if (fp)
    {
        char buf[128] = {0};
        while (fgets(buf, sizeof(buf), fp))
        {
            // Example: Physical size: 1080x2340
            char* p = strstr(buf, "Physical size:");
            if (p)
            {
                int w = 0, h = 0;
                if (sscanf(p, "Physical size: %dx%d", &w, &h) == 2)
                {
                    deviceInfo.screen_width = w;
                    deviceInfo.screen_height = h;
                }
                break;
            }
        }
        pclose(fp);
    }
}

AndroidDevice::~AndroidDevice()
{
    stopScreenCapture();
}

bool AndroidDevice::startScreenCapture()
{
    if (capturing)
        return true;
    capturing = true;
    captureThread = std::thread([this]() {
        while (capturing)
        {
            system("input keyevent KEYCODE_WAKEUP");
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            fp = popen("screenrecord --output-format=h264 -", "r");
            if (!fp)
                break;
            std::vector<uint8_t> buffer(4096);
            while (capturing)
            {
                size_t n = fread(buffer.data(), 1, buffer.size(), fp);
                if (n > 0)
                {
                    LOGI("Captured %zu bytes of screen data", n);
                    std::lock_guard<std::mutex> lock(cb_mutex);
                    for (auto& cb : callbacks)
                    {
                        if (cb)
                            cb(std::vector<uint8_t>(buffer.begin(), buffer.begin() + n));
                    }
                }
                else if (n == 0)
                {
                    break;
                }
            }
            if (fp)
            {
                pclose(fp);
                fp = nullptr;
            }
            if (capturing)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            }
        }
    });
    return true;
}

void AndroidDevice::stopScreenCapture()
{
    capturing = false;
    // Kill screenrecord process to ensure fread can return
    system("pkill -9 screenrecord");
    // Close pipe
    FILE* tmp = fp;
    fp = nullptr;
    if (tmp)
    {
        pclose(tmp);
    }
    if (captureThread.joinable())
    {
        captureThread.join();
    }
}

void AndroidDevice::registerDataCallback(DataCallback cb)
{
    std::lock_guard<std::mutex> lock(cb_mutex);
    callbacks.push_back(cb);
}

void AndroidDevice::unregisterDataCallback(DataCallback cb)
{
    std::lock_guard<std::mutex> lock(cb_mutex);
    callbacks.erase(std::remove_if(callbacks.begin(),
                                   callbacks.end(),
                                   [&](const DataCallback& item) {
                                       // std::function comparison can only compare target address
                                       return item.target_type() == cb.target_type();
                                   }),
                    callbacks.end());
}

DeviceInfo AndroidDevice::getDeviceInfo()
{
    return deviceInfo;
}