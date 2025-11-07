#include "processor/DeviceEventProcessor.h"

#include "manager/DeviceManager.h"

#include <QDebug>

DeviceEventProcessor::DeviceEventProcessor(QObject* parent) : QObject(parent)
{
    m_threadPool = QThreadPool::globalInstance();
    m_threadPool->setMaxThreadCount(4); // 设置最大线程数
}

DeviceEventProcessor::~DeviceEventProcessor()
{
    m_threadPool->waitForDone(); // 等待所有任务完成
}

void DeviceEventProcessor::processKeyEvent(const QString& eventString, const DeviceInfo& deviceInfo)
{
    // 创建任务并在非UI线程中执行
    EventTask* task = new EventTask(eventString, deviceInfo, this);
    task->setAutoDelete(true); // 任务完成后自动删除
    m_threadPool->start(task);
}

void DeviceEventProcessor::processTouchEvent(const QPoint& pos, const DeviceInfo& deviceInfo)
{
    // 在单独的任务中处理触摸事件
    QThreadPool::globalInstance()->start([pos, deviceInfo, this]() {
        qDebug() << "Processing touch event at position:" << pos;

        // 获取设备代理
        DeviceProxy* proxy = DeviceManager::instance().proxyForType(deviceInfo.type);
        if (!proxy)
        {
            qWarning() << "No proxy found for device type:" << static_cast<int>(deviceInfo.type);
            QMetaObject::invokeMethod(
                this, "onEventProcessed", Qt::QueuedConnection, Q_ARG(bool, false), Q_ARG(QString, "TOUCH"));
            return;
        }

        // 检查是否支持触摸事件
        if (!proxy->supportEvent(DeviceEvent::TOUCH))
        {
            qWarning() << "Device does not support touch event";
            QMetaObject::invokeMethod(
                this, "onEventProcessed", Qt::QueuedConnection, Q_ARG(bool, false), Q_ARG(QString, "TOUCH"));
            return;
        }

        // 发送触摸事件
        bool success = proxy->sendTouchEvent(deviceInfo, pos);

        // 通过队列连接通知处理器结果
        QMetaObject::invokeMethod(
            this, "onEventProcessed", Qt::QueuedConnection, Q_ARG(bool, success), Q_ARG(QString, "TOUCH"));

        if (success)
        {
            qDebug() << "Successfully sent touch event at position:" << pos;
        }
        else
        {
            qWarning() << "Failed to send touch event at position:" << pos;
        }
    });
}

void DeviceEventProcessor::processSwipeEvent(const QPoint& startPos, const QPoint& endPos, const DeviceInfo& deviceInfo)
{
    // 在单独的任务中处理滑动事件
    QThreadPool::globalInstance()->start([startPos, endPos, deviceInfo, this]() {
        qDebug() << "Processing swipe event from:" << startPos << "to:" << endPos;

        // 获取设备代理
        DeviceProxy* proxy = DeviceManager::instance().proxyForType(deviceInfo.type);
        if (!proxy)
        {
            qWarning() << "No proxy found for device type:" << static_cast<int>(deviceInfo.type);
            QMetaObject::invokeMethod(
                this, "onEventProcessed", Qt::QueuedConnection, Q_ARG(bool, false), Q_ARG(QString, "SWIPE"));
            return;
        }

        // 检查是否支持滑动事件
        if (!proxy->supportEvent(DeviceEvent::SWIPE))
        {
            qWarning() << "Device does not support swipe event";
            QMetaObject::invokeMethod(
                this, "onEventProcessed", Qt::QueuedConnection, Q_ARG(bool, false), Q_ARG(QString, "SWIPE"));
            return;
        }

        // 发送滑动事件
        bool success = proxy->sendSwipeEvent(deviceInfo, startPos, endPos);

        // 通过队列连接通知处理器结果
        QMetaObject::invokeMethod(
            this, "onEventProcessed", Qt::QueuedConnection, Q_ARG(bool, success), Q_ARG(QString, "SWIPE"));

        if (success)
        {
            qDebug() << "Successfully sent swipe event from:" << startPos << "to:" << endPos;
        }
        else
        {
            qWarning() << "Failed to send swipe event from:" << startPos << "to:" << endPos;
        }
    });
}

