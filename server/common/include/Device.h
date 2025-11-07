#pragma once
#include <functional>
#include <mutex>
#include <string>
#include <vector>

// Device information struct
struct DeviceInfo
{
    char model[32];
    char brand[32];
    char manufacturer[32];
    char market_name[32];
    char os_version[32];
    int api_version;
    int dpi;
    int screen_width;
    int screen_height;
    char cpu_arch[16];
};

// Data callback type
using DataCallback = std::function<void(const std::vector<uint8_t>&)>;

// Device interface
class Device
{
public:
    virtual ~Device() = default;
    virtual bool startScreenCapture() = 0;
    virtual void stopScreenCapture() = 0;
    virtual void registerDataCallback(DataCallback cb) = 0;
    virtual void unregisterDataCallback(DataCallback cb) = 0;
    virtual DeviceInfo getDeviceInfo() = 0;
};