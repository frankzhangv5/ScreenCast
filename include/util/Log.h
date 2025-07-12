#pragma once
#include "core/Settings.h"

#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QMutex>
#include <QTextStream>

class LogHandler
{
public:
    static LogHandler& instance()
    {
        static LogHandler inst;
        return inst;
    }

    static void install();
    static void messageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg);
    void writeLog(QtMsgType type, const QMessageLogContext& context, const QString& msg);

private:
    LogHandler() = default;
    Q_DISABLE_COPY(LogHandler)
    QMutex m_mutex;
};
