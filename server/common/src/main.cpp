#include "Log.h"

#include <atomic>
#include <condition_variable>
#include <cstring>
#include <errno.h>
#include <memory>
#include <mutex>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <queue>
#include <stdio.h>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>
#include <vector>

#ifdef __ANDROID__
#    include "AndroidDevice.h"
#elif defined(__OHOS__)
#    include "OHOSDevice.h"
#else
#    include "AndroidDevice.h" // Default to Android implementation
#endif

// Command types (client -> server)
enum CommandType
{
    CMD_QUERY_DEVICE_INFO = 1,
    CMD_GET_SCREEN_FRAME = 2,
    CMD_START_SCREEN_CAPUTRE = 3,
    CMD_STOP_SCREEN_CAPUTRE = 4,
    CMD_EXIT = 5,
    CMD_UNKNOWN = 255
};

// Packet types (server -> client)
enum PacketType
{
    PKT_DEVICE_INFO = 1,
    PKT_SCREEN_FRAME = 2,
    PKT_ACK = 3,
    PKT_ERROR = 4,
    PKT_UNKNOWN = 255
};

// Message header struct
#pragma pack(push, 1)
struct MessageHeader
{
    uint8_t type;    // Packet type
    uint32_t length; // Data length
};
#pragma pack(pop)

// Message sending wrapper
void send_message(int client_socket, PacketType type, const void* data, uint32_t length)
{
    MessageHeader header;
    header.type = type;
    header.length = length;
    send(client_socket, &header, sizeof(header), 0);
    if (length > 0 && data)
    {
        send(client_socket, data, length, 0);
    }
}

std::atomic<bool> g_running(true);
std::atomic<int> g_client_count(0);
int server_socket = -1;

// Thread-safe frame queue structure
struct FrameQueue
{
    std::queue<std::vector<uint8_t>> queue;
    std::mutex mtx;
    std::condition_variable cv;
    bool stopped = false;

    void push(const std::vector<uint8_t>& frame)
    {
        std::lock_guard<std::mutex> lock(mtx);
        queue.push(frame);
        cv.notify_one();
    }

    // Return false means the queue has stopped
    bool pop(std::vector<uint8_t>& frame)
    {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [&] { return !queue.empty() || stopped; });
        if (stopped && queue.empty())
            return false;
        frame = std::move(queue.front());
        queue.pop();
        return true;
    }

    void stop()
    {
        std::lock_guard<std::mutex> lock(mtx);
        stopped = true;
        cv.notify_all();
    }
};

// Handle client commands
void handle_client(int client_socket, Device* device)
{
    printf("Client thread started\n");
    g_client_count++;

    int flag = 1;
    setsockopt(client_socket, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));

    // 1. Create frame queue and sending thread
    auto frameQueue = std::make_shared<FrameQueue>();
    std::atomic<bool> running(true);

    // 2. Callback only responsible for enqueue
    DataCallback cb = [frameQueue](const std::vector<uint8_t>& frame) { frameQueue->push(frame); };
    device->registerDataCallback(cb);

    // 3. Sending thread
    std::thread sender([client_socket, frameQueue, &running] {
        std::vector<uint8_t> frame;
        while (running && frameQueue->pop(frame))
        {
            LOGD("Sending frame, size=%" FMT_PUBLIC "zu", frame.size());
            send_message(client_socket, PKT_SCREEN_FRAME, frame.data(), static_cast<uint32_t>(frame.size()));
        }
    });

    // 4. Main thread handles commands
    bool recording = true;
    LOGI("Client connected, total clients: %" FMT_PUBLIC "d", g_client_count.load());
    while (recording && g_running)
    {
        MessageHeader header;
        ssize_t n = recv(client_socket, &header, sizeof(header), MSG_WAITALL);
        if (n <= 0)
            break;

        CommandType cmd = static_cast<CommandType>(header.type);
        LOGI("Client cmd type: %" FMT_PUBLIC "d", cmd);

        switch (cmd)
        {
            case CMD_QUERY_DEVICE_INFO: {
                DeviceInfo info = device->getDeviceInfo();
                send_message(client_socket, PKT_DEVICE_INFO, &info, sizeof(info));
                break;
            }
            case CMD_GET_SCREEN_FRAME: {
                char fake_frame[256] = "FAKE_SCREEN_FRAME_DATA";
                send_message(client_socket, PKT_SCREEN_FRAME, fake_frame, sizeof(fake_frame));
                break;
            }
            case CMD_START_SCREEN_CAPUTRE: {
                if (!device->startScreenCapture())
                {
                    send_message(client_socket, PKT_ERROR, "Failed to start screen capture", 30);
                    break;
                }
                LOGI("Start screen recording");
                break;
            }
            case CMD_STOP_SCREEN_CAPUTRE: {
                recording = false;
                LOGI("Stop screen recording");
                break;
            }
            case CMD_EXIT: {
                g_running = false;
                LOGI("Exit command received, shutting down server.");
                close(server_socket);
                break;
            }
            default: {
                send_message(client_socket, PKT_UNKNOWN, nullptr, 0);
                break;
            }
        }
    }
    // 5. Exit and cleanup
    running = false;
    frameQueue->stop();
    if (sender.joinable())
        sender.join();
    device->unregisterDataCallback(cb);
    close(client_socket);
    LOGI("Client thread ended");

    g_client_count--;
    LOGI("Client disconnected, remaining clients: %" FMT_PUBLIC "d", g_client_count.load());
    if (g_client_count.load() == 0)
    {
        device->stopScreenCapture();
        LOGI("Auto stop screen capture(client count = 0)");
    }
}

int main()
{
    // Create corresponding Device object based on platform type
    Device* device = nullptr;
#ifdef __ANDROID__
    device = new AndroidDevice();
    LOGI("Platform: Android");
#elif defined(__OHOS__)
    device = new OHOSDevice();
    LOGI("Platform: OpenHarmony");
#else
    device = new AndroidDevice();
    LOGI("Platform: Default(Android)");
#endif

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0)
    {
        LOGE("Failed to create socket");
        delete device;
        return -1;
    }

    // Set port reuse
    int opt = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_socket, (struct sockaddr*) &server_addr, sizeof(server_addr)) < 0)
    {
        LOGE("Failed to bind socket");
        close(server_socket);
        delete device;
        return -1;
    }

    if (listen(server_socket, 4) < 0)
    {
        LOGE("Failed to listen on socket");
        close(server_socket);
        delete device;
        return -1;
    }

    LOGI("Server started, waiting for client...");

    std::vector<std::thread> threads;
    while (g_running)
    {
        int client_socket = accept(server_socket, NULL, NULL);
        if (client_socket < 0)
        {
            if (!g_running)
                break;
            LOGE("Failed to accept client");
            continue;
        }
        LOGI("Client connected!");
        threads.emplace_back(std::thread(handle_client, client_socket, device));
    }

    for (auto& t : threads)
    {
        if (t.joinable())
            t.join();
    }

    close(server_socket);
    delete device;
    LOGI("Server exited.");
    return 0;
}