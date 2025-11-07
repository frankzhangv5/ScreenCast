#include "MirrorWindow.h"

#include "MessageBox.h"
#include "processor/DeviceEventProcessor.h"

#include <QAction>
#include <QApplication>
#include <QContextMenuEvent>
#include <QDateTime>
#include <QFileDialog>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QPixmap>
#include <QResizeEvent>
#include <QScreen>
#include <QStandardPaths>
#include <QThread>
#include <QTimer>
#include <QToolBar>
#include <QVBoxLayout>

MirrorWindow::MirrorWindow(const DeviceInfo& deviceInfo, QWidget* parent)
    : FramelessWindow(parent),
      m_deviceInfo(deviceInfo),
      m_currentRotation(deviceInfo.rotation),
      m_actualDisplayWidth(0),
      m_actualDisplayHeight(0),
      m_rotationTimer(nullptr)
{
    // Initialize QtAwesome
    m_awesome = new fa::QtAwesome(this);
    m_awesome->initFontAwesome();

    // Set default icon color to white
    QVariantMap options;
    options.insert("color", QColor(255, 255, 255));
    options.insert("size", ICON_SIZE.width()); // Set default icon size
    m_awesome->setDefaultOption("color", options.value("color"));
    m_awesome->setDefaultOption("size", options.value("size"));

    setupUI();
    setupContextMenu();

    // Initialize rotation timer
    m_rotationTimer = new QTimer(this);
    connect(m_rotationTimer, &QTimer::timeout, this, &MirrorWindow::checkDeviceRotation);
    // Set timer interval to 3000 milliseconds (3 seconds)
    m_rotationTimer->start(3000);

    // Initialize rotation thread
    m_rotationThread = new QThread(this);

    // Create worker object for rotation check in background thread
    m_rotationWorker = new QObject();
    m_rotationWorker->moveToThread(m_rotationThread);

    // Connect signal/slot for executing rotation check in background thread
    connect(this, &MirrorWindow::checkRotationInBackground, m_rotationWorker, [this]() {
        // This lambda executes in background thread
        doCheckRotationInBackground();
    });

    // Connect rotation change signal to UI thread handler
    connect(this, &MirrorWindow::rotationChanged, this, &MirrorWindow::handleRotationChanged, Qt::QueuedConnection);

    // Start rotation thread
    m_rotationThread->start();

    // Initialize device event processor (runs in non-UI thread)
    m_eventProcessor = new DeviceEventProcessor(this);

    // Create worker thread for network operations - no parent to avoid double deletion
    m_workerThread = new QThread();

    // Initialize screen provider in main thread, but don't start it yet
    m_screenProvider = new StreamScreenProvider(m_deviceInfo);
    // Move screen provider to worker thread
    m_screenProvider->moveToThread(m_workerThread);

    // Set screenProvider's parent to nullptr to avoid QObject auto-deletion mechanism
    m_screenProvider->setParent(nullptr);

    // Connect signals and slots
    connect(m_screenProvider,
            &StreamScreenProvider::frameDecoded,
            this,
            &MirrorWindow::handleFrameDecoded,
            Qt::QueuedConnection);
    connect(m_screenProvider,
            &StreamScreenProvider::errorOccurred,
            this,
            &MirrorWindow::handleScreenProviderError,
            Qt::QueuedConnection);

    // Connect to deviceSetupError signal from DeviceManager
    DeviceManager& deviceManager = getDeviceManager();
    connect(&deviceManager,
            &DeviceManager::deviceSetupError,
            this,
            [this](const QString& serial, const QString& errorMessage) {
                // Only handle errors for this specific device
                if (serial == m_deviceInfo.serial)
                {
                    handleScreenProviderError(errorMessage);
                }
            });

    // Connect sendKeyEvent signal to a custom slot that will pass deviceInfo
    connect(this, &MirrorWindow::sendKeyEvent, [this](const QString& eventString) {
        m_eventProcessor->processKeyEvent(eventString, m_deviceInfo);
    });

    // 连接触摸事件信号
    connect(this, &MirrorWindow::sendTouchEvent, m_eventProcessor, &DeviceEventProcessor::processTouchEvent);
    // 连接滑动事件信号
    connect(this, &MirrorWindow::sendSwipeEvent, m_eventProcessor, &DeviceEventProcessor::processSwipeEvent);

    // Connect event processor result signal to handler
    connect(m_eventProcessor, &DeviceEventProcessor::eventProcessed, this, &MirrorWindow::handleEventProcessed);

    // 连接线程启动信号到屏幕提供者的启动槽
    connect(m_workerThread, &QThread::started, m_screenProvider, &StreamScreenProvider::startProvide);

    // 启动工作线程
    m_workerThread->start();

    // 初始化鼠标拖动相关变量
    m_isDragging = false;

    // 初始检查设备旋转角度
    checkDeviceRotation();

    // 初始化滚轮事件时间戳
    m_lastWheelEventTime = 0;
}

