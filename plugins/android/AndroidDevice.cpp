#include "AndroidDevice.h"

#include "utils/Shell.h"

#include <QDebug>
#include <QFile>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QTemporaryFile>
#include <QtConcurrent/QtConcurrent>

AndroidDevice::AndroidDevice(QObject* parent) : DeviceProxy(parent)
{
    qDebug() << "AndroidDevice::AndroidDevice";
}

QVector<DeviceInfo> AndroidDevice::listDevices()
{
    QVector<DeviceInfo> devices;
    QString output;
    if (!Shell::executeCommandQuiet("adb", {"devices"}, &output))
    {
        return devices;
    }

    QStringList lines = output.split('\n', Qt::SkipEmptyParts);
    for (const QString& line : lines)
    {
        if (line.contains("\tdevice"))
        {
            QString serial = line.section('\t', 0, 0).trimmed();
            DeviceInfo info;
            info.type = DeviceType::Android;
            info.serial = serial;
            devices.append(info);
        }
    }
    return devices;
}

QString AndroidDevice::deviceModel(const QString& serial)
{
    QString output;
    if (Shell::executeCommandQuiet("adb", {"-s", serial, "shell", "getprop", "ro.product.model"}, &output))
    {
        return output.trimmed();
    }
    return QString();
}

QString AndroidDevice::deviceName(const QString& serial)
{
    QString model = deviceModel(serial);
    qDebug() << "AndroidDevice::deviceName - serial:" << serial << "model:" << model;

    // If model is empty, use default name with serial number
    if (model.isEmpty())
    {
        model = QString("Android Device (%1)").arg(serial.left(8));
        qDebug() << "AndroidDevice::deviceName - using fallback name:" << model;
    }

    return model;
}

QSize AndroidDevice::deviceResolution(const QString& serial)
{
    QString output;
    if (!Shell::executeCommandQuiet("adb", {"-s", serial, "shell", "wm", "size"}, &output))
    {
        return QSize();
    }

    QRegularExpression re(R"(Physical size:\s*(\d+)x(\d+))");
    QRegularExpressionMatch match = re.match(output);
    if (match.hasMatch())
    {
        int width = match.captured(1).toInt();
        int height = match.captured(2).toInt();
        return QSize(width, height);
    }
    return QSize();
}

DeviceType AndroidDevice::deviceType() const
{
    return DeviceType::Android;
}

bool AndroidDevice::setupMirrorServer(const QString& serial, int forwardPort)
{
    qDebug() << "AndroidDevice::setupMirrorServer: " << serial << forwardPort;
    QMutexLocker locker(&m_mutex);

    if (!pushResourceToDevice(serial, ":/android/server", "/data/local/tmp/mirror_server"))
    {
        qWarning() << "Failed to push mirror_server to device";
        return false;
    }

    // Forward port
    QString* output = nullptr;
    QString* error = nullptr;
    Shell::executeCommand(
        "adb", {"-s", serial, "forward", "--remove", QString("tcp:%1").arg(forwardPort)}, output, error);

    if (!Shell::executeCommand(
            "adb", {"-s", serial, "forward", QString("tcp:%1").arg(forwardPort), "tcp:12345"}, output, error))
    {
        qWarning() << "Failed to forward port for Android device";
        return false;
    }

    return true;
}

bool AndroidDevice::startMirrorServer(const QString& serial)
{
    qDebug() << "AndroidDevice::startMirrorServer: " << serial;
    QMutexLocker locker(&m_mutex);

    // Stop existing server
    QString* output = nullptr;
    QString* error = nullptr;
    Shell::executeCommand("adb", {"-s", serial, "shell", "pkill", "-9", "mirror_server"}, output, error);
    Shell::executeCommand("adb", {"-s", serial, "shell", "pkill", "-9", "screenrecord"}, output, error);

    m_running = true;

    // Start server process
    [[maybe_unused]] auto future1 = QtConcurrent::run([=]() {
        QStringList args = {"-s", serial, "shell", "/data/local/tmp/mirror_server"};
        QProcess process;
        process.start("adb", args);
        while (m_running && !process.waitForFinished(1000))
        {
            // Keep running
        }
        process.kill();
    });

    // Check server status
    [[maybe_unused]] auto future2 = QtConcurrent::run([=]() {
        for (int i = 0; i < 30 && m_running; ++i)
        {
            QString output;
            if (Shell::executeCommandQuiet("adb", {"-s", serial, "shell", "netstat", "-ltn"}, &output) &&
                output.contains(":12345"))
            {
                emit serverStarted(serial);
                return;
            }
            QThread::sleep(1);
        }
    });

    return true;
}

