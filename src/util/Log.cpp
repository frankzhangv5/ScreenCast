#include "util/Log.h"

#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QMutex>
#include <QTextStream>

void LogHandler::install()
{
    qInstallMessageHandler(LogHandler::messageHandler);
}

void LogHandler::messageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
    LogHandler::instance().writeLog(type, context, msg);
}

void LogHandler::writeLog(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
    (void) context;
    QString level;
    switch (type)
    {
        case QtDebugMsg:
            level = "DEBUG";
            break;
        case QtInfoMsg:
            level = "INFO";
            break;
        case QtWarningMsg:
            level = "WARN";
            break;
        case QtCriticalMsg:
            level = "CRIT";
            break;
        case QtFatalMsg:
            level = "FATAL";
            break;
    }

    QString logLine =
        QString("%1 [%2] %3").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz")).arg(level).arg(msg);

    // Console output
    fprintf(stderr, "%s\n", logLine.toUtf8().constData());
    fflush(stderr);

    // Write to file
    if (Settings::instance().logToFile())
    {
        QMutexLocker locker(&m_mutex);
        QString logDir = Settings::instance().logDir();
        QDir().mkpath(logDir);
        QString logFilePath = logDir + "/app.log";
        QFile file(logFilePath);
        if (file.open(QIODevice::Append | QIODevice::Text))
        {
            QTextStream ts(&file);
            ts << logLine << "\n";
            file.close();
        }
    }

    if (type == QtFatalMsg)
        abort();
}