MirrorWindow::~MirrorWindow()
{
    qDebug() << "MirrorWindow: destructor started";

    // Stop rotation timer
    if (m_rotationTimer)
    {
        m_rotationTimer->stop();
        delete m_rotationTimer;
        m_rotationTimer = nullptr;
    }

    // Stop rotation thread
    if (m_rotationThread && m_rotationThread->isRunning())
    {
        // Exit thread event loop
        m_rotationThread->quit();
        // Wait for thread to finish
        m_rotationThread->wait();
        // Clean up thread object
        delete m_rotationThread;
        m_rotationThread = nullptr;
    }

    // Release rotation worker object
    if (m_rotationWorker)
    {
        m_rotationWorker->deleteLater();
        m_rotationWorker = nullptr;
    }

    // Disconnect all signal/slot connections to prevent signals from triggering after object destruction
    disconnect();

    // Ensure thread and resources are properly cleaned up
    if (m_workerThread)
    {
        qDebug() << "MirrorWindow: checking worker thread status";

        // If thread is still running, wait for it to complete
        if (m_workerThread->isRunning())
        {
            qDebug() << "MirrorWindow: worker thread is running, requesting stop";

            // Ensure screenProvider stops in worker thread
            if (m_screenProvider)
            {
                QMetaObject::invokeMethod(m_screenProvider, "stopProvide", Qt::QueuedConnection);
            }

            // Wait for thread to exit with timeout to prevent deadlock
            if (!m_workerThread->wait(1000))
            { // Wait 1 second
                qWarning() << "MirrorWindow: worker thread didn't exit in time, forcing quit";
                m_workerThread->quit();
                if (!m_workerThread->wait(500))
                { // Wait another 500 milliseconds
                    qWarning() << "MirrorWindow: worker thread force stopped";
                }
            }
        }

        qDebug() << "MirrorWindow: deleting worker thread";
        delete m_workerThread;
        m_workerThread = nullptr;
    }

    // Now safely delete screenProvider
    if (m_screenProvider)
    {
        qDebug() << "MirrorWindow: deleting screen provider";
        delete m_screenProvider;
        m_screenProvider = nullptr;
    }

    // Delete event processor
    if (m_eventProcessor)
    {
        qDebug() << "MirrorWindow: deleting event processor";
        delete m_eventProcessor;
        m_eventProcessor = nullptr;
    }
    // m_rotationTimer has already been cleaned up above to avoid double deletion

    qDebug() << "MirrorWindow: destructor completed";
}
void MirrorWindow::setupUI()
{
    // 初始设置窗口最小尺寸
    setMinimumSize(320, 480);

    // 使用FramelessWindow提供的标题栏和状态栏
    getTitleBar()->setTitle(m_deviceInfo.name);

    // 获取页面容器
    QWidget* pageContainer = getPage();

    // Main container layout
    QVBoxLayout* mainLayout = new QVBoxLayout(pageContainer);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // Mirror area
    m_mirrorLabel = new QLabel(pageContainer);
    m_mirrorLabel->setAlignment(Qt::AlignCenter);
    m_mirrorLabel->setProperty("class", "MirrorLabel");
    m_mirrorLabel->setText(tr("Connecting to device..."));
    m_mirrorLabel->setContextMenuPolicy(Qt::CustomContextMenu);
    m_mirrorLabel->setScaledContents(true); // 设置为true让图像自动适应标签大小
    connect(m_mirrorLabel, &QLabel::customContextMenuRequested, this, [this](const QPoint& pos) {
        m_contextMenu->popup(m_mirrorLabel->mapToGlobal(pos));
    });
    mainLayout->addWidget(m_mirrorLabel, 1);

    // Set up toolbar
    QToolBar* toolBar = getTitleBar()->getToolBar();
    toolBar->setProperty("class", "Toolbar");
    toolBar->setVisible(true);
    toolBar->addSeparator();
    toolBar->setIconSize(ICON_SIZE);

    // 根据设备分辨率设置默认窗口尺寸（后续会通过checkDeviceRotation更新）
    if (m_deviceInfo.width > 0 && m_deviceInfo.height > 0)
    {
        // 设置默认窗口尺寸
        updateWindowSizeBasedOnRotation();
    }
    else
    {
        // Fallback to default size if width or height not available
        resize(800, 600);
    }

    // Add screenshot button (icon only, no text)
    m_screenshotAction =
        new QAction(m_awesome->icon(fa::fa_solid, fa::fa_camera).pixmap(ICON_SIZE), tr("Screenshot"), this);
    connect(m_screenshotAction, &QAction::triggered, [this]() { handleAction(ActionType::Screenshot); });
    toolBar->addAction(m_screenshotAction);

    // Connect signals and slots using lambda expressions
    // connect(getTitleBar(), &TitleBar::closeClicked, [this]() { handleAction(ActionType::Close); });
    connect(getTitleBar(), &TitleBar::minimizeClicked, [this]() { handleAction(ActionType::Minimize); });
}

