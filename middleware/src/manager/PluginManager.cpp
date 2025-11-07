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
    return QString("/usr/lib/%1/plugins").arg(QApplication::applicationName());
#endif
}