bool AndroidDevice::stopMirrorServer(const QString& serial)
{
    qDebug() << "AndroidDevice::stopMirrorServer: " << serial;
    QMutexLocker locker(&m_mutex);
    m_running = false;

    QString* output = nullptr;
    QString* error = nullptr;
    Shell::executeCommand("adb", {"-s", serial, "shell", "pkill", "-9", "mirror_server"}, output, error);
    Shell::executeCommand("adb", {"-s", serial, "shell", "pkill", "-9", "screenrecord"}, output, error);

    emit serverStopped(serial);
    return true;
}

int AndroidDevice::deviceRotation(const QString& serial)
{
    QString output;
    // 使用 dumpsys window 命令获取屏幕旋转信息
    if (!Shell::executeCommandQuiet("adb", {"-s", serial, "shell", "dumpsys", "window", "displays"}, &output))
    {
        return 0; // 默认返回0度
    }

    // 查找 rotation 值
    QRegularExpression re(R"(rotation=([0-3]))");
    QRegularExpressionMatch match = re.match(output);
    if (match.hasMatch())
    {
        int rotation = match.captured(1).toInt();
        // Android返回0-3的旋转值，对应0、90、180、270度
        switch (rotation)
        {
            case 0:
                return 0;
            case 1:
                return 90;
            case 2:
                return 180;
            case 3:
                return 270;
            default:
                return 0;
        }
    }
    return 0;
}

bool AndroidDevice::queryDeviceInfo(const QString& serial, DeviceInfo& info)
{
    info.serial = serial;
    info.type = DeviceType::Android;
    info.model = deviceModel(serial);
    info.name = deviceName(serial);
    QSize resolution = deviceResolution(serial);
    if (resolution.isValid())
    {
        // 保留原始分辨率信息，不进行旋转调整
        info.width = resolution.width();
        info.height = resolution.height();
    }
    info.rotation = deviceRotation(serial);
    return true;
}
bool AndroidDevice::sendEvent(const DeviceInfo& dev, DeviceEvent event)
{
    // 定义 DeviceEvent 到命令模板的映射
    static const std::map<DeviceEvent, QString> eventMap = {
        {DeviceEvent::BACK, "input keyevent KEYCODE_BACK"},
        {DeviceEvent::HOME, "input keyevent KEYCODE_HOME"},
        {DeviceEvent::MENU, "input keyevent KEYCODE_APP_SWITCH"},
        {DeviceEvent::UNLOCK, "input swipe 500 1800 500 800 300"},
        {DeviceEvent::SHUTDOWN, "reboot -p"},
        {DeviceEvent::REBOOT, "reboot"},
        {DeviceEvent::POWER, "input keyevent KEYCODE_POWER"},
        {DeviceEvent::WAKEUP, "input keyevent KEYCODE_WAKEUP"},
        {DeviceEvent::SLEEP, "input keyevent KEYCODE_SLEEP"},
        {DeviceEvent::VOLUME_UP, "input keyevent KEYCODE_VOLUME_UP"},
        {DeviceEvent::VOLUME_DOWN, "input keyevent KEYCODE_VOLUME_DOWN"},
        {DeviceEvent::MUTE, "input keyevent KEYCODE_MUTE"},
        {DeviceEvent::ROTATE, "settings put system accelerometer_rotation 0"},
        {DeviceEvent::ROTATE_LOCK, "settings put system accelerometer_rotation 0"},
    };

    auto it = eventMap.find(event);
    if (it == eventMap.end() || it->second.isEmpty())
    {
        qWarning() << "adb handle event failed: not implement :" << static_cast<int>(event);
        return false;
    }

    // 如果屏幕关闭且当前事件不是唤醒事件，则不处理并返回成功
    if (isScreenOn(dev.serial))
    {
        qWarning() << "screen is on";
        if (event == DeviceEvent::WAKEUP)
        {
            return true;
        }
    }
    else
    {
        qWarning() << "screen is off";
        if (event != DeviceEvent::WAKEUP)
        {
            return false;
        }
    }

    QStringList args = {"-s", dev.serial, "shell"};
    args += it->second.split(' ', Qt::SkipEmptyParts);
    QString* output = nullptr;
    QString* error = nullptr;
    if (!Shell::executeCommand("adb", args, output, error))
    {
        qWarning() << "adb send event failed";
        return false;
    }
    if (event == DeviceEvent::ROTATE)
    {
        int rotation = 0;
        QString settingsOutput;
        QString* errorOutput = nullptr;
        if (Shell::executeCommandQuiet("adb",
                                       {"-s", dev.serial, "shell", "settings", "get", "system", "user_rotation"},
                                       &settingsOutput,
                                       errorOutput))
        {
            rotation = settingsOutput.toInt();
        }
        if (!Shell::executeCommand(
                "adb",
                {"-s", dev.serial, "shell", "settings", "put", "system", "user_rotation", QString::number(rotation)},
                output,
                error))
        {
            qWarning() << "adb send rotate event failed";
            return false;
        }
    }
    return true;
}

