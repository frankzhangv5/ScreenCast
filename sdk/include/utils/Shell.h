#pragma once

#include <QMutex>
#include <QProcess>
#include <QString>

class Shell
{
public:
    // Execute system command
    static bool execute(const QString& command, QString* output = nullptr, QString* error = nullptr);

    // Execute system command (QByteArray output version)
    static bool execute(const QString& command, QByteArray* output, QString* error = nullptr);

    // Execute system command (quiet mode, no debug output)
    static bool executeQuiet(const QString& command, QString* output = nullptr, QString* error = nullptr);

    // Execute system command (separate command and arguments)
    static bool executeCommand(const QString& program,
                               const QStringList& arguments,
                               QString* output = nullptr,
                               QString* error = nullptr);

    // Execute system command (separate command and arguments, QByteArray output version)
    static bool executeCommand(const QString& program,
                               const QStringList& arguments,
                               QByteArray* output = nullptr,
                               QString* error = nullptr);

    // Execute system command (quiet mode, separate command and arguments)
    static bool executeCommandQuiet(const QString& program,
                                    const QStringList& arguments,
                                    QString* output = nullptr,
                                    QString* error = nullptr);
};