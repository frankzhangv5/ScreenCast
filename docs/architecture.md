# 🏗️ 架构设计

## 🎯 架构概述

ScreenCast 采用高度模块化、插件化的架构设计，确保系统的可扩展性、可维护性和高性能。系统按照功能职责划分为多个独立模块，通过清晰的接口进行交互。整体架构以目录结构为基础，包括app（UI部分）、middleware（核心功能）、plugins（设备插件）、server（镜像服务器）等核心目录。

## 🏛️ 整体架构

### 目录结构

```
┌─────────────────────────────────────────────────────────────┐
│                        项目根目录                             │
├────────┬────────────┬─────────────┬───────────┬────────────┤
│  app   │ middleware │   plugins   │  server   │    sdk     │
├────────┼────────────┼─────────────┼───────────┼────────────┤
│  UI界面 │ 核心功能   │ 设备插件     │ 镜像服务器 │ 公共接口    │
└────────┴────────────┴─────────────┴───────────┴────────────┘
```

### 层级架构

```
┌─────────────────────────────────────────────────────────────┐
│                        界面层 (App Layer)                     │
├─────────────────────────────────────────────────────────────┤
│  主窗口  │  设备列表  │  投屏窗口  │  设置窗口  │  系统托盘    │
├─────────────────────────────────────────────────────────────┤
│                        中间件层 (Middleware Layer)            │
├─────────────────────────────────────────────────────────────┤
│  设备管理  │  插件管理  │  配置管理  │  日志系统  │  流处理      │
├─────────────────────────────────────────────────────────────┤
│                        插件层 (Plugin Layer)                  │
├─────────────────────────────────────────────────────────────┤
│  Android插件 │  HarmonyOS插件 │  扩展接口  │  设备通信      │
├─────────────────────────────────────────────────────────────┤
│                        服务器层 (Server Layer)                │
├─────────────────────────────────────────────────────────────┤
│  Android服务 │  HarmonyOS服务 │  屏幕捕获  │  数据传输      │
└─────────────────────────────────────────────────────────────┘
```

## 🔧 核心模块

在 ScreenCast 中，核心功能被组织在 middleware 目录下，实现了设备管理、插件管理、配置项持久化等核心功能。middleware 目录结构如下：

```
middleware/
├── include/
│   ├── connector/    # 设备连接器
│   ├── decoder/      # 视频解码器
│   ├── log/          # 日志系统
│   ├── manager/      # 管理器（设备、插件等）
│   ├── processor/    # 数据处理器
│   ├── provider/     # 服务提供者
│   └── settings/     # 配置管理
└── src/              # 源代码实现
```

### 配置管理 (Settings)
位于 middleware/settings/ 目录，负责应用程序的配置管理，包括用户设置、系统配置等持久化存储。

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
位于 middleware/decoder/ 目录，负责视频流的解码和处理，支持多种编码格式。

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

### 设备连接器 (DeviceConnector)
位于 middleware/connector/ 目录，负责与设备建立和维护连接。

```cpp
class DeviceConnector : public QObject
{
    Q_OBJECT

public:
    explicit DeviceConnector(QObject* parent = nullptr);
    
    // 连接控制
    bool connectDevice(const QString& deviceId);
    void disconnectDevice();
    
    // 数据传输
    bool sendCommand(const QByteArray& command);
    
signals:
    void connected();
    void disconnected();
    void dataReceived(const QByteArray& data);
    void connectionError(const QString& error);
    
private:
    void setupConnection();
    void handleIncomingData();
    
    QTcpSocket* m_socket;
    QString m_deviceId;
};
```

### 设备管理器 (DeviceManager)
位于 middleware/manager/ 目录，统一管理所有设备，提供设备发现、连接、管理等功能。

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

### 插件管理器 (PluginManager)
位于 middleware/manager/ 目录，负责插件的加载、初始化和管理。

```cpp
class PluginManager : public QObject
{
    Q_OBJECT

public:
    static PluginManager* instance();
    
    // 插件管理
    bool loadPlugin(const QString& pluginPath);
    void unloadPlugin(const QString& pluginName);
    QList<PluginInfo> loadedPlugins() const;
    
    // 插件查找
    PluginInterface* getPlugin(const QString& pluginName);
    
signals:
    void pluginLoaded(const PluginInfo& info);
    void pluginUnloaded(const QString& pluginName);
    
private:
    void scanPluginDir();
    void initializePlugins();
    
    QHash<QString, QPluginLoader*> m_pluginLoaders;
    QHash<QString, PluginInterface*> m_plugins;
};
```

