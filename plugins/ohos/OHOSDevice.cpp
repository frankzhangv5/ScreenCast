#include "OHOSDevice.h"

#include "utils/Shell.h"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QTemporaryFile>
#include <QtConcurrent/QtConcurrent>

OHOSDevice::OHOSDevice(QObject* parent) : DeviceProxy(parent)
{
    qDebug() << "OHOSDevice::OHOSDevice";
}
QVector<DeviceInfo> OHOSDevice::listDevices()
{
    QMutexLocker locker(&m_mutex);
    QVector<DeviceInfo> devices;

    QString output;
    if (!Shell::executeQuiet("hdc list targets", &output))
    {
        return devices;
    }

    QStringList lines = output.split('\n', Qt::SkipEmptyParts);
    for (const QString& line : lines)
    {
        QString serial = line.trimmed();
        if (!serial.isEmpty() && serial != "[Empty]")
        {
            DeviceInfo info;
            info.type = DeviceType::OHOS;
            info.serial = serial;
            devices.append(info);
        }
    }
    return devices;
}

QString OHOSDevice::deviceModel(const QString& serial)
{
    QString output;
    QString command = QString("hdc -t %1 shell param get const.product.model").arg(serial);
    if (Shell::executeQuiet(command, &output))
    {
        return output.trimmed();
    }
    return QString();
}

QString OHOSDevice::deviceName(const QString& serial)
{
    QString output;
    QString command = QString("hdc -t %1 shell param get const.product.name").arg(serial);
    if (Shell::executeQuiet(command, &output))
    {
        QString name = output.trimmed();
        qDebug() << "OHOSDevice::deviceName - serial:" << serial << "name:" << name;
        if (!name.isEmpty())
        {
            return name;
        }
    }

    // If name cannot be obtained, try using model
    QString model = deviceModel(serial);
    qDebug() << "OHOSDevice::deviceName - using model as fallback:" << model;

    // If model is also empty, use default name with serial number
    if (model.isEmpty())
    {
        model = QString("OHOS Device (%1)").arg(serial.left(8));
        qDebug() << "OHOSDevice::deviceName - using default fallback name:" << model;
    }

    return model;
}

QSize OHOSDevice::deviceResolution(const QString& serial)
{
    QString output;
    QString command = QString("hdc -t %1 shell hidumper -s RenderService -a screen").arg(serial);
    if (!Shell::executeQuiet(command, &output))
    {
        return QSize();
    }

    QRegularExpression re(R"(physical resolution=(\d+)x(\d+))");
    QRegularExpressionMatch match = re.match(output);
    if (match.hasMatch())
    {
        int width = match.captured(1).toInt();
        int height = match.captured(2).toInt();
        return QSize(width, height);
    }
    return QSize();
}

DeviceType OHOSDevice::deviceType() const
{
    return DeviceType::OHOS;
}

bool OHOSDevice::setupMirrorServer(const QString& serial, int forwardPort)
{
    QMutexLocker locker(&m_mutex);

    if (!pushResourceToDevice(serial, ":/ohos/server", "/data/local/tmp/mirror_server"))
    {
        qWarning() << "Failed to push mirror_server to device";
        return false;
    }

    // Forward port
    QString removeForwardCommand = QString("hdc -t %1 fport rm tcp:%2 tcp:12345").arg(serial).arg(forwardPort);
    Shell::executeQuiet(removeForwardCommand);

    QString forwardCommand = QString("hdc -t %1 fport tcp:%2 tcp:12345").arg(serial).arg(forwardPort);
    if (!Shell::execute(forwardCommand))
    {
        qWarning() << "Failed to forward port for OHOS device";
        return false;
    }
    return true;
}

bool OHOSDevice::startMirrorServer(const QString& serial)
{
    QMutexLocker locker(&m_mutex);

    // Stop existing server
    QString killCommand = QString("hdc -t %1 shell pkill -9 mirror_server").arg(serial);
    Shell::executeQuiet(killCommand);

    m_running = true;

    // Start server process
    [[maybe_unused]] auto future1 = QtConcurrent::run([=]() {
        QStringList args = {"-t", serial, "shell", "/data/local/tmp/mirror_server"};
        QProcess process;
        process.start("hdc", args);
        while (m_running && !process.waitForFinished(1000))
        {
            // Keep running
        }
        process.kill();
    });

    // Check server status
    [[maybe_unused]] auto future2 = QtConcurrent::run([=]() {
        QString command = QString("hdc -t %1 shell netstat -ltn | grep ':12345'").arg(serial);
        for (int i = 0; i < 30 && m_running; ++i)
        {
            QString output;
            if (Shell::executeQuiet(command, &output) && output.contains(":12345"))
            {
                emit serverStarted(serial);
                return;
            }
            QThread::sleep(1);
        }
    });

    return true;
}

bool OHOSDevice::stopMirrorServer(const QString& serial)
{
    QMutexLocker locker(&m_mutex);
    m_running = false;

    QString killCommand = QString("hdc -t %1 shell pkill -9 mirror_server").arg(serial);
    Shell::executeQuiet(killCommand);

    emit serverStopped(serial);
    return true;
}

bool OHOSDevice::queryDeviceInfo(const QString& serial, DeviceInfo& info)
{
    info.serial = serial;
    info.type = DeviceType::OHOS;
    info.model = deviceModel(serial);
    info.name = deviceName(serial);
    QSize resolution = deviceResolution(serial);
    info.rotation = deviceRotation(serial);
    if (resolution.isValid())
    {
        // 根据旋转角度调整宽和高
        info.width = resolution.width();
        info.height = resolution.height();
    }
    return true;
}

