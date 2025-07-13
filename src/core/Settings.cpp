#include "core/Settings.h"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLocale>
#include <QMutex>
#include <QObject>
#include <QSettings>
#include <QStandardPaths>

void Settings::updateToolPathsEnv()
{
#ifdef Q_OS_MAC
    QStringList adbPaths = {"/usr/local/bin/adb",
                            "/opt/homebrew/bin/adb",
                            "/opt/local/bin/adb",
                            QDir::homePath() + "/Library/Android/sdk/platform-tools/adb",
                            QDir::homePath() + "/Android/Sdk/platform-tools/adb"};
    QStringList hdcPaths = {"/usr/local/bin/hdc",
                            "/opt/homebrew/bin/hdc",
                            "/opt/local/bin/hdc",
                            QDir::homePath() + "/Library/DevEcoStudio/Sdk/toolchains/hdc"};
    QStringList dirsToAdd;
    for (const QString& path : adbPaths)
    {
        if (QFile::exists(path))
        {
            dirsToAdd << QFileInfo(path).absolutePath();
        }
    }
    for (const QString& path : hdcPaths)
    {
        if (QFile::exists(path))
        {
            dirsToAdd << QFileInfo(path).absolutePath();
        }
    }
    dirsToAdd.removeDuplicates();
    QString currentPath = qgetenv("PATH");
    QStringList pathList = currentPath.split(QLatin1Char(':'), Qt::SkipEmptyParts);
    for (const QString& dir : dirsToAdd)
    {
        if (!pathList.contains(dir))
        {
            pathList.prepend(dir);
        }
    }
    QString newPath = pathList.join(QLatin1Char(':'));
    qputenv("PATH", newPath.toUtf8());
#else
    return;
#endif
}

Settings::Settings() : QObject(nullptr), m_settings("settings.ini", QSettings::IniFormat)
{
    updateToolPathsEnv();

    // Log
    if (!m_settings.contains(KEY_LOG_TO_FILE))
        m_settings.setValue(KEY_LOG_TO_FILE, false);
    if (!m_settings.contains(KEY_LOG_DIR))
        m_settings.setValue(KEY_LOG_DIR, defaultLogDir());

    // Proxy
    bool adbInstalled = isAdbInstalled();
    bool hdcInstalled = isHdcInstalled();

    if (!m_settings.contains(KEY_PROXY_ADB))
        m_settings.setValue(KEY_PROXY_ADB, adbInstalled);
    if (!m_settings.contains(KEY_PROXY_HDC))
        m_settings.setValue(KEY_PROXY_HDC, hdcInstalled);

    // Media
    if (!m_settings.contains(KEY_MEDIA_SCREENSHOT_DIR))
        m_settings.setValue(KEY_MEDIA_SCREENSHOT_DIR, defaultScreenshotDir());
    if (!m_settings.contains(KEY_MEDIA_RECORD_DIR))
        m_settings.setValue(KEY_MEDIA_RECORD_DIR, defaultRecordDir());

    // Language
    if (!m_settings.contains(KEY_LANG))
        m_settings.setValue(KEY_LANG, systemDefaultLang());
}

bool Settings::logToFile() const
{
    return m_settings.value(KEY_LOG_TO_FILE, false).toBool();
}

void Settings::setLogToFile(bool v)
{
    m_settings.setValue(KEY_LOG_TO_FILE, v);
}

QString Settings::logDir() const
{
    return m_settings.value(KEY_LOG_DIR, defaultLogDir()).toString();
}

void Settings::setLogDir(const QString& dir)
{
    m_settings.setValue(KEY_LOG_DIR, dir);
}

bool Settings::adbProxy() const
{
    return m_settings.value(KEY_PROXY_ADB, false).toBool();
}

void Settings::setAdbProxy(bool v)
{
    m_settings.setValue(KEY_PROXY_ADB, v);
}

bool Settings::hdcProxy() const
{
    return m_settings.value(KEY_PROXY_HDC, false).toBool();
}

void Settings::setHdcProxy(bool v)
{
    m_settings.setValue(KEY_PROXY_HDC, v);
}

QString Settings::screenshotDir() const
{
    return m_settings.value(KEY_MEDIA_SCREENSHOT_DIR, defaultScreenshotDir()).toString();
}

void Settings::setScreenshotDir(const QString& dir)
{
    m_settings.setValue(KEY_MEDIA_SCREENSHOT_DIR, dir);
}

QString Settings::recordDir() const
{
    return m_settings.value(KEY_MEDIA_RECORD_DIR, defaultRecordDir()).toString();
}

void Settings::setRecordDir(const QString& dir)
{
    m_settings.setValue(KEY_MEDIA_RECORD_DIR, dir);
}

QString Settings::language() const
{
    return m_settings.value(KEY_LANG, systemDefaultLang()).toString();
}

void Settings::setLanguage(const QString& lang)
{
    m_settings.setValue(KEY_LANG, lang);
}

bool Settings::isProxyEnabled(DeviceType type) const
{
    switch (type)
    {
        case DeviceType::Android:
            return adbProxy();
        case DeviceType::OHOS:
            return hdcProxy();
        default:
            return false;
    }
}

QString Settings::settingsFilePath()
{
    return QFileInfo("settings.ini").absoluteFilePath();
}

QString Settings::systemDefaultLang()
{
    return QLocale::system().name();
}

QString Settings::defaultLogDir()
{
    return QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
}

QString Settings::defaultScreenshotDir()
{
    return QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
}

QString Settings::defaultRecordDir()
{
    return QStandardPaths::writableLocation(QStandardPaths::MoviesLocation);
}

void Settings::resetToDefault()
{
    // Clear all settings
    m_settings.clear();

    // Reinitialize with default values
    // Log
    m_settings.setValue(KEY_LOG_TO_FILE, false);
    m_settings.setValue(KEY_LOG_DIR, defaultLogDir());

    // Proxy
    bool adbInstalled = isAdbInstalled();
    bool hdcInstalled = isHdcInstalled();
    m_settings.setValue(KEY_PROXY_ADB, adbInstalled);
    m_settings.setValue(KEY_PROXY_HDC, hdcInstalled);

    // Media
    m_settings.setValue(KEY_MEDIA_SCREENSHOT_DIR, defaultScreenshotDir());
    m_settings.setValue(KEY_MEDIA_RECORD_DIR, defaultRecordDir());

    // Language
    m_settings.setValue(KEY_LANG, systemDefaultLang());

    qDebug() << "Settings reset to default values";
}

bool Settings::isAdbInstalled()
{
    // First try standard PATH
    QString adbPath = QStandardPaths::findExecutable("adb");
    if (!adbPath.isEmpty())
    {
        return true;
    }

    return false;
}

bool Settings::isHdcInstalled()
{
    // First try standard PATH
    QString hdcPath = QStandardPaths::findExecutable("hdc");
    if (!hdcPath.isEmpty())
    {
        return true;
    }

    return false;
}