bool AndroidDevice::sendTouchEvent(const DeviceInfo& dev, QPoint pos)
{
    QString* output = nullptr;
    QString* error = nullptr;
    return Shell::executeCommand(
        "adb",
        {"-s", dev.serial, "shell", "input", "tap", QString::number(pos.x()), QString::number(pos.y())},
        output,
        error);
}

bool AndroidDevice::sendTextEvent(const DeviceInfo& dev, const QString& text)
{
    QString* output = nullptr;
    QString* error = nullptr;
    return Shell::executeCommand("adb", {"-s", dev.serial, "shell", "input", "text", text}, output, error);
}

bool AndroidDevice::sendSwipeEvent(const DeviceInfo& dev, QPoint start, QPoint end, int duration)
{
    QString* output = nullptr;
    QString* error = nullptr;
    return Shell::executeCommand("adb",
                                 {"-s",
                                  dev.serial,
                                  "shell",
                                  "input",
                                  "swipe",
                                  QString::number(start.x()),
                                  QString::number(start.y()),
                                  QString::number(end.x()),
                                  QString::number(end.y()),
                                  QString::number(duration)},
                                 output,
                                 error);
}

bool AndroidDevice::screenshot(const QString& serial, QByteArray& imageData)
{
    if (!Shell::executeCommand("adb", {"-s", serial, "exec-out", "screencap", "-p"}, &imageData))
    {
        qWarning() << "Failed to execute screencap command";
        return false;
    }
    return true;
}

bool AndroidDevice::supportEvent(DeviceEvent event) const
{
    Q_UNUSED(event);
    return true; // Android 支持所有事件类型
}

bool AndroidDevice::pushResourceToDevice(const QString& serial, const QString& resourcePath, const QString& devicePath)
{
    QFile file(resourcePath);
    if (!file.open(QIODevice::ReadOnly))
    {
        qWarning() << "Failed to open resource:" << resourcePath;
        return false;
    }

    QTemporaryFile tempFile;
    if (!tempFile.open())
    {
        qWarning() << "Failed to create temporary file";
        return false;
    }

    tempFile.write(file.readAll());
    tempFile.close();

    QString output;
    Shell::executeCommand(
        "adb", {"-s", serial, "push", QDir::toNativeSeparators(tempFile.fileName()), devicePath}, &output);
    output.clear();
    Shell::executeCommand("adb", {"-s", serial, "shell", "chmod", "755", devicePath}, &output);

    return true;
}

bool AndroidDevice::isScreenOn(const QString& serial)
{
    QString output;
    if (!Shell::executeCommandQuiet("adb", {"-s", serial, "shell", "dumpsys", "display"}, &output))
    {
        qWarning() << "Failed to execute dumpsys display command";
        return false;
    }
    QStringList lines = output.split('\n');
    for (const QString& line : lines)
    {
        if (line.contains("mState="))
        {
            QString state = line.split('=').last().trimmed();
            return state.compare("ON", Qt::CaseInsensitive) == 0;
        }
    }

    qWarning() << "mState not found in dumpsys output";
    return false;
}

QString AndroidDevice::getMirrorServerIp(const QString& serial)
{
    Q_UNUSED(serial);
    return QString("127.0.0.1");
}