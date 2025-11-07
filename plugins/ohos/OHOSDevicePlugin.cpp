#include "OHOSDevicePlugin.h"

#include <QCoreApplication>
#include <QDir>
#include <QProcessEnvironment>
#include <QStandardPaths>

OHOSDevicePlugin::OHOSDevicePlugin(QObject* parent) : QObject(parent) {}

QString OHOSDevicePlugin::pluginName() const
{
    return "OpenHarmony";
}

QString OHOSDevicePlugin::pluginVersion() const
{
    return "1.0.0";
}

QIcon OHOSDevicePlugin::pluginIcon() const
{
    return QIcon(":/ohos/icon");
}

DeviceProxy* OHOSDevicePlugin::createDeviceProxy(QObject* parent)
{
    return new OHOSDevice(parent);
}

DeviceType OHOSDevicePlugin::deviceType() const
{
    return DeviceType::OHOS;
}

bool OHOSDevicePlugin::checkDriverAvailable() const
{
    QString hdcPath = getDriverPath();
    if (hdcPath.isEmpty())
    {
        return false;
    }

    QString currentPath = qgetenv("PATH");
    QStringList pathList = currentPath.split(QLatin1Char(':'), Qt::SkipEmptyParts);
    QString hdcDir = QFileInfo(hdcPath).absolutePath();
    if (!pathList.contains(hdcDir))
    {
        pathList.prepend(hdcDir);
    }
    QString newPath = pathList.join(QLatin1Char(':'));
    qputenv("PATH", newPath.toUtf8());

    return true;
}

QString OHOSDevicePlugin::description() const
{
    return "OpenHarmony device support plugin";
}

QString OHOSDevicePlugin::author() const
{
    return "ZhangFeng";
}

QString OHOSDevicePlugin::getDriverName() const
{
    return "hdc";
}

QString OHOSDevicePlugin::getDriverPath() const
{
    // 1. Check environment variables
    QString hdcPath = QStandardPaths::findExecutable("hdc");
    if (!hdcPath.isEmpty())
    {
        return hdcPath;
    }

    // 2. Check application directory
    QString appDir = QCoreApplication::applicationDirPath();
    hdcPath = QDir(appDir).filePath("hdc");
    if (QFile::exists(hdcPath))
    {
        return hdcPath;
    }

#ifdef Q_OS_WIN
    hdcPath += ".exe";
    if (QFile::exists(hdcPath))
    {
        return hdcPath;
    }
#endif

#ifdef Q_OS_MAC
    QStringList hdcPaths = {"/usr/local/bin/hdc",
                            "/opt/homebrew/bin/hdc",
                            "/opt/local/bin/hdc",
                            QDir::homePath() + "/Library/DevEcoStudio/Sdk/toolchains/hdc"};
    for (const QString& path : hdcPaths)
    {
        if (QFile::exists(path))
        {
            return path;
        }
    }
#endif

    return QString();
}