// Helper function to create action with pixmap icon
QAction* MirrorWindow::createActionWithIcon(QMenu* menu, int iconId, const QString& text, ActionType actionType)
{
    QAction* action = menu->addAction(m_awesome->icon(fa::fa_solid, iconId).pixmap(ICON_SIZE), text);
    connect(action, &QAction::triggered, [this, actionType]() { handleAction(actionType); });
    return action;
}

void MirrorWindow::setupContextMenu()
{
    // Create context menu
    m_contextMenu = new QMenu(this);

    // Navigation actions directly in main menu
    createActionWithIcon(m_contextMenu, fa::fa_home, tr("Home"), ActionType::NavigateHome);
    createActionWithIcon(m_contextMenu, fa::fa_arrow_left, tr("Back"), ActionType::NavigateBack);
    createActionWithIcon(m_contextMenu, fa::fa_bars, tr("Menu"), ActionType::NavigateMenu);

    // Add separator between navigation and shortcuts
    m_contextMenu->addSeparator();

    // Shortcut actions directly in main menu
    createActionWithIcon(m_contextMenu, fa::fa_power_off, tr("Wakeup"), ActionType::ShortcutWakeup);
    createActionWithIcon(m_contextMenu, fa::fa_moon, tr("Sleep"), ActionType::ShortcutSleep);
    createActionWithIcon(m_contextMenu, fa::fa_unlock, tr("Unlock"), ActionType::ShortcutUnlock);

    // Add separator before reboot with improved style
    QAction* separator = m_contextMenu->addSeparator();
    // Set style for better spacing and narrower appearance
    separator->setProperty("menuSeparatorStyle", true);
    createActionWithIcon(m_contextMenu, fa::fa_power_off, tr("Shutdown"), ActionType::ShortcutShutdown);
    createActionWithIcon(m_contextMenu, fa::fa_arrows_rotate, tr("Reboot"), ActionType::ShortcutReboot);

    // Remove the submenu pointers since we're no longer using them
    // Note: We keep the member variables for backward compatibility but they're no longer used
}

// Unified handler for all actions
void MirrorWindow::handleAction(ActionType type)
{
    switch (type)
    {
        case ActionType::Close:
            // 调用close()会触发closeEvent，所有资源清理逻辑在closeEvent中处理
            qDebug() << "MirrorWindow: calling close()";
            close();
            break;

        case ActionType::Minimize:
            showMinimized();
            getStatusBar()->setStatusMessage(tr("Window minimized"));
            break;

        case ActionType::Screenshot: {
            // Capture screenshot of current mirror window
            if (!m_mirrorLabel->pixmap().isNull())
            {
                // Generate file name with device name
                QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
                QString defaultFileName =
                    QString("%1_screenshot_%2.png").arg(m_deviceInfo.name.replace(" ", "_"), timestamp);

                // Select save location with default path
                QString defaultPath = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
                QString fileName = QFileDialog::getSaveFileName(
                    this,
                    tr("Save Screenshot"),
                    defaultPath + "/" + defaultFileName,
                    tr("Images (*.png *.jpg *.bmp);;PNG Files (*.png);;JPEG Files (*.jpg);;BMP Files (*.bmp)"));

                if (!fileName.isEmpty())
                {
                    // Save with higher quality
                    QImage image = m_mirrorLabel->pixmap().toImage();
                    bool success = false;
                    if (fileName.endsWith(".png", Qt::CaseInsensitive))
                    {
                        success = image.save(fileName, "PNG", 9);
                    }
                    else if (fileName.endsWith(".jpg", Qt::CaseInsensitive))
                    {
                        success = image.save(fileName, "JPEG", 95);
                    }
                    else
                    {
                        success = image.save(fileName);
                    }

                    if (success)
                    {
                        // Show save success message
                        getStatusBar()->setStatusMessage(tr("Screenshot saved to: %1").arg(fileName));
                        // Provide feedback with a small animation or highlight
                        flashMirrorLabel();
                    }
                    else
                    {
                        getStatusBar()->setStatusMessage(tr("Failed to save screenshot"));
                    }
                }
            }
            else
            {
                // Show no data message
                getStatusBar()->setStatusMessage(tr("No screen data available for screenshot"));
            }
            break;
        }

        case ActionType::NavigateHome:
            // Send Home key command
            emit sendKeyEvent("HOME");
            getStatusBar()->setStatusMessage(tr("Navigate to Home"));
            flashMirrorLabel();
            break;

        case ActionType::NavigateBack:
            // Send Back key command
            emit sendKeyEvent("BACK");
            getStatusBar()->setStatusMessage(tr("Navigate Back"));
            flashMirrorLabel();
            break;

        case ActionType::NavigateMenu:
            // Send Menu key command
            emit sendKeyEvent("MENU");
            getStatusBar()->setStatusMessage(tr("Navigate to Menu"));
            flashMirrorLabel();
            break;

        case ActionType::ShortcutWakeup:
            // Send wakeup command
            emit sendKeyEvent("WAKEUP");
            getStatusBar()->setStatusMessage(tr("Wakeup device"));
            break;

        case ActionType::ShortcutSleep:
            // Send sleep command
            emit sendKeyEvent("SLEEP");
            getStatusBar()->setStatusMessage(tr("Put device to sleep"));
            break;

        case ActionType::ShortcutUnlock:
            // Send unlock command - first wakeup, then unlock after 3 seconds
            emit sendKeyEvent("WAKEUP");
            // Use a single-shot timer to send UNLOCK after 3 seconds
            QTimer::singleShot(1000, this, [this]() {
                emit sendKeyEvent("UNLOCK");
                getStatusBar()->setStatusMessage(tr("Unlock device"));
                flashMirrorLabel();
            });
            break;

        case ActionType::ShortcutShutdown:
            // Send shutdown command with confirmation
            int reply;
            reply = MessageBox::custom(this,
                                       tr("Confirm Shutdown"),
                                       tr("Are you sure you want to shutdown the device?"),
                                       MessageBox::ButtonType::YesNo,
                                       MessageBox::IconType::Warning);
            if (reply == QDialog::Accepted)
            {
                emit sendKeyEvent("SHUTDOWN");
                getStatusBar()->setStatusMessage(tr("Shutdown command sent"));
                // Auto close window after shutdown command
                QTimer::singleShot(2000, this, &MirrorWindow::close);
            }
            break;

        case ActionType::ShortcutReboot:
            // Send reboot command with confirmation
            reply = MessageBox::custom(this,
                                       tr("Confirm Reboot"),
                                       tr("Are you sure you want to reboot the device?"),
                                       MessageBox::ButtonType::YesNo,
                                       MessageBox::IconType::Warning);
            if (reply == QDialog::Accepted)
            {
                emit sendKeyEvent("REBOOT");
                getStatusBar()->setStatusMessage(tr("Reboot command sent"));
                // Auto close window after reboot command
                QTimer::singleShot(2000, this, &MirrorWindow::close);
            }
            break;
    }
}

