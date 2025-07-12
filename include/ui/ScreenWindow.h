#pragma once
#include "FramelessWindow.h"
#include "core/StreamReader.h"
#include "device/Device.h"

#include <QDateTime>
#include <QLabel>
#include <QMenu>
#include <QToolButton>
#include <QVBoxLayout>

class ScreenWindow : public FramelessWindow
{
    Q_OBJECT
public:
    explicit ScreenWindow(const DeviceInfo& dev, QWidget* parent = nullptr);

protected:
    void showEvent(QShowEvent* event) override;
    void closeEvent(QCloseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;

private slots:
    void updateVideoFrame(const QImage& frame);
    void onErrorOccurred(const QString& msg);

private:
    QMenu* createNavMenu();
    QMenu* createShortcutMenu();
    void showContextMenu(const QPoint& globalPos);
    QMenu* createContextMenu();

    DeviceInfo m_device;
    StreamReader* m_streamReader;
    QLabel* screenLabel;
    bool m_isDragging = false;
    qint64 m_pressTime = 0;
    qint64 m_lastWheelTime = 0;
    int m_accumulatedWheelDelta = 0;
    QPoint m_lastMousePos;
};
