#include "manager/PluginManager.h"

#include <QCoreApplication>
#include <QDebug>

PluginManager& PluginManager::instance()
{
    static PluginManager instance;
    return instance;
}

PluginManager::PluginManager(QObject* parent) : QObject(parent) {}

PluginManager::~PluginManager()
{
    unloadAllPlugins();
}

void PluginManager::loadAllPlugins()
{
    QDir pluginDir(getPluginDirectory());
    if (!pluginDir.exists())
    {
        qWarning() << "Plugin directory does not exist:" << pluginDir.absolutePath();
        return;
    }
    qDebug() << "Plugin directory: " << pluginDir.absolutePath();

    const QStringList filters = {
        "*.dll",  // Windows
        "*.so",   // Linux
        "*.dylib" // macOS
    };

    const QStringList pluginFiles = pluginDir.entryList(filters, QDir::Files);
    qDebug() << "Plugin files: " << pluginFiles;
    for (const QString& fileName : pluginFiles)
    {
        const QString filePath = pluginDir.absoluteFilePath(fileName);
        loadPlugin(filePath);
    }
}

void PluginManager::unloadAllPlugins()
{
    for (auto it = m_loaders.begin(); it != m_loaders.end(); ++it)
    {
        IDevicePlugin* plugin = m_plugins[it.key()];
        if (plugin)
        {
            emit pluginUnloaded(it.key());
        }
        it.value()->unload();
    }

    m_loaders.clear();
    m_plugins.clear();
}

void PluginManager::loadPlugin(const QString& filePath)
{
    QSharedPointer<QPluginLoader> loader(new QPluginLoader(filePath));

    if (!loader->load())
    {
        emit pluginLoadError(loader->errorString());
        qWarning() << "Failed to load plugin:" << filePath << "-" << loader->errorString();
        return;
    }

    QObject* instance = loader->instance();
    if (!instance)
    {
        emit pluginLoadError("Failed to create plugin instance");
        qWarning() << "Failed to create plugin instance:" << filePath;
        return;
    }

    IDevicePlugin* plugin = qobject_cast<IDevicePlugin*>(instance);
    if (!plugin)
    {
        emit pluginLoadError("Invalid plugin interface");
        qWarning() << "Invalid plugin interface:" << filePath;
        loader->unload();
        return;
    }

    DeviceType type = plugin->deviceType();

    // Check if plugin of the same type is already loaded
    if (m_plugins.contains(type))
    {
        qWarning() << "Plugin for device type" << static_cast<int>(type) << "already loaded, skipping";
        loader->unload();
        return;
    }

    m_loaders[type] = loader;
    m_plugins[type] = plugin;

    emit pluginLoaded(type);
    qDebug() << "Successfully loaded plugin:" << plugin->pluginName() << "for type" << static_cast<int>(type);
}

IDevicePlugin* PluginManager::getPlugin(DeviceType type) const
{
    return m_plugins.value(type, nullptr);
}

QList<IDevicePlugin*> PluginManager::getAllPlugins() const
{
    return m_plugins.values();
}

bool PluginManager::hasPluginForType(DeviceType type) const
{
    return m_plugins.contains(type);
}

QList<DeviceType> PluginManager::getSupportedDeviceTypes() const
{
    return m_plugins.keys();
}

void PluginManager::rescanPlugins()
{
    unloadAllPlugins();
    loadAllPlugins();
}

QString PluginManager::getPluginDirectory() const
{
    QDir appDir(QCoreApplication::applicationDirPath());

#ifdef Q_OS_WIN

    appDir.cd("plugins");
    return appDir.absolutePath();
#    elifdef Q_OS_MAC
    if (appDir.dirName() == "MacOS")
    {
        appDir.cdUp();
        appDir.cd("PlugIns");
    }
    return appDir.absolutePath();
#else
    // 检查是否在AppImage环境中运行
    const char* appImageEnv = qgetenv("APPIMAGE").constData();
    const char* argv0Env = qgetenv("ARGV0").constData();
    const char* qtPluginPath = qgetenv("QT_QPA_PLATFORM_PLUGIN_PATH").constData();

    // 检查是否是AppImage环境
    bool isAppImage = appImageEnv || (argv0Env && QString(argv0Env).contains(".AppImage")) ||
                      appDir.absolutePath().contains(".AppImage");

    // 如果是AppImage环境，使用与打包脚本一致的插件路径结构
    if (isAppImage)
    {
        // 首先尝试使用QT_QPA_PLATFORM_PLUGIN_PATH环境变量的父目录
        if (qtPluginPath && QDir(QString(qtPluginPath)).exists())
        {
            return QString(qtPluginPath);
        }

        // 检查可执行文件是否在usr/bin目录中（标准AppImage结构）
        QDir parentDir = appDir;
        if (appDir.dirName() == "bin" && parentDir.cdUp() && parentDir.dirName() == "usr")
        {
            // 在AppImage标准结构中，插件位于usr/plugins
            return parentDir.absoluteFilePath("plugins");
        }

        // 备选方案：相对于可执行文件的plugins目录
        appDir.cd("plugins");
        if (appDir.exists())
        {
            return appDir.absolutePath();
        }
    }

    // 常规Linux安装的路径
    return QString("/usr/lib/%1/plugins").arg(QCoreApplication::applicationName());
#endif
}