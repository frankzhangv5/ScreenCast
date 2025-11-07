#include "settings/Settings.h"

#include "log/Logger.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLocale>
#include <QMutex>
#include <QObject>
#include <QSettings>
#include <QStandardPaths>
#include <QTextStream>

Settings::Settings() : QObject(nullptr), m_settings("settings.ini", QSettings::IniFormat)
{
    // Log
    if (!m_settings.contains(KEY_LOG_TO_FILE))
        m_settings.setValue(KEY_LOG_TO_FILE, true);
    if (!m_settings.contains(KEY_LOG_DIR))
        m_settings.setValue(KEY_LOG_DIR, defaultLogDir());

    // Media
    if (!m_settings.contains(KEY_MEDIA_SCREENSHOT_DIR))
        m_settings.setValue(KEY_MEDIA_SCREENSHOT_DIR, defaultScreenshotDir());
    if (!m_settings.contains(KEY_MEDIA_RECORD_DIR))
        m_settings.setValue(KEY_MEDIA_RECORD_DIR, defaultRecordDir());

    // Language
    if (!m_settings.contains(KEY_LANG))
        m_settings.setValue(KEY_LANG, systemDefaultLang());

    // Initialize log file handler based on current setting
    setupLogFileHandler(logToFile());
}

bool Settings::logToFile() const
{
    return m_settings.value(KEY_LOG_TO_FILE, false).toBool();
}

void Settings::setLogToFile(bool v)
{
    m_settings.setValue(KEY_LOG_TO_FILE, v);
    setupLogFileHandler(v);
}

QString Settings::logDir() const
{
    return m_settings.value(KEY_LOG_DIR, defaultLogDir()).toString();
}

void Settings::setLogDir(const QString& dir)
{
    m_settings.setValue(KEY_LOG_DIR, dir);
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

bool Settings::isPluginEnabled(const QString& pluginName) const
{
    // Check if plugin is enabled in format plugin.enabled.${pluginName}
    // Default value is true, meaning plugins are enabled by default
    return m_settings.value(QString("plugin.enabled.%1").arg(pluginName), true).toBool();
}

void Settings::setPluginEnabled(const QString& pluginName, bool enabled)
{
    // Save plugin enabled state in format plugin.enabled.${pluginName}
    m_settings.setValue(QString("plugin.enabled.%1").arg(pluginName), enabled);
}

QStringList Settings::getEnabledPlugins() const
{
    // Get list of all enabled plugins
    QStringList enabledPlugins;

    // Get all keys
    QStringList allKeys = m_settings.allKeys();

    // Find all keys starting with "plugin.enabled." with value true
    foreach (const QString& key, allKeys)
    {
        if (key.startsWith("plugin.enabled.") && m_settings.value(key).toBool())
        {
            // Extract plugin name (remove "plugin.enabled." prefix)
            QString pluginName = key.mid(QString("plugin.enabled.").length());
            enabledPlugins << pluginName;
        }
    }

    return enabledPlugins;
}

void Settings::setEnabledPlugins(const QStringList& pluginNames)
{
    // Note: This method is kept for compatibility with existing code, but internally converts to individual key-value
    // pairs Get current state of all plugins
    QStringList allKeys = m_settings.allKeys();
    QStringList existingPlugins;

    // Collect all existing plugin.enabled. keys
    foreach (const QString& key, allKeys)
    {
        if (key.startsWith("plugin.enabled."))
        {
            QString pluginName = key.mid(QString("plugin.enabled.").length());
            existingPlugins << pluginName;
        }
    }

    // Set state for all existing plugins
    foreach (const QString& pluginName, existingPlugins)
    {
        setPluginEnabled(pluginName, pluginNames.contains(pluginName));
    }

    // Set plugins in pluginNames but not in existingPlugins as enabled
    foreach (const QString& pluginName, pluginNames)
    {
        if (!existingPlugins.contains(pluginName))
        {
            setPluginEnabled(pluginName, true);
        }
    }
}

bool Settings::autoCheckUpdates() const
{
    return m_settings.value("update/autoCheck", true).toBool();
}

void Settings::setAutoCheckUpdates(bool enabled)
{
    m_settings.setValue("update/autoCheck", enabled);
}

QVariant Settings::value(const QString& key, const QVariant& defaultValue) const
{
    return m_settings.value(key, defaultValue);
}

void Settings::setValue(const QString& key, const QVariant& value)
{
    m_settings.setValue(key, value);
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
    // Use application's current working directory as default log directory to avoid permission issues
    QString logDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    qDebug() << "Default log directory set to:" << logDir;
    return logDir;
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
    m_settings.setValue(KEY_LOG_TO_FILE, true);
    m_settings.setValue(KEY_LOG_DIR, defaultLogDir());

    // Media
    m_settings.setValue(KEY_MEDIA_SCREENSHOT_DIR, defaultScreenshotDir());
    m_settings.setValue(KEY_MEDIA_RECORD_DIR, defaultRecordDir());

    // Language
    m_settings.setValue(KEY_LANG, systemDefaultLang());

    qDebug() << "Settings reset to default values";
}

void Settings::setupLogFileHandler(bool enable)
{
    try
    {
        qDebug() << "Settings::setupLogFileHandler called with enable=" << enable;

        if (enable)
        {
            // Check if log directory exists to ensure logs can be written
            QString logDir = this->logDir();
            QDir dir(logDir);
            if (!dir.exists())
            {
                qDebug() << "Settings::setupLogFileHandler - Creating log directory:" << logDir;
                if (!dir.mkpath("."))
                {
                    qWarning() << "Settings::setupLogFileHandler - Failed to create log directory:" << logDir;
                }
            }

            // If directory exists, set up file logging
            if (dir.exists())
            {
                QFileInfo dirInfo(logDir);
                if (dirInfo.isWritable())
                {
                    qDebug() << "Settings::setupLogFileHandler - Setting up file logger with logDir=" << logDir;
                    // Use Logger class's public method to set up file logging
                    Logger::setupFileLogger(true, logDir);
                }
                else
                {
                    qWarning() << "Settings::setupLogFileHandler - Log directory not writable:" << logDir;
                }
            }
        }
        else
        {
            qDebug() << "Settings::setupLogFileHandler - Disabling file logging";

            // Call removeFileLogger asynchronously to avoid blocking main thread
            QMetaObject::invokeMethod(
                QCoreApplication::instance(),
                []() {
                    try
                    {
                        Logger::instance().removeFileLogger();
                        qDebug() << "Settings::setupLogFileHandler - removeFileLogger completed successfully";
                    }
                    catch (const std::exception& e)
                    {
                        qWarning() << "Settings::setupLogFileHandler - Exception in async removeFileLogger:"
                                   << e.what();
                    }
                    catch (...)
                    {
                        qWarning() << "Settings::setupLogFileHandler - Unknown exception in async removeFileLogger";
                    }
                    qDebug() << "Settings::setupLogFileHandler - File logger disabled (handler reference cleared)";
                },
                Qt::QueuedConnection);
        }
    }
    catch (const std::exception& e)
    {
        // Catch any possible exceptions to prevent program crashes
        qWarning() << "Settings::setupLogFileHandler - Exception:" << e.what();
    }
    catch (...)
    {
        // Catch all non-standard exceptions
        qWarning() << "Settings::setupLogFileHandler - Unknown exception";
    }
}