void MirrorWindow::handleFrameDecoded(const QImage& frame)
{
    if (!frame.isNull())
    {
        // Convert QImage to QPixmap
        QPixmap pixmap = QPixmap::fromImage(frame);

        // Get the available space in the mirror area (excluding title and status bars)
        QRect availableRect = m_mirrorLabel->geometry();

        // 调整缩放策略：让图像宽度优先填满整个可用宽度，同时保持宽高比
        // 这样可以避免两边出现黑色间隙
        QPixmap scaledPixmap = pixmap.scaled(availableRect.width(), // 优先匹配宽度
                                             availableRect.height(),
                                             Qt::KeepAspectRatio,
                                             Qt::SmoothTransformation);

        // Set the scaled pixmap to the label
        m_mirrorLabel->setPixmap(scaledPixmap);
    }
}

void MirrorWindow::handleScreenProviderError(const QString& error)
{
    qWarning() << "Screen provider error:" << error;

    // 停止旋转检测定时器，避免界面宽高改变
    if (m_rotationTimer)
    {
        m_rotationTimer->stop();
    }

    // 初始化倒计时变量
    static int countdownSeconds = 5;
    countdownSeconds = 5;

    // 创建并配置定时器
    QTimer* countdownTimer = new QTimer(this);
    connect(countdownTimer, &QTimer::timeout, this, [this, error, countdownTimer]() mutable {
        if (countdownSeconds > 0)
        {
            // 更新状态栏和mirrorLabel显示错误和倒计时
            QString message = tr("Error: %1 (Closing in %2 seconds...)").arg(error).arg(countdownSeconds);
            getStatusBar()->setStatusMessage(message);

            // 在mirrorLabel上显示错误信息（使用HTML格式使其居中并设置样式）
            QString htmlMessage =
                QString("<center><div style='font-size: 16px; color: #ff4444;'>%1</div></center>").arg(message);
            m_mirrorLabel->setText(htmlMessage);
            m_mirrorLabel->setAlignment(Qt::AlignCenter);
            m_mirrorLabel->setWordWrap(true); // 设置自动换行

            countdownSeconds--;
        }
        else
        {
            // 倒计时结束，停止定时器并关闭窗口
            countdownTimer->stop();
            countdownTimer->deleteLater();
            this->close();
        }
    });

    // 立即触发一次显示初始消息
    QString initialMessage = tr("Error: %1 (Closing in %2 seconds...)").arg(error).arg(countdownSeconds);
    getStatusBar()->setStatusMessage(initialMessage);

    // 在mirrorLabel上立即显示初始错误信息
    QString initialHtmlMessage =
        QString("<center><div style='font-size: 16px; color: #ff4444;'>%1</div></center>").arg(initialMessage);
    m_mirrorLabel->setText(initialHtmlMessage);
    m_mirrorLabel->setAlignment(Qt::AlignCenter);
    m_mirrorLabel->setWordWrap(true); // 设置自动换行
    m_mirrorLabel->setAlignment(Qt::AlignCenter);

    countdownSeconds--;

    // 启动定时器，每秒触发一次
    countdownTimer->start(1000);
}

