#pragma once
#include "device/DeviceInfo.h"

#include <QDebug>
#include <QFileInfo>
#include <QLocale>
#include <QMutex>
#include <QObject>
#include <QSettings>
#include <QStandardPaths>
#include <QVariant>

class Settings : public QObject
{
    Q_OBJECT
public:
    // Key string constants
    static constexpr const char* KEY_LOG_TO_FILE = "log/toFile";
    static constexpr const char* KEY_LOG_DIR = "log/dir";
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

    // Media
    QString screenshotDir() const;
    void setScreenshotDir(const QString& dir);
    QString recordDir() const;
    void setRecordDir(const QString& dir);

    // Language
    QString language() const;
    void setLanguage(const QString& lang);

    // Plugin enable state
    bool isPluginEnabled(const QString& pluginName) const;
    void setPluginEnabled(const QString& pluginName, bool enabled);
    QStringList getEnabledPlugins() const;
    void setEnabledPlugins(const QStringList& pluginNames);

    // Update settings
    bool autoCheckUpdates() const;
    void setAutoCheckUpdates(bool enabled);

    // Generic get/set
    QVariant value(const QString& key, const QVariant& defaultValue = QVariant()) const;
    void setValue(const QString& key, const QVariant& value);

    // Reset to default settings
    void resetToDefault();

    // Get absolute path of settings.ini
    static QString settingsFilePath();

private:
    Settings();
    static QString systemDefaultLang();
    static QString defaultLogDir();
    static QString defaultScreenshotDir();
    static QString defaultRecordDir();

    // Log file handler
    void setupLogFileHandler(bool enable);

    QSettings m_settings;
};
