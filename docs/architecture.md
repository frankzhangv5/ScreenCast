# 🏗️ 架构设计

## 🎯 架构概述

ScreenCast 采用模块化、插件化的架构设计，确保系统的可扩展性、可维护性和高性能。整个系统分为核心层、设备层、界面层和工具层四个主要层次。

## 🏛️ 整体架构

```
┌─────────────────────────────────────────────────────────────┐
│                        界面层 (UI Layer)                      │
├─────────────────────────────────────────────────────────────┤
│  主窗口  │  设备列表  │  投屏窗口  │  设置窗口  │  系统托盘    │
├─────────────────────────────────────────────────────────────┤
│                        设备层 (Device Layer)                  │
├─────────────────────────────────────────────────────────────┤
│  设备管理器  │  Android设备  │  OHOS设备  │  设备连接器      │
├─────────────────────────────────────────────────────────────┤
│                        核心层 (Core Layer)                    │
├─────────────────────────────────────────────────────────────┤
│  配置管理  │  流解码器  │  流读取器  │  日志系统            │
├─────────────────────────────────────────────────────────────┤
│                        工具层 (Util Layer)                    │
├─────────────────────────────────────────────────────────────┤
│  工具函数  │  常量定义  │  类型定义  │  错误处理            │
└─────────────────────────────────────────────────────────────┘
```

## 🔧 核心模块

### 配置管理 (Settings)
负责应用程序的配置管理，包括用户设置、系统配置等。

```cpp
class Settings : public QObject
{
    Q_OBJECT

public:
    static Settings* instance();
    
    // 配置读写
    QVariant getValue(const QString& key, const QVariant& defaultValue = QVariant());
    void setValue(const QString& key, const QVariant& value);
    
    // 配置重置
    void resetToDefault();
    
private:
    Settings();
    void loadSettings();
    void saveSettings();
    
    QSettings* m_settings;
};
```

### 流解码器 (StreamDecoder)
负责视频流的解码和处理，支持多种编码格式。

```cpp
class StreamDecoder : public QObject
{
    Q_OBJECT

public:
    explicit StreamDecoder(QObject* parent = nullptr);
    ~StreamDecoder();
    
    // 解码控制
    bool startDecode(const QString& deviceId);
    void stopDecode();
    
    // 帧处理
    void processFrame(const QByteArray& frameData);
    
signals:
    void frameReady(const QImage& frame);
    void decodeError(const QString& error);
    
private:
    void initializeCodec();
    void cleanupCodec();
    
    AVCodecContext* m_codecContext;
    SwsContext* m_swsContext;
};
```

### 流读取器 (StreamReader)
负责从设备读取视频流数据。

```cpp
class StreamReader : public QObject
{
    Q_OBJECT

public:
    explicit StreamReader(QObject* parent = nullptr);
    ~StreamReader();
    
    // 流控制
    bool startRead(const QString& deviceId);
    void stopRead();
    
signals:
    void dataReceived(const QByteArray& data);
    void readError(const QString& error);
    
private:
    void readLoop();
    
    QThread* m_readThread;
    bool m_isReading;
};
```

## 📱 设备模块

### 设备管理器 (DeviceManager)
统一管理所有设备，提供设备发现、连接、管理等功能。

```cpp
class DeviceManager : public QObject
{
    Q_OBJECT

public:
    static DeviceManager* instance();
    
    // 设备管理
    QVector<DeviceInfo> devices() const;
    bool connectDevice(const QString& deviceId);
    void disconnectDevice(const QString& deviceId);
    
    // 缓存管理
    void clearCache();
    
signals:
    void deviceDiscovered(const DeviceInfo& device);
    void deviceConnected(const QString& deviceId);
    void deviceDisconnected(const QString& deviceId);
    
private:
    void initializeDevices();
    void loadDeviceCache();
    
    QVector<DeviceInfo> m_deviceList;
    QHash<QString, Device*> m_connectedDevices;
    QVector<DeviceProxy*> m_proxies;
};
```

### 设备代理 (DeviceProxy)
抽象设备接口，支持不同平台的设备实现。

```cpp
class DeviceProxy : public QObject
{
    Q_OBJECT

public:
    virtual ~DeviceProxy() = default;
    
    // 设备查询
    virtual QVector<DeviceInfo> queryDevices() = 0;
    
    // 设备连接
    virtual bool setupDeviceServer(const QString& serial, int forwardPort) = 0;
    virtual bool startDeviceServer(const QString& serial) = 0;
    virtual void stopDeviceServer(const QString& serial) = 0;
    
    // 设备类型
    virtual DeviceType deviceType() const = 0;
    
signals:
    void deviceFound(const DeviceInfo& device);
    void deviceLost(const QString& serial);
};
```

### Android 设备 (AndroidDevice)
Android 设备的具体实现。

```cpp
class AndroidDevice : public DeviceProxy
{
    Q_OBJECT

public:
    explicit AndroidDevice(QObject* parent = nullptr);
    
    // 实现 DeviceProxy 接口
    QVector<DeviceInfo> queryDevices() override;
    bool setupDeviceServer(const QString& serial, int forwardPort) override;
    bool startDeviceServer(const QString& serial) override;
    void stopDeviceServer(const QString& serial) override;
    DeviceType deviceType() const override { return DeviceType::Android; }
    
private:
    bool checkAdbAvailable();
    QString executeAdbCommand(const QStringList& args);
    
    QString m_adbPath;
};
```

### OpenHarmony 设备 (OHOSDevice)
OpenHarmony 设备的具体实现。