## 📱 设备模块

在 ScreenCast 中，设备支持通过插件化架构实现，所有设备插件位于 plugins 目录下。插件化设计使得添加新设备支持变得简单且独立，无需修改核心代码。

### 插件目录结构

```
plugins/
├── android/        # Android设备插件
│   ├── AndroidDevice.cpp
│   ├── AndroidDevice.h
│   ├── AndroidDevicePlugin.cpp
│   ├── AndroidDevicePlugin.h
│   ├── android.pro
│   └── res/        # 资源文件
└── ohos/           # HarmonyOS设备插件
    ├── OHOSDevice.cpp
    ├── OHOSDevice.h
    ├── OHOSDevicePlugin.cpp
    ├── OHOSDevicePlugin.h
    ├── ohos.pro
    └── res/        # 资源文件
```

### 设备插件接口

设备插件通过实现标准接口来集成到系统中：

```cpp
class DevicePluginInterface : public PluginInterface
{
public:
    virtual ~DevicePluginInterface() = default;
    
    // 获取设备代理
    virtual DeviceProxy* getDeviceProxy() = 0;
    
    // 设备类型
    virtual DeviceType supportedDeviceType() const = 0;
    
    // 设备名称
    virtual QString deviceTypeName() const = 0;
};

Q_DECLARE_INTERFACE(DevicePluginInterface, "com.screencast.DevicePluginInterface")

// 设备代理基类
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

### Android 设备插件实现

Android设备插件通过ADB工具与Android设备通信：

```cpp
// Android设备插件类
class AndroidDevicePlugin : public QObject, public DevicePluginInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "com.screencast.DevicePluginInterface")
    Q_INTERFACES(DevicePluginInterface)

public:
    AndroidDevicePlugin(QObject* parent = nullptr);
    
    // 实现 PluginInterface
    QString name() const override { return "AndroidDevicePlugin"; }
    QString version() const override { return "1.0.0"; }
    bool initialize() override;
    void cleanup() override;
    
    // 实现 DevicePluginInterface
    DeviceProxy* getDeviceProxy() override { return m_deviceProxy; }
    DeviceType supportedDeviceType() const override { return DeviceType::Android; }
    QString deviceTypeName() const override { return "Android"; }
    
private:
    AndroidDevice* m_deviceProxy;
};

// Android设备代理实现
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

### HarmonyOS 设备插件实现

HarmonyOS设备插件通过HDC工具与OpenHarmony设备通信：

```cpp
// HarmonyOS设备插件类
class OHOSDevicePlugin : public QObject, public DevicePluginInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "com.screencast.DevicePluginInterface")
    Q_INTERFACES(DevicePluginInterface)

public:
    OHOSDevicePlugin(QObject* parent = nullptr);
    
    // 实现 PluginInterface
    QString name() const override { return "OHOSDevicePlugin"; }
    QString version() const override { return "1.0.0"; }
    bool initialize() override;
    void cleanup() override;
    
    // 实现 DevicePluginInterface
    DeviceProxy* getDeviceProxy() override { return m_deviceProxy; }
    DeviceType supportedDeviceType() const override { return DeviceType::OHOS; }
    QString deviceTypeName() const override { return "HarmonyOS"; }
    
private:
    OHOSDevice* m_deviceProxy;
};

// HarmonyOS设备代理实现
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

## 🖥️ 界面模块 (App Layer)

在ScreenCast版本中，界面模块位于`app`目录下，采用模块化设计，包含主窗口、设备列表、屏幕投射和设置等UI组件。

### app目录结构
```
app/
├── main.cpp          # 应用程序入口
├── mainwindow/       # 主窗口模块
├── device/           # 设备相关UI组件
├── screencast/       # 屏幕投射相关UI组件
├── settings/         # 设置相关UI组件
└── resources/        # 资源文件
```

### 主窗口 (MainWindow)

主应用窗口，作为整个应用的容器，集成了各个功能模块的UI组件。

```cpp
// app/mainwindow/mainwindow.h
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
    void setupNavigation();
    
    // UI组件
    QTabWidget* m_tabWidget;
    DeviceListWidget* m_deviceListWidget;
    ScreenCastWidget* m_screenCastWidget;
    
    // 业务对象
    DeviceConnector* m_deviceConnector;
    PluginManager* m_pluginManager;
    
    // 设置和配置
    SettingsManager* m_settingsManager;
    
    // 状态栏
    TitleBar* m_titleBar;
    StatusBar* m_statusBar;
};
```

### 设备列表组件 (DeviceListWidget)

显示已连接设备的列表，支持设备搜索、过滤和连接管理功能。

```cpp
// app/device/devicelistwidget.h
class DeviceListWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DeviceListWidget(QWidget* parent = nullptr);
    
