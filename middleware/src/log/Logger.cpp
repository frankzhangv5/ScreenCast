#include "log/Logger.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMutex>
#include <QStandardPaths>
#include <QTextStream>
#include <QThread>
#include <QVector>
#include <algorithm>
#include <functional>

namespace
{
    // Default log file name
    QString g_logFileName = "log";
} // namespace

// 初始化静态成员变量
Logger::LogHandlerFunc Logger::s_fileHandler = nullptr;

void Logger::install()
{
    qInstallMessageHandler(Logger::messageHandler);
}

void Logger::messageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
    Logger::instance().writeLog(type, context, msg);
}

Logger::Logger()
{
    // Add default console handler
    addDefaultConsoleHandler();
}

void Logger::addDefaultConsoleHandler()
{
    addHandler([](QtMsgType type, const QMessageLogContext& context, const QString& msg) {
        QString level;
        switch (type)
        {
            case QtDebugMsg:
                level = "D";
                break;
            case QtInfoMsg:
                level = "I";
                break;
            case QtWarningMsg:
                level = "W";
                break;
            case QtCriticalMsg:
                level = "CRIT";
                break;
            case QtFatalMsg:
                level = "FATAL";
                break;
        }

        // Get file name (handle potentially null case)
        QString fileName = context.file ? QFileInfo(QString::fromUtf8(context.file)).fileName() : "unknown";

        QString logLine = QString("%1 %2 %3 %4 %5:%6 > %7")
                              .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz"))
                              .arg(level)
                              .arg(QCoreApplication::applicationPid())
                              .arg(reinterpret_cast<quintptr>(QThread::currentThreadId()))
                              .arg(fileName.rightJustified(20, ' '))
                              .arg(QString::number(context.line).rightJustified(3, ' '))
                              .arg(msg);

        // Console output
        fprintf(stderr, "%s\n", logLine.toUtf8().constData());
        fflush(stderr);

        if (type == QtFatalMsg)
            abort();
    });
}

void Logger::addHandler(LogHandlerFunc handler)
{
    m_handlers.append(handler);
}

// 内部日志记录函数，支持自定义文件名和行号
void Logger::log(QtMsgType type, const char* file, int line, const QString& msg)
{
    // 创建一个自定义的消息上下文
    QMessageLogContext context;
    context.file = file;
    context.line = line;

    // 调用writeLog来处理日志
    writeLog(type, context, msg);
}

// 公共日志接口实现
void Logger::logDebug(const char* file, int line, const QString& msg)
{
    log(QtDebugMsg, file, line, msg);
}

void Logger::logInfo(const char* file, int line, const QString& msg)
{
    log(QtInfoMsg, file, line, msg);
}

void Logger::logWarning(const char* file, int line, const QString& msg)
{
    log(QtWarningMsg, file, line, msg);
}

void Logger::logError(const char* file, int line, const QString& msg)
{
    log(QtCriticalMsg, file, line, msg);
}

void Logger::logCritical(const char* file, int line, const QString& msg)
{
    log(QtFatalMsg, file, line, msg);
}

void Logger::writeLog(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
    QMutexLocker locker(&m_mutex);
    // 调用所有已注册的处理器
    for (const auto& handler : m_handlers)
    {
        handler(type, context, msg);
    }
}

void Logger::setupFileLogger(bool enabled, const QString& logDir)
{
    // Default to using Documents directory as log directory
    QString logDirectory = logDir;
    if (logDirectory.isEmpty())
    {
        logDirectory = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    }

    // First remove existing file log handler
    removeFileLogger();

    if (enabled)
    {
        // Create file handler
        s_fileHandler = [logDirectory](QtMsgType type, const QMessageLogContext& context, const QString& msg) {
            QString level;
            switch (type)
            {
                case QtDebugMsg:
                    level = "D";
                    break;
                case QtInfoMsg:
                    level = "I";
                    break;
                case QtWarningMsg:
                    level = "W";
                    break;
                case QtCriticalMsg:
                    level = "CRIT";
                    break;
                case QtFatalMsg:
                    level = "FATAL";
                    break;
            }

            // Get file name (handle potentially null case)
            QString fileName = context.file ? QFileInfo(QString::fromUtf8(context.file)).fileName() : "unknown";

            // Use standard log format with source file name and line number
            QString logLine = QString("%1 %2 %3 %4 %5:%6 > %7")
                                  .arg(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz"))
                                  .arg(level)
                                  .arg(QCoreApplication::applicationPid())
                                  .arg(reinterpret_cast<quintptr>(QThread::currentThreadId()))
                                  .arg(fileName.rightJustified(20, ' '))
                                  .arg(QString::number(context.line).rightJustified(3, ' '))
                                  .arg(msg);

            // Ensure log directory exists
            QDir dir(logDirectory);
            if (!dir.exists())
            {
                dir.mkpath(".");
            }

            // Use configured log file name
            QString logFilePath = dir.absoluteFilePath(g_logFileName + ".txt");

            QFile logFile(logFilePath);
            if (logFile.open(QIODevice::Append | QIODevice::Text))
            {
                QTextStream stream(&logFile);
                stream << logLine << "\n";
                stream.flush();
                logFile.close();
            }
            else
            {
                // File open failed, failing silently to avoid cascading errors
            }
        };

        // Add file log handler
        QMutexLocker locker(&instance().m_mutex);
        instance().addHandler(s_fileHandler);
    }
}

void Logger::removeFileLogger()
{
    if (s_fileHandler)
    {
        // Remove all handlers, then re-add default console handler
        QMutexLocker locker(&instance().m_mutex);
        instance().m_handlers.clear();
        instance().addDefaultConsoleHandler();

        // Clear file handler reference
        s_fileHandler = nullptr;
    }
}

void Logger::setLogFileName(const QString& fileName)
{
    if (!fileName.isEmpty())
    {
        g_logFileName = fileName;
        qDebug() << "Logger: Log file name set to:" << g_logFileName;
    }
    else
    {
        qWarning() << "Logger: Invalid log file name provided, keeping current:" << g_logFileName;
    }
}

QString Logger::getLogFileName()
{
    return g_logFileName;
}