```cpp
class OHOSDevice : public DeviceProxy
{
    Q_OBJECT

public:
    explicit OHOSDevice(QObject* parent = nullptr);
    
    // 实现 DeviceProxy 接口
    QVector<DeviceInfo> queryDevices() override;
    bool setupDeviceServer(const QString& serial, int forwardPort) override;
    bool startDeviceServer(const QString& serial) override;
    void stopDeviceServer(const QString& serial) override;
    DeviceType deviceType() const override { return DeviceType::OHOS; }
    
private:
    bool checkHdcAvailable();
    QString executeHdcCommand(const QStringList& args);
    
    QString m_hdcPath;
};
```

## 🖥️ 界面模块

### 主窗口 (MainWindow)
应用程序的主窗口，提供整体界面框架。

```cpp
class MainWindow : public FramelessWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

private slots:
    void onDeviceSelected(const QString& deviceId);
    void onSettingsClicked();
    void onAboutClicked();
    
private:
    void setupUI();
    void setupConnections();
    
    TitleBar* m_titleBar;
    DeviceListPage* m_deviceListPage;
    StatusBar* m_statusBar;
};
```

### 设备列表页面 (DeviceListPage)
显示和管理设备列表的页面。

```cpp
class DeviceListPage : public QWidget
{
    Q_OBJECT

public:
    explicit DeviceListPage(QWidget* parent = nullptr);

signals:
    void deviceSelected(const QString& deviceId);
    
private slots:
    void onDeviceDiscovered(const DeviceInfo& device);
    void onDeviceConnected(const QString& deviceId);
    void onDeviceDisconnected(const QString& deviceId);
    void onRefreshClicked();
    
private:
    void setupUI();
    void updateDeviceList();
    
    QVBoxLayout* m_layout;
    QListWidget* m_deviceList;
    QPushButton* m_refreshButton;
};
```

### 投屏窗口 (ScreenWindow)
显示设备投屏内容的窗口。

```cpp
class ScreenWindow : public FramelessWindow
{
    Q_OBJECT

public:
    explicit ScreenWindow(const QString& deviceId, QWidget* parent = nullptr);
    ~ScreenWindow();

private slots:
    void onFrameReady(const QImage& frame);
    void onScreenshotClicked();
    void onFullscreenClicked();
    
private:
    void setupUI();
    void setupConnections();
    void handleMouseEvent(QMouseEvent* event);
    void handleKeyEvent(QKeyEvent* event);
    
    QString m_deviceId;
    QLabel* m_screenLabel;
    StreamDecoder* m_decoder;
    StreamReader* m_reader;
};
```

## 🛠️ 工具模块

### 日志系统 (Log)
提供统一的日志记录和管理功能。

```cpp
class Log : public QObject
{
    Q_OBJECT

public:
    enum Level {
        Debug,
        Info,
        Warning,
        Error,
        Critical
    };
    
    static void debug(const QString& message);
    static void info(const QString& message);
    static void warning(const QString& message);
    static void error(const QString& message);
    static void critical(const QString& message);
    
private:
    static void writeLog(Level level, const QString& message);
    
    static QFile* m_logFile;
    static QMutex m_mutex;
};
```

### 无边框窗口 (FramelessWindow)
提供无边框窗口的基础功能。

```cpp
class FramelessWindow : public QWidget
{
    Q_OBJECT

public:
    explicit FramelessWindow(QWidget* parent = nullptr);

protected:
    bool nativeEvent(const QByteArray& eventType, void* message, long* result) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    
private:
    bool m_isResizing;
    QPoint m_lastPos;
    Qt::Edges m_resizeEdges;
};
```

## 🔄 数据流

### 设备发现流程
```
1. 应用启动 → DeviceManager 初始化
2. 加载设备代理 → AndroidDevice, OHOSDevice
3. 定期查询设备 → queryDevices()
4. 发现新设备 → deviceDiscovered 信号
5. 更新设备列表 → DeviceListPage 更新
```

### 投屏连接流程
```
1. 用户选择设备 → deviceSelected 信号
2. 创建设备服务器 → setupDeviceServer()
3. 启动流读取器 → StreamReader::startRead()
4. 启动流解码器 → StreamDecoder::startDecode()
5. 显示投屏窗口 → ScreenWindow 显示
6. 处理视频帧 → frameReady 信号
```

### 数据传递流程
```
设备 → StreamReader → StreamDecoder → ScreenWindow
  ↓         ↓            ↓              ↓
原始数据 → 字节数组 → QImage → 界面显示
```

## 🔧 扩展机制

### 插件系统
通过插件系统支持功能扩展：

```cpp
class PluginInterface
{
public:
    virtual ~PluginInterface() = default;
    virtual QString name() const = 0;
    virtual QString version() const = 0;
    virtual bool initialize() = 0;
    virtual void cleanup() = 0;
};

Q_DECLARE_INTERFACE(PluginInterface, "com.screencast.PluginInterface")
```

### 新平台支持
添加新平台支持的步骤：

1. **实现 DeviceProxy 接口**
2. **注册到 DeviceManager**
3. **添加平台类型枚举**
4. **更新设备识别逻辑**

## 📊 性能优化

### 内存管理
- **智能缓存**: 缓存常用数据和对象
- **及时释放**: 及时释放不再使用的资源
- **内存池**: 使用内存池减少分配开销

### 多线程处理
- **UI 线程**: 处理界面更新和用户交互
- **工作线程**: 处理设备通信和数据解码
- **线程安全**: 使用互斥锁保护共享数据

### 网络优化
- **连接复用**: 复用网络连接减少开销
- **数据压缩**: 压缩传输数据减少带宽
- **流量控制**: 控制数据传输速率

---

**ScreenCast 的架构设计注重模块化、可扩展性和高性能，为项目的长期发展奠定了坚实的基础。** 