void MirrorWindow::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);

    // If there's a pixmap displayed, scale it to fit the new window size
    if (!m_mirrorLabel->pixmap().isNull())
    {
        // Get the available space in the mirror area
        QRect availableRect = m_mirrorLabel->geometry();

        // 调整缩放策略：让图像宽度优先填满整个可用宽度，同时保持宽高比
        QPixmap scaledPixmap = m_mirrorLabel->pixmap().scaled(availableRect.width(), // 优先匹配宽度
                                                              availableRect.height(),
                                                              Qt::KeepAspectRatio,
                                                              Qt::SmoothTransformation);

        // Update the label with the scaled pixmap
        m_mirrorLabel->setPixmap(scaledPixmap);
    }
}

void MirrorWindow::closeEvent(QCloseEvent* event)
{
    qDebug() << "MirrorWindow: closeEvent called";

    // Stop screen provider in worker thread via signal/slot
    if (m_workerThread && m_workerThread->isRunning() && m_screenProvider)
    {
        qDebug() << "MirrorWindow: requesting screen provider to stop";
        // Try to call stopProvide method directly
        bool invokeSuccess = QMetaObject::invokeMethod(m_screenProvider, "stopProvide", Qt::QueuedConnection);
        if (!invokeSuccess)
        {
            qWarning() << "MirrorWindow: Failed to invoke stopProvide method, trying stopCapture as fallback";
            // If stopProvide call fails, try calling stopCapture as fallback
            QMetaObject::invokeMethod(m_screenProvider, "stopCapture", Qt::QueuedConnection);
        }

        // Add a brief delay to ensure stop operation has a chance to start executing
        // This won't block UI thread as it only waits for event queue processing
        QCoreApplication::processEvents(QEventLoop::AllEvents, 200);
    }

    // Stop event processor if it exists
    if (m_eventProcessor)
    {
        qDebug() << "MirrorWindow: stopping event processor";
        // If DeviceEventProcessor has a stop method, it should be called here
    }

    // 停止旋转定时器
    if (m_rotationTimer && m_rotationTimer->isActive())
    {
        m_rotationTimer->stop();
    }

    qDebug() << "MirrorWindow: closeEvent completed";
    // 接受关闭事件，只关闭当前窗口
    QWidget::closeEvent(event);
}

// Helper method to provide visual feedback when an action is performed
void MirrorWindow::flashMirrorLabel()
{
    // Store original style
    QString originalStyle = m_mirrorLabel->styleSheet();

    // Create flash effect
    m_mirrorLabel->setStyleSheet(originalStyle + "background-color: rgba(255, 255, 255, 30);");

    // Reset style after a short delay
    QTimer::singleShot(200, this, [this, originalStyle]() { m_mirrorLabel->setStyleSheet(originalStyle); });
}

void MirrorWindow::handleEventProcessed(bool success, const QString& eventString)
{
    // Handle event processing result
    if (!success)
    {
        getStatusBar()->setStatusMessage(tr("Failed to send command: %1").arg(eventString));
    }
    // Success case doesn't need additional processing as status is already set in handleAction
}

void MirrorWindow::mousePressEvent(QMouseEvent* event)
{
    // 使用更准确的方法来检测点击是否在镜像标签上
    // 直接判断点击坐标是否在mirrorLabel的可视区域内
    QPoint eventPos = event->pos();

    // 获取mirrorLabel在窗口中的全局位置
    QPoint mirrorLabelGlobalPos = m_mirrorLabel->mapToGlobal(QPoint(0, 0));
    QRect mirrorLabelGlobalRect(mirrorLabelGlobalPos, m_mirrorLabel->size());

    // 获取鼠标全局位置（使用非废弃方法）
    QPoint globalPos = event->globalPosition().toPoint();

    // 方法1：使用全局坐标进行检测
    bool isInLabelGlobal = mirrorLabelGlobalRect.contains(globalPos);

    // 方法2：使用窗口坐标进行检测（mirrorLabel的geometry是相对于父窗口的）
    bool isInLabelLocal = m_mirrorLabel->geometry().contains(eventPos);

    // 方法3：使用mirrorLabel自己的坐标系统
    QPoint posInLabel = m_mirrorLabel->mapFrom(this, eventPos);
    bool isInLabelOwn = m_mirrorLabel->rect().contains(posInLabel);

    // 增加调试信息，帮助排查问题
    qDebug() << "Mouse Press - Event pos:" << eventPos << "Global pos:" << globalPos
             << "Mirror Label pos:" << m_mirrorLabel->pos() << "Mirror Label geometry:" << m_mirrorLabel->geometry()
             << "Mirror Label size:" << m_mirrorLabel->size() << "Pos in label:" << posInLabel
             << "isInLabelGlobal:" << isInLabelGlobal << "isInLabelLocal:" << isInLabelLocal
             << "isInLabelOwn:" << isInLabelOwn;

    // 使用isInLabelOwn作为最终判断，这是最准确的方法
    if (isInLabelOwn)
    {
        if (event->button() == Qt::LeftButton)
        {
            // 左键点击，记录拖动起始位置
            m_isDragging = true;
            m_dragStartPos = event->pos();
            m_lastPos = event->pos();

            qDebug() << "Mouse Press - Recognized as click inside mirrorLabel";
        }
        // 移除了右键点击发送BACK事件的功能
    }
    else
    {
        qDebug() << "Mouse Press - Click outside mirrorLabel";
    }

    // 调用基类的mousePressEvent以确保其他功能正常
    QWidget::mousePressEvent(event);
}

