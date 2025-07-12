#pragma once
#include "device/DeviceInfo.h"

#include <QDebug>
#include <QFileInfo>
#include <QLocale>
#include <QMutex>
#include <QObject>
#include <QSettings>
#include <QStandardPaths>

class Settings : public QObject
{
    Q_OBJECT
public:
    // Key string constants
    static constexpr const char* KEY_LOG_TO_FILE = "log/toFile";
    static constexpr const char* KEY_LOG_DIR = "log/dir";
    static constexpr const char* KEY_PROXY_ADB = "proxy/adb";
    static constexpr const char* KEY_PROXY_HDC = "proxy/hdc";
    static constexpr const char* KEY_MEDIA_SCREENSHOT_DIR = "media/screenshotDir";
    static constexpr const char* KEY_MEDIA_RECORD_DIR = "media/recordDir";
    static constexpr const char* KEY_LANG = "lang";

    static Settings& instance()
    {
        static Settings _instance;
        return _instance;
    }

    // Log
    bool logToFile() const;
    void setLogToFile(bool v);
    QString logDir() const;
    void setLogDir(const QString& dir);

    // Proxy
    bool adbProxy() const;
    void setAdbProxy(bool v);
    bool hdcProxy() const;
    void setHdcProxy(bool v);

    // Media
    QString screenshotDir() const;
    void setScreenshotDir(const QString& dir);
    QString recordDir() const;
    void setRecordDir(const QString& dir);

    // Language
    QString language() const;
    void setLanguage(const QString& lang);

    bool isProxyEnabled(DeviceType type) const;

    // Reset to default settings
    void resetToDefault();

    // Get absolute path of settings.ini
    static QString settingsFilePath();

    // Check if tools are installed
    static bool isAdbInstalled();
    static bool isHdcInstalled();

private:
    Settings();
    static QString systemDefaultLang();
    static QString defaultLogDir();
    static QString defaultScreenshotDir();
    static QString defaultRecordDir();

    QSettings m_settings;
};
