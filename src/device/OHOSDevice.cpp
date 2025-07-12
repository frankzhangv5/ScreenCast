#include "device/OHOSDevice.h"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QMutex>
#include <QProcess>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QTemporaryFile>
#include <QtConcurrent/QtConcurrent>

// OpenHarmony device proxy implementation
OHOSDevice::OHOSDevice(QObject* parent) : DeviceProxy(parent) {}

QVector<DeviceInfo> OHOSDevice::queryDevices()
{
    QMutexLocker locker(&m_mutex); // Lock
    QVector<DeviceInfo> devices;
    QString hdcOutput = execCommand("hdc", {"list", "targets"});
    QStringList hdcLines = hdcOutput.split('\n', Qt::SkipEmptyParts);
    for (const QString& line : hdcLines)
    {
        QString serial = line.trimmed();
        if (serial.isEmpty() || serial == "[Empty]")
            continue; // Filter empty lines and [Empty]
        DeviceInfo info;
        info.type = DeviceType::OHOS;
        info.serial = serial;
        devices.append(info);
    }
    return devices;
}

QString OHOSDevice::deviceModel(const QString& serial)
{
    QStringList args = {"-t", serial, "shell", "param", "get", "const.product.model"};
    return execCommand("hdc", args);
}

QString OHOSDevice::deviceName(const QString& serial)
{
    QStringList args = {"-t", serial, "shell", "param", "get", "const.product.name"};
    return execCommand("hdc", args);
}

