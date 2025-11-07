#pragma once

#include <QDateTime>
#include <QDebug>
#include <QFileInfo>
#include <QMutex>
#include <QVector>
#include <functional>

// Custom log macros that support recording file name and line number even in release mode
#define LOG_DEBUG(msg) Logger::instance().logDebug(QFileInfo(__FILE__).fileName().toStdString().c_str(), __LINE__, msg)
#define LOG_INFO(msg)  Logger::instance().logInfo(QFileInfo(__FILE__).fileName().toStdString().c_str(), __LINE__, msg)
#define LOG_WARNING(msg)                                                                                               \
    Logger::instance().logWarning(QFileInfo(__FILE__).fileName().toStdString().c_str(), __LINE__, msg)
#define LOG_ERROR(msg) Logger::instance().logError(QFileInfo(__FILE__).fileName().toStdString().c_str(), __LINE__, msg)
#define LOG_CRITICAL(msg)                                                                                              \
    Logger::instance().logCritical(QFileInfo(__FILE__).fileName().toStdString().c_str(), __LINE__, msg)

class Logger
{
public:
    using LogHandlerFunc = std::function<void(QtMsgType, const QMessageLogContext&, const QString&)>;

    static Logger& instance()
    {
        static Logger inst;
        return inst;
    }

    static void install();

    // 设置文件日志记录
    static void setupFileLogger(bool enabled, const QString& logDir);
    static void removeFileLogger();

    // 设置日志文件名
    static void setLogFileName(const QString& fileName);
    static QString getLogFileName();

private:
    Logger();
    Q_DISABLE_COPY(Logger)

    void addDefaultConsoleHandler();
    void addHandler(LogHandlerFunc handler);
    void writeLog(QtMsgType type, const QMessageLogContext& context, const QString& msg);
    static void messageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg);

    // 内部日志记录函数，支持自定义文件名和行号
    void log(QtMsgType type, const char* file, int line, const QString& msg);

public:
    // 公共日志接口，支持在release模式下记录文件名和行号
    void logDebug(const char* file, int line, const QString& msg);
    void logInfo(const char* file, int line, const QString& msg);
    void logWarning(const char* file, int line, const QString& msg);
    void logError(const char* file, int line, const QString& msg);
    void logCritical(const char* file, int line, const QString& msg);

    // 保存当前的文件日志处理器
    static LogHandlerFunc s_fileHandler;

    QMutex m_mutex;
    QVector<LogHandlerFunc> m_handlers;
};