public slots:
    void updateDeviceList(const QVector<DeviceInfo>& devices);
    void setConnectedDevice(const QString& deviceId);
    
private slots:
    void onDeviceDiscovered(const DeviceInfo& device);
    void onDeviceConnected(const QString& deviceId);
    void onDeviceDisconnected(const QString& deviceId);
    void onRefreshClicked();
    void onFilterTextChanged(const QString& text);
    
private:
    void setupUI();
    void updateDeviceList();
    
    QVBoxLayout* m_layout;
    QListWidget* m_deviceList;
    QLineEdit* m_filterLineEdit;
    QPushButton* m_refreshButton;
    
    QString m_currentDeviceId;
    
signals:
    void deviceSelected(const QString& deviceId);
    void refreshDevicesRequested();
};
```

### 屏幕投射组件 (ScreenCastWidget)

显示设备的屏幕内容，并提供控制功能如录制、截图、缩放等。

```cpp
// app/screencast/screencastwidget.h
class ScreenCastWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ScreenCastWidget(const QString& deviceId, QWidget* parent = nullptr);
    ~ScreenCastWidget();

private slots:
    void onFrameReady(const QImage& frame);
    void onScreenshotClicked();
    void onFullscreenClicked();
    void onRecordButtonClicked();
    void onZoomLevelChanged(int zoom);
    
private:
    void setupUI();
    void setupConnections();
    void handleMouseEvent(QMouseEvent* event);
    void handleKeyEvent(QKeyEvent* event);
    
    QString m_deviceId;
    QLabel* m_screenLabel;
    QPushButton* m_screenshotButton;
    QPushButton* m_fullscreenButton;
    QPushButton* m_recordButton;
    QSlider* m_zoomSlider;
    
    bool m_isRecording;
    StreamDecoder* m_decoder;
    StreamReader* m_reader;
    
signals:
    void startRecording();
    void stopRecording();
    void takeScreenshot();
};

### 设置组件 (SettingsDialog)

提供应用程序配置选项，包括设备工具路径、网络设置和界面设置等。

```cpp
// app/settings/settingsdialog.h
class SettingsDialog : public QDialog
{
    Q_OBJECT

public:
    SettingsDialog(QWidget *parent = nullptr);
    
public slots:
    void accept() override;
    
private slots:
    void onBrowseAdbPathButtonClicked();
    void onBrowseHdcPathButtonClicked();
    
private:
    // 设备工具设置
    QLineEdit* m_adbPathLineEdit;
    QLineEdit* m_hdcPathLineEdit;
    
    // 网络设置
    QSpinBox* m_serverPortSpinBox;
    QCheckBox* m_autoConnectCheckBox;
    
    // 插件设置
    QLineEdit* m_pluginDirectoryLineEdit;
    QPushButton* m_browsePluginDirButton;
    
    // 界面设置
    QComboBox* m_themeComboBox;
    QCheckBox* m_autoRefreshCheckBox;
    QSpinBox* m_refreshIntervalSpinBox;
    
    Settings* m_settings;
    
    void setupUI();
    void loadSettings();
    void saveSettings();
};

### 设置管理器 (SettingsManager)

管理应用程序的所有设置，提供读写配置的统一接口。

```cpp
// app/settings/settingsmanager.h
class SettingsManager : public QObject
{
    Q_OBJECT

public:
    static SettingsManager* instance();
    
    // 设备工具配置
    QString adbPath() const;
    void setAdbPath(const QString& path);
    
    QString hdcPath() const;
    void setHdcPath(const QString& path);
    
    // 网络配置
    int serverPort() const;
    void setServerPort(int port);
    
    bool autoConnect() const;
    void setAutoConnect(bool enable);
    
    // 插件配置
    QString pluginDirectory() const;
    void setPluginDirectory(const QString& directory);
    
    // 界面配置
    QString theme() const;
    void setTheme(const QString& theme);
    
    bool autoRefreshDevices() const;
    void setAutoRefreshDevices(bool enable);
    
    int refreshInterval() const;
    void setRefreshInterval(int seconds);
    
