#pragma once
#include "Device.h"
#include "Log.h"

#include <atomic>
#include <cstdio>
#include <cstring>
#include <functional>
#include <mutex>
#include <sys/system_properties.h>
#include <thread>
#include <vector>

class AndroidDevice : public Device
{
public:
    AndroidDevice();
    ~AndroidDevice() override;
    bool startScreenCapture() override;
    void stopScreenCapture() override;
    void registerDataCallback(DataCallback cb) override;
    void unregisterDataCallback(DataCallback cb) override;
    DeviceInfo getDeviceInfo() override;

private:
    std::atomic<bool> capturing;
    std::thread captureThread;
    std::vector<DataCallback> callbacks;
    std::mutex cb_mutex;
    DeviceInfo deviceInfo;
    FILE* fp; // Added, used to save popen handle
};