void DeviceEventProcessor::onEventProcessed(bool success, const QString& eventString)
{
    // 发出信号通知UI线程处理结果
    emit eventProcessed(success, eventString);
}

// EventTask 实现
DeviceEventProcessor::EventTask::EventTask(const QString& eventString,
                                           const DeviceInfo& deviceInfo,
                                           DeviceEventProcessor* processor)
    : m_eventString(eventString), m_deviceInfo(deviceInfo), m_processor(processor)
{
}

void DeviceEventProcessor::EventTask::run()
{
    qDebug() << "Processing event in non-UI thread:" << m_eventString;

    // 获取对应的DeviceEvent枚举值
    DeviceEvent eventType = stringToEvent(m_eventString);

    if (eventType == DeviceEvent::INVALID)
    {
        qWarning() << "Invalid event type:" << m_eventString;
        QMetaObject::invokeMethod(
            m_processor, "onEventProcessed", Qt::QueuedConnection, Q_ARG(bool, false), Q_ARG(QString, m_eventString));
        return;
    }

    // 获取设备代理
    DeviceProxy* proxy = DeviceManager::instance().proxyForType(m_deviceInfo.type);
    if (!proxy)
    {
        qWarning() << "No proxy found for device type:" << static_cast<int>(m_deviceInfo.type);
        QMetaObject::invokeMethod(
            m_processor, "onEventProcessed", Qt::QueuedConnection, Q_ARG(bool, false), Q_ARG(QString, m_eventString));
        return;
    }

    // 检查是否支持该事件类型
    if (!proxy->supportEvent(eventType))
    {
        qWarning() << "Device does not support event:" << m_eventString;
        QMetaObject::invokeMethod(
            m_processor, "onEventProcessed", Qt::QueuedConnection, Q_ARG(bool, false), Q_ARG(QString, m_eventString));
        return;
    }

    // 发送事件
    bool success = proxy->sendEvent(m_deviceInfo, eventType);

    // 通过队列连接通知处理器结果
    QMetaObject::invokeMethod(
        m_processor, "onEventProcessed", Qt::QueuedConnection, Q_ARG(bool, success), Q_ARG(QString, m_eventString));

    if (success)
    {
        qDebug() << "Successfully sent event:" << m_eventString;
    }
    else
    {
        qWarning() << "Failed to send event:" << m_eventString;
    }
}

DeviceEvent DeviceEventProcessor::EventTask::stringToEvent(const QString& eventString) const
{
    // 将字符串转换为DeviceEvent枚举
    if (eventString.compare("HOME", Qt::CaseInsensitive) == 0)
        return DeviceEvent::HOME;
    else if (eventString.compare("BACK", Qt::CaseInsensitive) == 0)
        return DeviceEvent::BACK;
    else if (eventString.compare("MENU", Qt::CaseInsensitive) == 0)
        return DeviceEvent::MENU;
    else if (eventString.compare("WAKEUP", Qt::CaseInsensitive) == 0)
        return DeviceEvent::WAKEUP;
    else if (eventString.compare("SLEEP", Qt::CaseInsensitive) == 0)
        return DeviceEvent::SLEEP;
    else if (eventString.compare("UNLOCK", Qt::CaseInsensitive) == 0)
        return DeviceEvent::UNLOCK;
    else if (eventString.compare("SHUTDOWN", Qt::CaseInsensitive) == 0)
        return DeviceEvent::SHUTDOWN;
    else if (eventString.compare("REBOOT", Qt::CaseInsensitive) == 0)
        return DeviceEvent::REBOOT;
    else
        return DeviceEvent::INVALID;
}