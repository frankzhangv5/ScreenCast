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

Settings::Settings() : QObject(nullptr), m_settings("settings.ini", QSettings::IniFormat)
{
    // Log
    if (!m_settings.contains(KEY_LOG_TO_FILE))
        m_settings.setValue(KEY_LOG_TO_FILE, false);
    if (!m_settings.contains(KEY_LOG_DIR))
        m_settings.setValue(KEY_LOG_DIR, defaultLogDir());

    // Proxy
    bool adbInstalled = !QStandardPaths::findExecutable("adb").isEmpty();
    bool hdcInstalled = !QStandardPaths::findExecutable("hdc").isEmpty();

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
    bool adbInstalled = !QStandardPaths::findExecutable("adb").isEmpty();
    bool hdcInstalled = !QStandardPaths::findExecutable("hdc").isEmpty();
    m_settings.setValue(KEY_PROXY_ADB, adbInstalled);
    m_settings.setValue(KEY_PROXY_HDC, hdcInstalled);

    // Media
    m_settings.setValue(KEY_MEDIA_SCREENSHOT_DIR, defaultScreenshotDir());
    m_settings.setValue(KEY_MEDIA_RECORD_DIR, defaultRecordDir());

    // Language
    m_settings.setValue(KEY_LANG, systemDefaultLang());

    qDebug() << "Settings reset to default values";
}