    // 通用方法
    QVariant getValue(const QString& key, const QVariant& defaultValue = QVariant());
    void setValue(const QString& key, const QVariant& value);
    void resetToDefault();
    
private:
    SettingsManager(QObject* parent = nullptr);
    ~SettingsManager();
    
    QSettings* m_settings;
    static SettingsManager* m_instance;
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

ScreenCast版本提供了强大且灵活的插件系统，使开发者能够轻松扩展应用功能，特别是添加对新设备类型的支持。插件系统基于Qt的插件框架实现，支持热插拔和动态加载。

### 插件架构概述

插件系统主要包括以下核心组件：

1. **插件接口**：定义插件必须实现的标准接口
2. **插件管理器**：负责发现、加载、初始化和卸载插件
3. **设备插件**：实现对特定设备类型的支持
4. **插件加载机制**：支持从指定目录动态加载插件

### 插件接口体系

#### 基础插件接口 (PluginInterface)

所有ScreenCast插件都必须实现的基础接口：

```cpp
// middleware/plugin/plugininterface.h
class PluginInterface
{
public:
    virtual ~PluginInterface() = default;
    
    // 插件基本信息
    virtual QString name() const = 0;
    virtual QString version() const = 0;
    virtual QString description() const = 0;
    
    // 插件生命周期管理
    virtual bool initialize() = 0;
    virtual void cleanup() = 0;
    
    // 插件状态查询
    virtual bool isInitialized() const = 0;
    virtual QString errorString() const = 0;
};

Q_DECLARE_INTERFACE(PluginInterface, "com.screencast.PluginInterface/1.1")
```

#### 设备插件接口 (DevicePluginInterface)

用于实现设备支持的专用插件接口：

```cpp
// middleware/plugin/deviceplugininterface.h
class DevicePluginInterface : public PluginInterface
{
public:
    virtual ~DevicePluginInterface() = default;
    
    // 设备代理获取
    virtual DeviceProxy* getDeviceProxy() = 0;
    
    // 设备类型信息
    virtual DeviceType supportedDeviceType() const = 0;
    virtual QString deviceTypeName() const = 0;
    
    // 设备特性检查
    virtual bool supportsFeature(DeviceFeature feature) const = 0;
    virtual QStringList supportedFeatures() const = 0;
    
    // 设备连接测试
    virtual bool canConnectToDevice(const QString& deviceId) const = 0;
};

Q_DECLARE_INTERFACE(DevicePluginInterface, "com.screencast.DevicePluginInterface/1.1")
```

### 插件管理器 (PluginManager)

负责插件的生命周期管理，包括发现、加载、初始化和卸载：

```cpp
// middleware/plugin/pluginmanager.h
class PluginManager : public QObject
{
    Q_OBJECT

public:
    PluginManager(QObject* parent = nullptr);
    ~PluginManager();
    
    // 插件加载相关
    bool loadPlugins(const QString& directory);
    bool loadPlugin(const QString& filename);
    bool unloadPlugin(const QString& pluginName);
    void unloadAllPlugins();
    
    // 插件查询
    QVector<PluginInterface*> loadedPlugins() const;
    QVector<DevicePluginInterface*> devicePlugins() const;
    
    // 特定插件查找
    PluginInterface* findPluginByName(const QString& name) const;
    DevicePluginInterface* findDevicePluginByType(DeviceType type) const;
    
    // 插件状态信息
    QStringList pluginNames() const;
    QStringList loadedPluginPaths() const;
    
    // 错误信息
    QString lastError() const;
    
public slots:
    // 插件重载
    void reloadPlugins();
    
    // 扫描新插件
    void scanForNewPlugins();
    
signals:
    // 插件事件通知
    void pluginLoaded(const QString& name, const QString& version);
    void pluginUnloaded(const QString& name);
    void pluginError(const QString& name, const QString& error);
    void allPluginsLoaded();
    
private:
    // 插件存储
    QVector<QPluginLoader*> m_pluginLoaders;
    QVector<PluginInterface*> m_plugins;
    QVector<DevicePluginInterface*> m_devicePlugins;
    
    // 错误状态
    QString m_lastError;
    
    // 内部方法
    bool initializePlugin(PluginInterface* plugin);
    void registerDevicePlugin(DevicePluginInterface* plugin);
    void unregisterDevicePlugin(DevicePluginInterface* plugin);
    
    // 插件验证
    bool validatePlugin(PluginInterface* plugin);
    bool isPluginAlreadyLoaded(const QString& name);
};
```

### 插件系统工作流程

#### 1. 插件发现与加载

应用启动时，插件管理器会扫描配置的插件目录：

1. 搜索所有`.dll`/`.so`/`.dylib`文件
2. 尝试通过Qt的`QPluginLoader`加载每个文件
3. 检查加载的对象是否实现了`PluginInterface`
4. 调用插件的`initialize()`方法进行初始化
5. 对于设备插件，进行额外的注册和验证

#### 2. 插件使用流程

应用程序通过插件管理器访问已加载的插件：

```cpp
// 示例：使用设备插件扫描设备
PluginManager* pluginManager = PluginManager::instance();
QVector<DevicePluginInterface*> devicePlugins = pluginManager->devicePlugins();

QVector<DeviceInfo> allDevices;
for (auto plugin : devicePlugins) {
    DeviceProxy* proxy = plugin->getDeviceProxy();
    if (proxy) {
        QVector<DeviceInfo> devices = proxy->queryDevices();
        allDevices.append(devices);
    }
}
```

#### 3. 插件开发流程

开发新插件的基本步骤：

1. 创建Qt共享库项目
2. 实现相应的插件接口
3. 使用`Q_PLUGIN_METADATA`宏声明插件
4. 使用`Q_INTERFACES`宏声明实现的接口
5. 编译生成插件文件
6. 将插件文件放入应用的插件目录

### 插件配置与管理

- 插件目录可在设置中配置（默认位于应用程序目录下的`plugins`文件夹）
- 插件加载状态和版本信息显示在设置界面
- 支持禁用特定插件（通过配置文件）
- 支持插件依赖检查和版本兼容性验证

### 新平台支持

添加新平台支持的步骤：

1. **实现 DeviceProxy 接口**
2. **注册到 DeviceManager**
3. **添加平台类型枚举**
4. **更新设备识别逻辑**

### 安全性考虑

- 插件加载前进行签名验证（可选）
- 插件执行在隔离环境中
- 资源访问权限控制
- 异常处理机制确保插件崩溃不影响主应用

## 📊 性能优化

ScreenCast版本在模块化架构的基础上，对性能进行了全面优化，以提供更流畅的用户体验和更低的资源占用。

### 模块化性能优化

1. **按需加载**：采用延迟加载机制，仅在需要时加载相应的模块和插件，减少内存占用
2. **模块隔离**：各模块独立运行，避免相互影响，提高系统稳定性
3. **资源共享**：核心资源在模块间共享，减少重复加载和内存占用

### 视频流处理优化

1. **多线程解码**：将视频解码任务分配到独立线程，避免阻塞UI主线程
2. **硬件加速**：利用GPU加速视频解码和渲染，支持OpenGL和DirectX
3. **自适应质量调整**：根据设备性能和网络状况动态调整视频质量和帧率
4. **帧缓冲优化**：优化帧缓冲策略，减少延迟和卡顿

### 插件系统性能

1. **插件懒加载**：插件仅在首次使用时加载，减少启动时间
2. **热插拔支持**：支持在应用运行时加载和卸载插件，无需重启
3. **插件缓存**：已加载插件信息缓存，加速二次启动

### 设备通信优化

1. **连接池管理**：维护设备连接池，减少频繁建立连接的开销
2. **命令批处理**：将多个设备操作合并，减少通信次数
3. **异步通信框架**：基于Qt信号槽机制实现全异步通信，提高响应速度
4. **连接状态监控**：实时监控设备连接状态，快速响应连接变化

### 内存和资源管理

1. **智能内存池**：为频繁创建的对象实现内存池，减少内存碎片
2. **资源自动回收**：使用Qt的智能指针和对象树机制，确保资源自动回收
3. **大图缓存策略**：屏幕图像采用多级缓存，平衡内存占用和访问速度
4. **内存使用监控**：内置内存监控工具，及时发现和解决内存泄漏

### 启动性能优化

1. **核心启动流程优化**：精简启动流程，优先加载必要组件
2. **配置预加载**：应用配置异步预加载，避免阻塞启动
3. **界面渐进式加载**：界面组件分批次加载和渲染，提高首屏显示速度

### 跨平台性能适配

1. **平台特定优化**：针对不同操作系统进行性能调优
2. **CPU架构适配**：优化代码以适应不同CPU架构（x86、ARM等）
3. **资源管理策略调整**：根据平台特性调整资源管理策略

---

**ScreenCast 的架构设计注重模块化、可扩展性和高性能，为项目的长期发展奠定了坚实的基础。**