#include "utils/Shell.h"

#include <QDebug>
#include <QPoint>
#include <QProcess>

// 私有函数：执行进程并处理结果（通用模板）
template<typename OutputType>
static bool executeProcessInternal(const QString& program,
                                   const QStringList& arguments,
                                   OutputType* output,
                                   QString* error,
                                   bool verbose = true)
{
    if (verbose)
        qDebug() << "executeCommand: " << program << arguments.join(" ");

    QProcess process;
    process.start(program, arguments);

    if (!process.waitForStarted(3000))
    {
        if (error)
            *error = "Failed to start process: " + process.errorString();
        return false;
    }

    if (!process.waitForFinished(10000))
    {
        if (error)
            *error = "Process timeout: " + process.errorString();
        return false;
    }

    if (output)
    {
        if constexpr (std::is_same_v<OutputType, QByteArray>)
            *output = process.readAllStandardOutput();
        else if constexpr (std::is_same_v<OutputType, QString>)
        {
            *output = QString::fromUtf8(process.readAllStandardOutput());
            if (verbose)
            {
                qDebug() << "executeCommand:" << program << arguments.join(" ") << "exitCode:" << process.exitCode()
                         << "output:" << *output;
            }
        }
    }

    if (error)
    {
        *error = QString::fromUtf8(process.readAllStandardError());
        if (verbose)
            qDebug() << "executeCommand:" << program << arguments.join(" ") << "exitCode:" << process.exitCode()
                     << "error:" << *error;
    }

    return process.exitCode() == 0;
}

bool Shell::executeCommand(const QString& program, const QStringList& arguments, QString* output, QString* error)
{
    return executeProcessInternal(program, arguments, output, error, true);
}

bool Shell::executeCommandQuiet(const QString& program, const QStringList& arguments, QString* output, QString* error)
{
    return executeProcessInternal(program, arguments, output, error, false);
}

bool Shell::executeCommand(const QString& program, const QStringList& arguments, QByteArray* output, QString* error)
{
    return executeProcessInternal(program, arguments, output, error, false);
}

// 私有函数：拆分命令字符串为程序和参数
static bool splitCommandString(const QString& command, QString& program, QStringList& arguments)
{
    QStringList parts;
    QString current;
    bool inQuote = false;

    for (int i = 0; i < command.length(); ++i)
    {
        QChar c = command[i];
        if (c == '"')
        {
            inQuote = !inQuote;
        }
        else if (c == ' ' && !inQuote)
        {
            if (!current.isEmpty())
            {
                parts.append(current);
                current.clear();
            }
        }
        else
        {
            current.append(c);
        }
    }

    if (!current.isEmpty())
    {
        parts.append(current);
    }

    if (parts.isEmpty())
        return false;

    program = parts.first();
    arguments = parts.mid(1);
    return true;
}

bool Shell::execute(const QString& command, QString* output, QString* error)
{
    QString program;
    QStringList arguments;

    if (!splitCommandString(command, program, arguments))
        return false;

    return executeCommand(program, arguments, output, error);
}

bool Shell::execute(const QString& command, QByteArray* output, QString* error)
{
    QString program;
    QStringList arguments;

    if (!splitCommandString(command, program, arguments))
        return false;

    return executeCommand(program, arguments, output, error);
}

bool Shell::executeQuiet(const QString& command, QString* output, QString* error)
{
    QString program;
    QStringList arguments;

    if (!splitCommandString(command, program, arguments))
        return false;

    return executeCommandQuiet(program, arguments, output, error);
}
