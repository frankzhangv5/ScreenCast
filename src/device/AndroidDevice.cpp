#include "device/AndroidDevice.h"

#include <QDebug>
#include <QFile>
#include <QMutex>
#include <QProcess>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QTemporaryFile>
#include <QtConcurrent/QtConcurrent>

// Android device proxy implementation
AndroidDevice::AndroidDevice(QObject* parent) : DeviceProxy(parent) {}

QVector<DeviceInfo> AndroidDevice::queryDevices()
{
    QVector<DeviceInfo> devices;
    QString adbOutput = execCommand("adb", {"devices"});
    QStringList adbLines = adbOutput.split('\n', Qt::SkipEmptyParts);
    for (const QString& line : adbLines)
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
    QStringList args = {"-s", serial, "shell", "getprop", "ro.product.model"};
    return execCommand("adb", args);
}

QString AndroidDevice::deviceName(const QString& serial)
{
    QStringList args = {"-s", serial, "shell", "getprop", "ro.product.model"};
    return execCommand("adb", args);
}

QSize AndroidDevice::deviceResolution(const QString& serial)
{
    QStringList args = {"-s", serial, "shell", "wm", "size"};
    QString output = execCommand("adb", args);
    if (output.isEmpty())
        return QSize();

    // Parse output format "Physical size: 1080x2340"
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

bool AndroidDevice::setupDeviceServer(const QString& serial, int forwardPort)
{
    QMutexLocker locker(&m_mutex); // Lock

    if (!pushResourceToDevice(serial, ":/server/android", "/data/local/tmp/screen_server"))
    {
        qWarning() << "push screen_server failed";
        return false;
    }

    // adb forward port
    QStringList forwardArgs = {"-s", serial, "forward", QString("tcp:%1").arg(forwardPort), "tcp:12345"};
    QProcess proc;
    proc.start("adb", forwardArgs);
    if (!proc.waitForFinished(30000) || proc.exitCode() != 0)
    {
        qWarning() << "adb forward failed for android";
        return false;
    }

    return true;
}

bool AndroidDevice::startDeviceServer(const QString& serial)
{
    QMutexLocker locker(&m_mutex); // Lock
    // Stop screen_server process via pkill
    QStringList args = {"-s", serial, "shell", "pkill", "-9", "screen_server"};
    QProcess proc;
    proc.start("adb", args);
    if (!proc.waitForFinished(30000) || proc.exitCode() != 0)
    {
        qWarning() << "adb stop screen_server failed:" << proc.readAllStandardError();
    }

    // Stop screenrecord process via pkill
    QStringList killArgs = {"-s", serial, "shell", "pkill", "-9", "screenrecord"};

    proc.start("adb", killArgs);
    if (!proc.waitForFinished(30000) || proc.exitCode() != 0)
    {
        qWarning() << "adb stop screenrecord failed:" << proc.readAllStandardError();
    }

    m_running = true;
    QtConcurrent::run([=]() {
        qDebug() << "adb start screen_server process";
        QString remotePath = "/data/local/tmp/screen_server";
        QStringList args = {"-s", serial, "shell", remotePath};
        QProcess proc;
        proc.start("adb", args);
        while (m_running)
        {
            proc.waitForFinished(1000);
        }
        proc.kill();
        proc.close();
        qWarning() << "adb screen_server exit";
    });

    QtConcurrent::run([=]() {
        qDebug() << "adb check screen_server start";
        QStringList args = {"-s", serial, "shell", "netstat -ltn | grep ':12345'"};
        QProcess proc;
        while (m_running)
        {
            proc.start("adb", args);
            if (!proc.waitForFinished(2000))
            {
                qWarning() << "adb check screen_server fail, check again";
                proc.kill();
                continue;
            }
            QString output = QString::fromLocal8Bit(proc.readAllStandardOutput().trimmed());
            qDebug() << "adb netstat output:" << output;
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
        qDebug() << "adb check screen_server start done";
    });
    return true;
}

bool AndroidDevice::stopDeviceServer(const QString& serial)
{
    QMutexLocker locker(&m_mutex); // Lock
    m_running = false;

    QtConcurrent::run([=]() {
        qDebug() << "adb check screen_server stop";
        QStringList args = {"-s", serial, "shell", "ps -e | grep screen_server"};
        QProcess proc;
        while (true)
        {
            proc.start("adb", args);
            if (!proc.waitForFinished(2000))
            {
                qWarning() << "adb check screen_server fail, check again";
                proc.kill();
                continue;
            }
            QString output = QString::fromLocal8Bit(proc.readAllStandardOutput().trimmed());
            qDebug() << "adb check output:" << output;
            if (!output.contains("screen_server"))
            {
                qInfo() << "adb screen_server stopped";
                emit serverStopped(serial);
                break;
            }
        }
        proc.kill();
        proc.close();
        qDebug() << "adb check screen_server stop done";
    });

    return true;
}

bool AndroidDevice::queryDeviceInfo(const QString& serial, DeviceInfo& info)
{
    QMutexLocker locker(&m_mutex); // Lock

    info.type = DeviceType::Android;
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

DeviceType AndroidDevice::deviceType() const
{
    return DeviceType::Android;
}

// Send events
bool AndroidDevice::sendEvent(const DeviceInfo& dev, DeviceEvent eventType)
{
    QString eventStr;
    switch (eventType)
    {
        case DeviceEvent::BACK:
            eventStr = "KEYCODE_BACK";
            break;
        case DeviceEvent::HOME:
            eventStr = "KEYCODE_HOME";
            break;
        case DeviceEvent::MENU:
            eventStr = "KEYCODE_APP_SWITCH";
            break;
        case DeviceEvent::WAKEUP: {
            if (isScreenOn(dev.serial))
            {
                qWarning() << "screen is on, no need wakup event";
                return true;
            }
            eventStr = "KEYCODE_POWER";
        }
        break;
        case DeviceEvent::SLEEP: {
            if (!isScreenOn(dev.serial))
            {
                qWarning() << "screen is off, no need sleep event";
                return true;
            }
            eventStr = "KEYCODE_POWER";
        }
        break;
        default:
            break;
    }
    if (!eventStr.isEmpty())
    {
        qWarning() << "key event type: " << eventStr;
        QStringList args = {"-s", dev.serial, "shell", "input", "keyevent", eventStr};
        QProcess proc;
        proc.start("adb", args);
        if (!proc.waitForFinished(30000) || proc.exitCode() != 0)
        {
            qWarning() << "adb send event failed:" << proc.readAllStandardError();
            return false;
        }
        return true;
    }

    switch (eventType)
    {
        case DeviceEvent::ROTATE: {
            static int rotation = 0;
            QStringList args = {"-s", dev.serial, "shell", "settings", "put", "system", "acscreen_orientation", "1"};
            QProcess proc;
            proc.start("adb", args);
            if (!proc.waitForFinished(30000) || proc.exitCode() != 0)
            {
                qWarning() << "adb ROTATE event failed:" << proc.readAllStandardError();
                return false;
            }
            QStringList rotateArgs = {
                "-s", dev.serial, "shell", "settings", "put", "system", "user_rotation", QString::number(rotation)};
            proc.start("adb", rotateArgs);
            if (!proc.waitForFinished(30000) || proc.exitCode() != 0)
            {
                qWarning() << "adb ROTATE event failed:" << proc.readAllStandardError();
                return false;
            }
            rotation = (rotation + 1) % 4;
            return true;
        }
        break;
        case DeviceEvent::UNLOCK: {
            QStringList args = {"-s", dev.serial, "shell", "input", "swipe", "500", "1800", "500", "800", "300"};
            QProcess proc;
            proc.start("adb", args);
            if (!proc.waitForFinished(30000) || proc.exitCode() != 0)
            {
                qWarning() << "adb UNLOCK event failed:" << proc.readAllStandardError();
                return false;
            }
            return true;
        }
        break;
        case DeviceEvent::SHUTDOWN: {
            QStringList args = {"-s", dev.serial, "shell", "reboot", "-p"};
            QProcess proc;
            proc.start("adb", args);
            if (!proc.waitForFinished(30000) || proc.exitCode() != 0)
            {
                qWarning() << "adb POWER_OFF event failed:" << proc.readAllStandardError();
                return false;
            }
            return true;
        }
        case DeviceEvent::REBOOT: {
            QStringList args = {"-s", dev.serial, "shell", "reboot"};
            QProcess proc;
            proc.start("adb", args);
            if (!proc.waitForFinished(30000) || proc.exitCode() != 0)
            {
                qWarning() << "adb REBOOT event failed:" << proc.readAllStandardError();
                return false;
            }
            return true;
        }
        break;
        default:
            break;
    }

    return true;
}

bool AndroidDevice::sendTouchEvent(const DeviceInfo& dev, QPoint pos)
{
    QPoint scaledPos = restorePoint(pos, dev.scale);
    QStringList args = {
        "-s", dev.serial, "shell", "input", "tap", QString::number(scaledPos.x()), QString::number(scaledPos.y())};
    return !execCommand("adb", args).isEmpty();
}

bool AndroidDevice::sendTextEvent(const DeviceInfo& dev, const QString& text)
{
    QStringList args = {"-s", dev.serial, "shell", "input", "text", text};
    return !execCommand("adb", args).isEmpty();
}

bool AndroidDevice::sendSwipeEvent(const DeviceInfo& dev, QPoint start, QPoint end, int duration)
{
    QPoint scaledStart = restorePoint(start, dev.scale);
    QPoint scaledEnd = restorePoint(end, dev.scale);
    QStringList args = {"-s",
                        dev.serial,
                        "shell",
                        "input",
                        "swipe",
                        QString::number(scaledStart.x()),
                        QString::number(scaledStart.y()),
                        QString::number(scaledEnd.x()),
                        QString::number(scaledEnd.y()),
                        QString::number(duration)};
    return !execCommand("adb", args).isEmpty();
}

bool AndroidDevice::screenshot(const QString& serial, QByteArray& imageData)
{
    QMutexLocker locker(&m_mutex);
    QStringList args = {"-s", serial, "exec-out", "screencap", "-p"};
    QProcess proc;
    proc.start("adb", args);
    if (!proc.waitForFinished(30000) || proc.exitCode() != 0)
    {
        qWarning() << "adb screenshot failed:" << proc.readAllStandardError();
        return false;
    }
    imageData = proc.readAllStandardOutput();
    return true;
}

bool AndroidDevice::pushResourceToDevice(const QString& serial, const QString& resId, const QString& dstPath)
{
    QFile resFile(resId);
    if (!resFile.open(QIODevice::ReadOnly))
    {
        qWarning() << "failed to open resource file:" << resId;
        return false;
    }

    QTemporaryFile tmpFile;
    tmpFile.setAutoRemove(false);
    if (!tmpFile.open())
    {
        qWarning() << "failed to create temp file";
        return false;
    }

    tmpFile.write(resFile.readAll());
    tmpFile.flush();
    tmpFile.close();
    resFile.close();

    QString localPath = tmpFile.fileName();

    // adb push
    QStringList pushArgs = {"-s", serial, "push", QDir::toNativeSeparators(localPath), dstPath};
    QProcess proc;
    proc.start("adb", pushArgs);
    if (!proc.waitForFinished(30000) || proc.exitCode() != 0)
    {
        qWarning() << "adb push failed:" << proc.readAllStandardError();
        tmpFile.remove();
        return false;
    }

    // adb shell chmod 755
    QStringList chmodArgs = {"-s", serial, "shell", "chmod", "755", dstPath};
    proc.start("adb", chmodArgs);
    if (!proc.waitForFinished(30000) || proc.exitCode() != 0)
    {
        qWarning() << "adb chmod failed:" << proc.readAllStandardError();
        tmpFile.remove();
        return false;
    }

    tmpFile.remove();
    qDebug() << "adb push resource to device success";
    return true;
}

bool AndroidDevice::isScreenOn(const QString& serial)
{
    QStringList args = {"-s", serial, "shell", "dumpsys", "display"};
    QString output = execCommand("adb", args);
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

bool AndroidDevice::supportEvent(DeviceEvent eventType) const
{
    return eventType != DeviceEvent::ROTATE;
}
