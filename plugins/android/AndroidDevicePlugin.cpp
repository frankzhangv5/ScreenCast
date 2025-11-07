#include "AndroidDevicePlugin.h"

#include <QCoreApplication>
#include <QDir>
#include <QProcessEnvironment>
#include <QStandardPaths>

AndroidDevicePlugin::AndroidDevicePlugin(QObject* parent) : QObject(parent) {}

QString AndroidDevicePlugin::pluginName() const
{
    return "Android";
}

QString AndroidDevicePlugin::pluginVersion() const
{
    return "1.0.0";
}

QIcon AndroidDevicePlugin::pluginIcon() const
{
    return QIcon(":/android/icon");
}

DeviceProxy* AndroidDevicePlugin::createDeviceProxy(QObject* parent)
{
    return new AndroidDevice(parent);
}

DeviceType AndroidDevicePlugin::deviceType() const
{
    return DeviceType::Android;
}

bool AndroidDevicePlugin::checkDriverAvailable() const
{
    QString adbPath = getDriverPath();
    if (adbPath.isEmpty())
    {
        return false;
    }

    QString currentPath = qgetenv("PATH");
    QStringList pathList = currentPath.split(QLatin1Char(':'), Qt::SkipEmptyParts);
    QString adbDir = QFileInfo(adbPath).absolutePath();
    if (!pathList.contains(adbDir))
    {
        pathList.prepend(adbDir);
    }
    QString newPath = pathList.join(QLatin1Char(':'));
    qputenv("PATH", newPath.toUtf8());

    return true;
}

QString AndroidDevicePlugin::description() const
{
    return "Android device support plugin";
}

QString AndroidDevicePlugin::author() const
{
    return "ZhangFeng";
}

QString AndroidDevicePlugin::getDriverName() const
{
    return "adb";
}

QString AndroidDevicePlugin::getDriverPath() const
{
    // 1. Check environment variables
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    QString path = env.value("ANDROID_HOME");
    if (!path.isEmpty())
    {
        QString adbPath = QDir(path).filePath("platform-tools/adb");
        if (QFile::exists(adbPath))
        {
            return adbPath;
        }
    }

    // 2. Check adb in PATH
    QString adbPath = QStandardPaths::findExecutable("adb");
    if (!adbPath.isEmpty())
    {
        return adbPath;
    }

    // 3. Check application directory
    QString appDir = QCoreApplication::applicationDirPath();
    adbPath = QDir(appDir).filePath("adb");
    if (QFile::exists(adbPath))
    {
        return adbPath;
    }

#ifdef Q_OS_WIN
    adbPath += ".exe";
    if (QFile::exists(adbPath))
    {
        return adbPath;
    }
#endif

#ifdef Q_OS_MAC
    QStringList adbPaths = {"/usr/local/bin/adb",
                            "/opt/homebrew/bin/adb",
                            "/opt/local/bin/adb",
                            QDir::homePath() + "/Library/Android/sdk/platform-tools/adb",
                            QDir::homePath() + "/Android/Sdk/platform-tools/adb"};
    for (const QString& path : adbPaths)
    {
        if (QFile::exists(path))
        {
            return path;
        }
    }
#endif

    return QString();
}