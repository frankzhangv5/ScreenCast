#pragma once

#include <QDir>
#include <QMap>
#include <QObject>
#include <QPluginLoader>
#include <QSharedPointer>
#include <plugin/IDevicePlugin.h>

/**
 * @brief 插件管理器
 *
 * 负责动态加载和管理设备插件
 */
class PluginManager : public QObject
{
    Q_OBJECT

public:
    static PluginManager& instance();

    /**
     * @brief 加载所有插件
     */
    void loadAllPlugins();

    /**
     * @brief 卸载所有插件
     */
    void unloadAllPlugins();

    /**
     * @brief 获取指定类型的插件
     */
    IDevicePlugin* getPlugin(DeviceType type) const;

    /**
     * @brief 获取所有已加载的插件
     */
    QList<IDevicePlugin*> getAllPlugins() const;

    /**
     * @brief 检查是否有插件支持指定设备类型
     */
    bool hasPluginForType(DeviceType type) const;

    /**
     * @brief 获取支持的设备类型列表
     */
    QList<DeviceType> getSupportedDeviceTypes() const;

    /**
     * @brief 重新扫描插件目录
     */
    void rescanPlugins();

signals:
    void pluginLoaded(DeviceType type);
    void pluginUnloaded(DeviceType type);
    void pluginLoadError(const QString& error);

private:
    explicit PluginManager(QObject* parent = nullptr);
    ~PluginManager();

    void loadPlugin(const QString& filePath);
    void unloadPlugin(DeviceType type);

    QString getPluginDirectory() const;

    QMap<DeviceType, QSharedPointer<QPluginLoader>> m_loaders;
    QMap<DeviceType, IDevicePlugin*> m_plugins;

    Q_DISABLE_COPY(PluginManager)
};