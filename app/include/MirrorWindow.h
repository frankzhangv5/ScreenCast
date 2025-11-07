#pragma once

#include "FramelessWindow.h"
#include "QtAwesome.h"
#include "provider/StreamScreenProvider.h"

#include <QAction>
#include <QImage>
#include <QMenu>
#include <QString>
#include <QThread>
#include <QWidget>

// Forward declaration
class DeviceEventProcessor;

// Define constants
const QSize ICON_SIZE(14, 14);

// Define action type enumeration
enum class ActionType
{
    Close,
    Minimize,
    Screenshot,
    NavigateHome,
    NavigateBack,
    NavigateMenu,
    ShortcutWakeup,
    ShortcutSleep,
    ShortcutUnlock,
    ShortcutShutdown,
    ShortcutReboot
};

class MirrorWindow : public FramelessWindow
{
    Q_OBJECT

public:
    explicit MirrorWindow(const DeviceInfo& deviceInfo, QWidget* parent = nullptr);
    ~MirrorWindow();

signals:
    void sendKeyEvent(const QString& keyEvent);
    void sendTouchEvent(const QPoint& pos, const DeviceInfo& deviceInfo);
    void sendSwipeEvent(const QPoint& startPos, const QPoint& endPos, const DeviceInfo& deviceInfo);
    void checkRotationInBackground();      // Signal to trigger background rotation check
    void rotationChanged(int newRotation); // Signal when rotation changes

private slots:
    void handleAction(ActionType type);
    void handleFrameDecoded(const QImage& frame);
    void handleScreenProviderError(const QString& error);
    void handleRotationChanged(int newRotation); // Slot to handle rotation changes
    void doCheckRotationInBackground();          // Perform rotation check in background thread

protected:
    void resizeEvent(QResizeEvent* event) override;
    void closeEvent(QCloseEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

private:
    void setupUI();
    void setupContextMenu();
    QAction* createActionWithIcon(QMenu* menu, int iconId, const QString& text, ActionType actionType);
    void flashMirrorLabel();

    // Event processing handler
    void handleEventProcessed(bool success, const QString& eventString);

    // Rotation handling methods
    void updateWindowSizeBasedOnRotation();
    void checkDeviceRotation(); // Trigger background check

    DeviceInfo m_deviceInfo;
    QLabel* m_mirrorLabel;
    QAction* m_screenshotAction;
    QMenu* m_contextMenu;
    QMenu* m_navigateMenu;
    QMenu* m_shortcutMenu;
    fa::QtAwesome* m_awesome;

    // Screen data provider
    StreamScreenProvider* m_screenProvider;

    // Device event processor (runs in non-UI thread)
    DeviceEventProcessor* m_eventProcessor;

    // Worker thread for network operations
    QThread* m_workerThread;

    // Mouse dragging related variables
    bool m_isDragging;
    QPoint m_dragStartPos;
    QPoint m_lastPos;

    // Rotation handling related variables
    int m_currentRotation;     // Current device rotation angle (0, 90, 180, 270 degrees)
    int m_actualDisplayWidth;  // Actual width of current display window
    int m_actualDisplayHeight; // Actual height of current display window
    QTimer* m_rotationTimer;
    qint64 m_lastWheelEventTime; // For throttling wheel events
    QThread* m_rotationThread;   // Background thread for rotation checks
    QObject* m_rotationWorker;   // Worker object for rotation checks
};