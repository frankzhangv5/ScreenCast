#pragma once

#include "AndroidDevice.h"
#include "plugin/IDevicePlugin.h"

class AndroidDevicePlugin : public QObject, public IDevicePlugin
{
    Q_OBJECT
    Q_INTERFACES(IDevicePlugin)
    Q_PLUGIN_METADATA(IID "IDevicePlugin")

public:
    AndroidDevicePlugin(QObject* parent = nullptr);
    ~AndroidDevicePlugin() override = default;

    QString pluginName() const override;
    QString pluginVersion() const override;
    QIcon pluginIcon() const override;

    DeviceProxy* createDeviceProxy(QObject* parent = nullptr) override;

    DeviceType deviceType() const override;

    bool checkDriverAvailable() const override;
    QString description() const override;
    QString author() const override;
    QString getDriverName() const override;
    QString getDriverPath() const override;
};