void MirrorWindow::updateWindowSizeBasedOnRotation()
{
    if (m_deviceInfo.width <= 0 || m_deviceInfo.height <= 0)
    {
        return;
    }

    // 获取电脑屏幕的可用尺寸
    QScreen* screen = QGuiApplication::primaryScreen();
    QRect screenGeometry = screen->availableGeometry();
    int screenWidth = screenGeometry.width();
    int screenHeight = screenGeometry.height();

    // 设置最大缩放比例，确保窗口不超过屏幕的80%
    const float MAX_SCALE_FACTOR = 0.8f;
    int maxWindowWidth = static_cast<int>(screenWidth * MAX_SCALE_FACTOR);
    int maxWindowHeight = static_cast<int>(screenHeight * MAX_SCALE_FACTOR);

    // 根据旋转角度确定实际显示的宽高
    int displayWidth = m_deviceInfo.width;
    int displayHeight = m_deviceInfo.height;

    if (m_currentRotation == 90 || m_currentRotation == 270)
    {
        // 旋转90或270度时，对调宽高
        displayWidth = m_deviceInfo.height;
        displayHeight = m_deviceInfo.width;
    }

    // 保存当前显示窗口的实际宽高
    m_actualDisplayWidth = displayWidth;
    m_actualDisplayHeight = displayHeight;

    // 计算设备的原始宽高比
    float deviceAspectRatio = static_cast<float>(displayWidth) / displayHeight;

    // 计算适合屏幕的最大窗口尺寸，保持原始宽高比
    int windowWidth, windowHeight;

    // 先尝试以宽度为基准进行缩放
    windowWidth = maxWindowWidth;
    windowHeight = static_cast<int>(windowWidth / deviceAspectRatio);

    // 如果高度超出屏幕限制，则以高度为基准进行缩放
    if (windowHeight > maxWindowHeight)
    {
        windowHeight = maxWindowHeight;
        windowWidth = static_cast<int>(windowHeight * deviceAspectRatio);
    }

    // 确保窗口不会过小
    int minWindowSize = static_cast<int>(std::min(maxWindowWidth, maxWindowHeight) * 0.3f);
    if (windowWidth < minWindowSize || windowHeight < minWindowSize)
    {
        if (deviceAspectRatio >= 1.0f)
        {
            // 横屏设备，以宽度为基准
            windowWidth = minWindowSize;
            windowHeight = static_cast<int>(windowWidth / deviceAspectRatio);
        }
        else
        {
            // 竖屏设备，以高度为基准
            windowHeight = minWindowSize;
            windowWidth = static_cast<int>(windowHeight * deviceAspectRatio);
        }
    }

    // 检查窗口状态，如果是最大化或最小化，先恢复正常状态
    if (windowState() & (Qt::WindowMaximized | Qt::WindowMinimized))
    {
        showNormal();
    }

    if (getStatusBar() && getTitleBar())
        windowHeight += getStatusBar()->height() + getTitleBar()->height();
    // 使用resize方法更新窗口尺寸
    resize(windowWidth, windowHeight);

    // 刷新窗口内容
    update();

    qDebug() << "Window size updated based on rotation:" << m_currentRotation << "degrees. Window size:" << windowWidth
             << "x" << windowHeight << "with aspect ratio:" << deviceAspectRatio;
}

void MirrorWindow::checkDeviceRotation()
{
    // 触发信号，在后台线程中执行实际的旋转检查
    emit checkRotationInBackground();
}