QSize OHOSDevice::deviceResolution(const QString& serial)
{
    QStringList args = {"-t", serial, "shell", "hidumper", "-s", "RenderService", "-a", "screen"};
    QString output = execCommand("hdc", args);
    if (output.isEmpty())
        return QSize();
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

bool OHOSDevice::setupDeviceServer(const QString& serial, int forwardPort)
{
    QMutexLocker locker(&m_mutex); // Lock
    QFile resFile(":/server/ohos");
    if (!resFile.open(QIODevice::ReadOnly))
    {
        qWarning() << "failed to open file: /server/ohos";
        return false;
    }
    QTemporaryFile tmpFile;
    tmpFile.setAutoRemove(false);
    if (!tmpFile.open())
    {
        qWarning() << "failed to create tmp file";
        return false;
    }
    tmpFile.write(resFile.readAll());
    tmpFile.flush();
    tmpFile.close();
    resFile.close();

    QString localPath = tmpFile.fileName();
    QString remotePath = "/data/local/tmp/screen_server";

    // 2. hdc push
    QStringList pushArgs = {"-t", serial, "file", "send", QDir::toNativeSeparators(localPath), remotePath};
    qDebug() << "hdc push args: " << pushArgs.join(" ");
    QProcess proc;
    proc.start("hdc", pushArgs);
    if (!proc.waitForFinished(30000) || proc.exitCode() != 0)
    {
        qWarning() << "hdc push failed";
        tmpFile.remove();
        return false;
    }

    // 3. hdc shell chmod 755
    QStringList chmodArgs = {"-t", serial, "shell", "chmod", "755", remotePath};
    proc.start("hdc", chmodArgs);
    if (!proc.waitForFinished(30000) || proc.exitCode() != 0)
    {
        qWarning() << "hdc chmod failed";
        tmpFile.remove();
        return false;
    }

    // 4. hdc fport port
    QStringList forwardArgs = {"-t", serial, "fport", QString("tcp:%1").arg(forwardPort), "tcp:12345"};
    proc.start("hdc", forwardArgs);
    if (!proc.waitForFinished(30000) || proc.exitCode() != 0)
    {
        qWarning() << "hdc forward failed";
        tmpFile.remove();
        return false;
    }

    tmpFile.remove();
    qDebug() << "hdc push resource to device success";
    return true;
}

bool OHOSDevice::startDeviceServer(const QString& serial)
{
    QMutexLocker locker(&m_mutex); // Lock
    // Stop screen_server process via pkill
    QStringList args = {"-t", serial, "shell", "pkill", "-9", "screen_server"};
    QProcess proc;
    proc.start("hdc", args);
    if (!proc.waitForFinished(30000) || proc.exitCode() != 0)
    {
        qWarning() << "hdc stop screen_server failed:" << proc.readAllStandardError();
    }

    m_running = true;
    QtConcurrent::run([=]() {
        qDebug() << "hdc start screen_server process";
        QString remotePath = "/data/local/tmp/screen_server";
        QStringList args = {"-t", serial, "shell", remotePath};
        QProcess proc;
        proc.start("hdc", args);
        while (m_running)
        {
            proc.waitForFinished(1000);
        }
        proc.kill();
        proc.close();
        qWarning() << "hdc screen_server exit";
    });

    QtConcurrent::run([=]() {
        qDebug() << "hdc check screen_server start";
        QStringList args = {"-t", serial, "shell", "netstat -ltn | grep ':12345'"};
        QProcess proc;
        while (m_running)
        {
            proc.start("hdc", args);
            if (!proc.waitForFinished(2000))
            {
                qWarning() << "hdc check screen_server fail, check again";
                proc.kill();
                continue;
            }
            QString output = QString::fromLocal8Bit(proc.readAllStandardOutput().trimmed());
            qDebug() << "hdc netstat output:" << output;
            QStringList lines = output.split('\n');
            bool serverFound = false;
            foreach (const QString& line, lines)
            {
                if (line.contains(":12345") && line.contains("LISTEN"))
                {
                    serverFound = true;
                    break;
                }
            }
            if (serverFound)
            {
                qInfo() << "hdc screen_server started";
                emit serverStarted(serial);
                break;
            }
        }
        proc.kill();
        proc.close();
        qDebug() << "hdc check screen_server start done";
    });

    return true;
}

bool OHOSDevice::stopDeviceServer(const QString& serial)
{
    QMutexLocker locker(&m_mutex); // Lock
    m_running = false;

    QtConcurrent::run([=]() {
        qDebug() << "hdc check screen_server stop";
        QStringList args = {"-t", serial, "shell", "ps -e | grep screen_server"};
        QProcess proc;
        while (true)
        {
            proc.start("hdc", args);
            if (!proc.waitForFinished(2000))
            {
                qWarning() << "hdc check screen_server fail, check again";
                proc.kill();
                continue;
            }
            QString output = QString::fromLocal8Bit(proc.readAllStandardOutput().trimmed());
            qDebug() << "hdc check output:" << output;
            if (!output.contains("screen_server"))
            {
                qInfo() << "hdc screen_server stopped";
                emit serverStopped(serial);
                break;
            }
        }
        proc.kill();
        proc.close();
        qDebug() << "hdc check screen_server stop done";
    });
    return true;
}

bool OHOSDevice::queryDeviceInfo(const QString& serial, DeviceInfo& info)
{
    QMutexLocker locker(&m_mutex); // Lock

    info.type = DeviceType::OHOS;
    info.serial = serial;
    info.name = deviceName(serial);
    QSize res = deviceResolution(serial);
    if (res.isValid())
    {
        info.scale = getScale(res);
        QSize scaledSize = scaleSize(res);
        info.width = scaledSize.width();
        info.height = scaledSize.height();
    }
    else
    {
        info.width = 320;  // Default width
        info.height = 560; // Default height
    }
    return true;
}

DeviceType OHOSDevice::deviceType() const
{
    return DeviceType::OHOS;
}

// Send events
bool OHOSDevice::sendEvent(const DeviceInfo& dev, DeviceEvent eventType)
{
    QStringList args = {};
    switch (eventType)
    {
        case DeviceEvent::HOME: {
            args += {"-t", dev.serial, "shell", "uinput", "-K", "-d", "1", "-u", "1"};
        }
        break;
        case DeviceEvent::BACK: {
            args += {"-t", dev.serial, "shell", "uinput", "-K", "-d", "2", "-u", "2"};
        }
        break;
        case DeviceEvent::MENU: {
            args += {"-t", dev.serial, "shell", "uinput", "-K", "-d", "2078", "-u", "2078"};
        }
        break;
        case DeviceEvent::WAKEUP: {
            if (isScreenOn(dev.serial))
            {
                qInfo() << "hdc screen is on, no need to wakeup";
                return true;
            }
            args += {"-t", dev.serial, "shell", "power-shell", "wakeup"};
        }
        break;
        case DeviceEvent::SLEEP: {
            if (!isScreenOn(dev.serial))
            {
                qInfo() << "hdc screen is off, no need to sleep";
                return true;
            }
            args += {"-t", dev.serial, "shell", "power-shell", "suspend"};
        }
        break;
        case DeviceEvent::ROTATE: {
        }
        break;
        case DeviceEvent::UNLOCK: {
            args += {"-t", dev.serial, "shell", "uinput", "-T", "-m", "500", "1000", "500", "500", "100"};
        }
        break;
        case DeviceEvent::SHUTDOWN: {
            args += {"-t", dev.serial, "shell", "poweroff", "-f"};
        }
        break;
        case DeviceEvent::REBOOT: {
            args += {"-t", dev.serial, "shell", "reboot"};
        }
        break;
        default:
            break;
    }

    if (!args.isEmpty())
    {
        execCommand("hdc", args);
        return true;
    }
    else
    {
        qWarning() << "hdc handle event failed: not implement :" << eventType;
    }
    return false;
}

bool OHOSDevice::sendTouchEvent(const DeviceInfo& dev, QPoint pos)
{
    QPoint scaledPos = restorePoint(pos, dev.scale);
    QStringList args = {"-t",
                        dev.serial,
                        "shell",
                        "uinput",
                        "-T",
                        "-c",
                        QString::number(scaledPos.x()),
                        QString::number(scaledPos.y())};
    return !execCommand("hdc", args).isEmpty();
}

bool OHOSDevice::sendTextEvent(const DeviceInfo& dev, const QString& text)
{
    QStringList args = {"-t", dev.serial, "shell", "uinput", "-K", "-t", text};
    return !execCommand("hdc", args).isEmpty();
}

bool OHOSDevice::sendSwipeEvent(const DeviceInfo& dev, QPoint start, QPoint end, int duration)
{
    QPoint scaledStart = restorePoint(start, dev.scale);
    QPoint scaledEnd = restorePoint(end, dev.scale);
    QStringList args = {"-t",
                        dev.serial,
                        "shell",
                        "uinput",
                        "-T",
                        "-m",
                        QString::number(scaledStart.x()),
                        QString::number(scaledStart.y()),
                        QString::number(scaledEnd.x()),
                        QString::number(scaledEnd.y()),
                        QString::number(duration)};
    return !execCommand("hdc", args).isEmpty();
}

bool OHOSDevice::screenshot(const QString& serial, QByteArray& imageData)
{
    QMutexLocker locker(&m_mutex);
    QString tempPath =
        QString("/data/local/tmp/screenshot_%1.jpeg").arg(QDateTime::currentDateTime().toString("yyyyMMddhhmmss"));

    // Take screenshot
    QStringList args = {"-t", serial, "shell", "snapshot_display", "-f", tempPath};
    if (execCommand("hdc", args).isEmpty())
    {
        qWarning() << "hdc screencap failed";
        return false;
    }

    // Pull screenshot to local
    QString localPath = QDir::tempPath() + QDir::separator() + "screenshot_" +
                        QDateTime::currentDateTime().toString("yyyyMMddhhmmss") + ".jpeg";
    localPath = QDir::toNativeSeparators(localPath);
    QStringList pullArgs = {"-t", serial, "file", "recv", tempPath, localPath};
    if (execCommand("hdc", pullArgs).isEmpty())
    {
        qWarning() << "hdc pull screenshot failed";
        return false;
    }

    // rm data temp screenshot
    QStringList rmArgs = {"-t", serial, "shell", "rm", "-f", tempPath};
    execCommand("hdc", rmArgs);

    // Read image data
    QFile tempFile(localPath);
    if (tempFile.open(QIODevice::ReadOnly))
    {
        imageData = tempFile.readAll();
        tempFile.close();
        QFile::remove(localPath); // Clean up local file
        return true;
    }

    return false;
}

bool OHOSDevice::isScreenOn(const QString& serial)
{
    QStringList args = {"-t", serial, "shell", "hidumper", "-s", "RenderService", "-a", "screen"};
    QString output = execCommand("hdc", args);
    return output.contains("powerStatus=POWER_STATUS_ON");
}

bool OHOSDevice::supportEvent(DeviceEvent eventType) const
{
    return eventType != DeviceEvent::ROTATE && eventType != DeviceEvent::MENU;
}
