#pragma once

#include "device/DeviceInfo.h"
#include "device/DeviceProxy.h"

#include <QObject>
#include <QRunnable>
#include <QThreadPool>

/**
 * @brief 设备事件处理器
 *
 * 在非UI线程中处理设备事件，调用deviceProxy接口执行相应动作
 */
class DeviceEventProcessor : public QObject
{
    Q_OBJECT

public:
    explicit DeviceEventProcessor(QObject* parent = nullptr);
    ~DeviceEventProcessor();

public slots:
    /**
     * @brief 处理键盘事件
     * @param eventString 事件字符串
     * @param deviceInfo 设备信息
     */
    void processKeyEvent(const QString& eventString, const DeviceInfo& deviceInfo);

    /**
     * @brief 处理触摸事件
     * @param pos 触摸位置
     * @param deviceInfo 设备信息
     */
    void processTouchEvent(const QPoint& pos, const DeviceInfo& deviceInfo);

    /**
     * @brief 处理滑动事件
     * @param startPos 滑动起始位置
     * @param endPos 滑动结束位置
     * @param deviceInfo 设备信息
     */
    void processSwipeEvent(const QPoint& startPos, const QPoint& endPos, const DeviceInfo& deviceInfo);

    /**
     * @brief 事件处理完成回调
     * @param success 是否成功
     * @param eventString 事件字符串
     */
    void onEventProcessed(bool success, const QString& eventString);

private:
    /**
     * @brief 事件处理任务类
     */
    class EventTask : public QRunnable
    {
    public:
        EventTask(const QString& eventString, const DeviceInfo& deviceInfo, DeviceEventProcessor* processor);
        void run() override;

    private:
        QString m_eventString;
        DeviceInfo m_deviceInfo;
        DeviceEventProcessor* m_processor;
        DeviceEvent stringToEvent(const QString& eventString) const;
    };

    QThreadPool* m_threadPool;

signals:
    /**
     * @brief 事件处理完成信号
     * @param success 是否成功
     * @param eventString 事件字符串
     */
    void eventProcessed(bool success, const QString& eventString);
};