void MirrorWindow::doCheckRotationInBackground()
{
    // 此方法在后台线程中执行
    // 获取设备代理并直接调用deviceRotation方法，传入设备序列号
    DeviceProxy* deviceProxy = getDeviceProxy(m_deviceInfo.type);
    if (deviceProxy)
    {
        int newRotation = deviceProxy->deviceRotation(m_deviceInfo.serial);

        // 如果旋转角度发生变化，发送信号到UI线程
        if (newRotation != m_currentRotation)
        {
            qDebug() << "Device rotation changed from" << m_currentRotation << "to" << newRotation << "degrees";

            // 发送信号到UI线程
            emit rotationChanged(newRotation);
        }
    }
}

void MirrorWindow::handleRotationChanged(int newRotation)
{
    // 此方法在UI线程中执行
    m_currentRotation = newRotation;
    updateWindowSizeBasedOnRotation();
}

void MirrorWindow::mouseMoveEvent(QMouseEvent* event)
{
    // 只有在拖动状态下才处理移动事件
    if (m_isDragging && (event->buttons() & Qt::LeftButton))
    {
        // 计算移动距离
        QPoint delta = event->pos() - m_lastPos;
        m_lastPos = event->pos();

        // 只有当移动距离足够大时才视为有效移动（可以调整阈值）
        if (qAbs(delta.x()) >= 3 || qAbs(delta.y()) >= 3)
        {
            // 获取原始图像尺寸和显示尺寸
            QSize originalSize = QSize(m_actualDisplayWidth, m_actualDisplayHeight);
            QSize displaySize = m_mirrorLabel->size();

            if (!originalSize.isEmpty() && !displaySize.isEmpty())
            {
                // 计算缩放比例
                qreal scaleX = static_cast<qreal>(originalSize.width()) / displaySize.width();
                qreal scaleY = static_cast<qreal>(originalSize.height()) / displaySize.height();

                // 计算缩放后的移动距离
                int scaledDeltaX = static_cast<int>(delta.x() * scaleX);
                int scaledDeltaY = static_cast<int>(delta.y() * scaleY);

                // 在日志中显示为正值，但在实际处理中保留原始方向
                qDebug() << "Mouse moved with delta:" << delta
                         << "Scaled delta:" << QPoint(qAbs(scaledDeltaX), qAbs(scaledDeltaY));
            }
        }
    }

    // 调用基类的mouseMoveEvent
    QWidget::mouseMoveEvent(event);
}

void MirrorWindow::wheelEvent(QWheelEvent* event)
{
    // 检查事件是否发生在镜像标签上
    QPoint globalPos = event->globalPosition().toPoint();
    if (m_mirrorLabel->geometry().contains(m_mirrorLabel->mapFromGlobal(globalPos)))
    {
        // 时间节流机制：限制滚轮事件处理频率
        const int THROTTLE_INTERVAL_MS = 200; // 200毫秒的节流间隔
        qint64 currentTime = QDateTime::currentMSecsSinceEpoch();

        if (currentTime - m_lastWheelEventTime < THROTTLE_INTERVAL_MS)
        {
            // 如果距离上次滚轮事件时间太短，忽略当前事件
            event->accept();
            return;
        }

        // 更新上次滚轮事件时间戳
        m_lastWheelEventTime = currentTime;

        // 获取原始图像尺寸和显示尺寸
        QSize originalSize = QSize(m_actualDisplayWidth, m_actualDisplayHeight);
        QSize displaySize = m_mirrorLabel->size();

        if (!originalSize.isEmpty() && !displaySize.isEmpty())
        {
            // 计算缩放比例
            qreal scaleX = static_cast<qreal>(originalSize.width()) / displaySize.width();
            qreal scaleY = static_cast<qreal>(originalSize.height()) / displaySize.height();

            // 计算相对于镜像标签的位置
            QPoint localPos = m_mirrorLabel->mapFromGlobal(globalPos);

            // 转换为原始图像坐标
            int centerX = static_cast<int>(localPos.x() * scaleX);
            int centerY = static_cast<int>(localPos.y() * scaleY);

            // 确保坐标在有效范围内
            centerX = qBound(0, centerX, originalSize.width() - 1);
            centerY = qBound(0, centerY, originalSize.height() - 1);

            // 获取滚轮滚动量，通常每个轮齿为120
            int wheelDelta = event->angleDelta().y();

            // 基础滑动距离，增大滚动幅度
            const int BASE_SWIPE_DISTANCE = static_cast<int>(originalSize.height() * 0.10); // 10%

            // 根据滚轮滚动量计算实际滑动距离
            int actualSwipeDistance = static_cast<int>(BASE_SWIPE_DISTANCE * qAbs(wheelDelta) / 120.0);

            // 设置最小和最大滑动距离，增大滚动效果
            actualSwipeDistance = qBound(static_cast<int>(originalSize.height() * 0.05), // 最小5%
                                         actualSwipeDistance,
                                         static_cast<int>(originalSize.height() * 0.30)); // 最大30%

            // 根据滚轮方向确定滑动方向
            QPoint startPos, endPos;
            if (wheelDelta > 0)
            {
                // 滚轮向上滚动，转换为向上的滑动
                startPos = QPoint(centerX, centerY + actualSwipeDistance);
                endPos = QPoint(centerX, centerY);
                // 确保起始位置在有效范围内
                startPos.setY(qBound(0, startPos.y(), originalSize.height() - 1));
            }
            else
            {
                // 滚轮向下滚动，转换为向下的滑动
                startPos = QPoint(centerX, centerY);
                endPos = QPoint(centerX, centerY + actualSwipeDistance);
                // 确保结束位置在有效范围内
                endPos.setY(qBound(0, endPos.y(), originalSize.height() - 1));
            }

            qDebug() << "Mouse wheel event converted to swipe from:" << startPos << "to:" << endPos
                     << "with delta:" << wheelDelta;

            // 发送滑动事件
            emit sendSwipeEvent(startPos, endPos, m_deviceInfo);

            // 阻止事件继续传播，避免窗口滚动等默认行为
            event->accept();
            return;
        }
    }

    // 调用基类的wheelEvent
    QWidget::wheelEvent(event);
}

