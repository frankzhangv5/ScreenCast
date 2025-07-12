#include "device/DeviceProxy.h"

#include <QDebug>
#include <QObject>
#include <QPoint>
#include <QProcess>
#include <QSize>
#include <QString>
#include <QVector>

DeviceProxy::DeviceProxy(QObject* parent) : QObject(parent) {}

QString DeviceProxy::execCommand(const QString& program, const QStringList& arguments, int timeout)
{
    // qDebug() << "execCommand:" << program << arguments;

    QProcess process;
    process.start(program, arguments);
    if (!process.waitForFinished(timeout) || process.exitCode() != 0)
    {
        qWarning() << program << "failed:" << process.readAllStandardError();
        process.kill();
        process.waitForFinished(5000);
        return QString();
    }
    return QString::fromLocal8Bit(process.readAllStandardOutput().trimmed());
}

qreal DeviceProxy::getScale(QSize resolution, int max_size) const
{
    return qMin(static_cast<qreal>(max_size) / qMax(resolution.width(), resolution.height()), 1.0);
}

QPoint DeviceProxy::scalePoint(QPoint pos) const
{
    const qreal scale = getScale(QSize(pos.x(), pos.y()));
    return QPoint(qRound(pos.x() * scale), qRound(pos.y() * scale));
}

QSize DeviceProxy::scaleSize(QSize resolution) const
{
    const qreal scale = getScale(resolution);
    return QSize(qRound(resolution.width() * scale), qRound(resolution.height() * scale));
}

QSize DeviceProxy::restoreSize(QSize scaledSize, qreal scale) const
{
    return scale > 0 ? QSize(qRound(scaledSize.width() / scale), qRound(scaledSize.height() / scale)) : scaledSize;
}

QPoint DeviceProxy::restorePoint(QPoint scaledPoint, qreal scale) const
{
    return scale > 0 ? QPoint(qRound(scaledPoint.x() / scale), qRound(scaledPoint.y() / scale)) : scaledPoint;
}