bool OHOSDevice::sendEvent(const DeviceInfo& dev, DeviceEvent event)
{
    // 定义事件与命令的映射
    // clang-format off
    static const std::map<DeviceEvent, QString> eventMap = {
        {DeviceEvent::HOME, "uinput -K -d 1 -u 1"},
        {DeviceEvent::BACK, "uinput -K -d 2 -u 2"},
        {DeviceEvent::MENU, "uinput -K -d 2078 -u 2078"},
        {DeviceEvent::WAKEUP, "power-shell wakeup"},
        {DeviceEvent::SLEEP, "power-shell suspend"},
        {DeviceEvent::ROTATE, ""},
        {DeviceEvent::UNLOCK, "uinput -T -m 500 1000 500 500 100"},
        {DeviceEvent::SHUTDOWN, "poweroff -f"},
        {DeviceEvent::REBOOT, "reboot"}
    };
    // clang-format on

    auto it = eventMap.find(event);
    if (it == eventMap.end() || it->second.isEmpty())
    {
        qWarning() << "hdc handle event failed: not implement :" << static_cast<int>(event);
        return false;
    }

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

    // 执行时添加 hdc -t serial shell 前缀
    QString command = QString("hdc -t %1 shell %2").arg(dev.serial, it->second);
    if (!Shell::execute(command))
    {
        qWarning() << "hdc send event failed: " << command;
        return false;
    }
    return true;
}

bool OHOSDevice::sendTouchEvent(const DeviceInfo& dev, QPoint pos)
{
    // 实现简单的反向缩放逻辑
    QString command = QString("hdc -t %1 shell uinput -T -c %2 %3").arg(dev.serial).arg(pos.x()).arg(pos.y());
    return Shell::execute(command);
}

bool OHOSDevice::sendTextEvent(const DeviceInfo& dev, const QString& text)
{
    QString escapedText = text;
    escapedText.replace("\"", "\\\"");

    QString command = QString("hdc -t %1 shell uinput -k -s \"%2\"").arg(dev.serial).arg(escapedText);
    return Shell::execute(command);
}

bool OHOSDevice::sendSwipeEvent(const DeviceInfo& dev, QPoint start, QPoint end, int duration)
{
    // 实现简单的反向缩放逻辑
    QString command = QString("hdc -t %1 shell uinput -T -m %2 %3 %4 %5 %6")
                          .arg(dev.serial)
                          .arg(start.x())
                          .arg(start.y())
                          .arg(end.x())
                          .arg(end.y())
                          .arg(duration);
    return Shell::execute(command);
}

bool OHOSDevice::screenshot(const QString& serial, QByteArray& imageData)
{
    QMutexLocker locker(&m_mutex);
    const QString timestamp = QDateTime::currentDateTime().toString("yyyyMMddhhmmss");
    const QString tempPath = QString("/data/local/tmp/screenshot_%1.jpeg").arg(timestamp);
    const QString localPath =
        QDir::toNativeSeparators(QDir::tempPath() + QDir::separator() + QString("screenshot_%1.jpeg").arg(timestamp));

    // 截取屏幕截图
    QString command = QString("hdc -t %1 shell snapshot_display -f %2").arg(serial, tempPath);
    if (!Shell::execute(command))
    {
        qWarning() << "hdc screencap failed";
        return false;
    }

    // 拉取截图到本地
    command = QString("hdc -t %1 file recv %2 %3").arg(serial, tempPath, localPath);
    if (!Shell::execute(command))
    {
        qWarning() << "hdc pull screenshot failed";
        return false;
    }

    // 删除设备上的临时截图
    command = QString("hdc -t %1 shell rm -f %2").arg(serial, tempPath);
    Shell::executeQuiet(command);

    // 读取图像数据并清理本地文件
    QFile tempFile(localPath);
    if (tempFile.open(QIODevice::ReadOnly))
    {
        imageData = tempFile.readAll();
        tempFile.close();
        QFile::remove(localPath);
        return true;
    }

    return false;
}

bool OHOSDevice::supportEvent(DeviceEvent event) const
{
    return event != DeviceEvent::ROTATE;
}

bool OHOSDevice::pushResourceToDevice(const QString& serial, const QString& resourcePath, const QString& devicePath)
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

    QString command =
        QString("hdc -t %1 file send %2 %3").arg(serial, QDir::toNativeSeparators(tempFile.fileName()), devicePath);
    QString output;
    if (!Shell::execute(command, &output))
    {
        qWarning() << "Failed to push resource to device:" << output;
        return false;
    }

    command = QString("hdc -t %1 shell chmod 755 %2").arg(serial, devicePath);
    output.clear();
    Shell::executeQuiet(command, &output);

    return true;
}

int OHOSDevice::deviceRotation(const QString& serial)
{
    QString command = QString("hdc -t %1 shell hidumper -s RenderService -a screen").arg(serial);
    QString output;
    if (Shell::executeQuiet(command, &output))
    {
        // 尝试从输出中解析旋转角度
        QRegularExpression re("rotation=([0-9]+)");
        QRegularExpressionMatch match = re.match(output);
        if (match.hasMatch())
        {
            int rotation = match.captured(1).toInt();
            // OHOS设备通常返回0、90、180、270等角度值
            return rotation;
        }
    }
    // 默认返回0度
    return 0;
}

bool OHOSDevice::isScreenOn(const QString& serial)
{
    QString command = QString("hdc -t %1 shell hidumper -s RenderService -a screen").arg(serial);
    QString output;
    if (Shell::executeQuiet(command, &output))
    {
        return output.contains("powerStatus=POWER_STATUS_ON");
    }
    return false;
}

QString OHOSDevice::getMirrorServerIp(const QString& serial)
{
    Q_UNUSED(serial);
    return QString("127.0.0.1");
}