void MirrorWindow::mouseReleaseEvent(QMouseEvent* event)
{
    // 如果是左键释放且处于拖动状态，处理滑动结束
    if (event->button() == Qt::LeftButton && m_isDragging)
    {
        // 计算总的滑动距离
        QPoint totalDelta = event->pos() - m_dragStartPos;

        // 获取原始图像尺寸和显示尺寸
        QSize originalSize = QSize(m_actualDisplayWidth, m_actualDisplayHeight);
        QSize displaySize = m_mirrorLabel->size();

        if (!originalSize.isEmpty() && !displaySize.isEmpty())
        {
            // 计算缩放比例
            qreal scaleX = static_cast<qreal>(originalSize.width()) / displaySize.width();
            qreal scaleY = static_cast<qreal>(originalSize.height()) / displaySize.height();

            // 计算起始和结束位置 - 使用mapFrom方法进行准确的坐标转换
            QPoint startLocal = m_mirrorLabel->mapFrom(this, m_dragStartPos);
            QPoint endLocal = m_mirrorLabel->mapFrom(this, event->pos());

            // 转换为原始图像坐标
            int startX = static_cast<int>(startLocal.x() * scaleX);
            int startY = static_cast<int>(startLocal.y() * scaleY);
            int endX = static_cast<int>(endLocal.x() * scaleX);
            int endY = static_cast<int>(endLocal.y() * scaleY);

            // 确保坐标在有效范围内
            startX = qBound(0, startX, originalSize.width() - 1);
            startY = qBound(0, startY, originalSize.height() - 1);
            endX = qBound(0, endX, originalSize.width() - 1);
            endY = qBound(0, endY, originalSize.height() - 1);

            // 根据旋转角度调整坐标
            int adjustedStartX = startX;
            int adjustedStartY = startY;
            int adjustedEndX = endX;
            int adjustedEndY = endY;

            // 这里根据当前旋转角度调整坐标
            // 注意：这里只是调整了坐标值，但实际的触摸事件发送仍然使用原始设备信息
            // 在Android设备端会处理旋转适配

            // 增加更详细的坐标调试信息
            qDebug() << "Mouse Release - Original Window Pos:" << event->pos();
            qDebug() << "Mouse Release - Mirror Label Pos:" << m_mirrorLabel->pos();
            qDebug() << "Mouse Release - Local Coordinates in MirrorLabel:" << endLocal;
            qDebug() << "Mouse Release - Original Size:" << originalSize << "Display Size:" << displaySize;
            qDebug() << "Mouse Release - Scale Factors: X=" << scaleX << "Y=" << scaleY;
            qDebug() << "Mouse Release - Transformed Coordinates (pre-bound):" << QPoint(endX, endY);
            qDebug() << "Mouse Release - Transformed Coordinates (post-bound):" << QPoint(endX, endY);
            qDebug() << "Mouse Release - Current Rotation:" << m_currentRotation;

            QPoint startPos(adjustedStartX, adjustedStartY);
            QPoint endPos(adjustedEndX, adjustedEndY);

            // 增加调整后坐标的调试信息
            qDebug() << "Mouse Release - Final Adjusted Coordinates:" << endPos;

            // 根据移动距离判断是点击还是滑动
            if (qAbs(totalDelta.x()) >= 20 || qAbs(totalDelta.y()) >= 20)
            { // 滑动距离足够大，发送滑动事件
                qDebug() << "Sending swipe event from:" << startPos << "to:" << endPos;
                emit sendSwipeEvent(startPos, endPos, m_deviceInfo);
            }
            else
            { // 移动距离很小，视为点击事件
                qDebug() << "Sending touch event at:" << endPos;
                emit sendTouchEvent(endPos, m_deviceInfo);
            }
        }

        // 重置拖动状态
        m_isDragging = false;
    }

    // 调用基类的mouseReleaseEvent
    QWidget::mouseReleaseEvent(event);
}