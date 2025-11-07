#pragma once

#include "OHOSDevice.h"
#include "plugin/IDevicePlugin.h"

class OHOSDevicePlugin : public QObject, public IDevicePlugin
{
    Q_OBJECT
    Q_INTERFACES(IDevicePlugin)
    Q_PLUGIN_METADATA(IID IDevicePlugin_iid)

public:
    OHOSDevicePlugin(QObject* parent = nullptr);
    ~OHOSDevicePlugin() override = default;

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