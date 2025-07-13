#include "ui/ScreenWindow.h"

#include <QApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QLabel>
#include <QMenu>
#include <QStandardPaths>
#include <QToolButton>
#include <QVBoxLayout>
#include <QtConcurrent>

ScreenWindow::ScreenWindow(const DeviceInfo& dev, QWidget* parent) : FramelessWindow(parent, dev.width), m_device(dev)
{
    getPage()->setFixedSize(dev.width, dev.height);

    // Initialize screenshot watcher
    m_screenshotWatcher = new QFutureWatcher<QByteArray>(this);
    connect(m_screenshotWatcher, &QFutureWatcher<QByteArray>::finished, this, &ScreenWindow::onScreenshotCompleted);

    // Page content: QLabel for displaying screen recording
    QWidget* page = getPage();
    QVBoxLayout* layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);

    m_streamReader = new StreamReader(dev);
    connect(m_streamReader, &StreamReader::frameDecoded, this, &ScreenWindow::updateVideoFrame);
    connect(m_streamReader, &StreamReader::errorOccurred, this, &ScreenWindow::onErrorOccurred);

    screenLabel = new QLabel(tr("Connecting ..."), page);
    screenLabel->setAlignment(Qt::AlignCenter);
    screenLabel->setStyleSheet("background:#222; color:#fff; font-size:18px;");
    layout->addWidget(screenLabel, 1); // Take up remaining space

    page->setLayout(layout);

    // Set title bar to display device name
    getTitleBar()->setIconText(m_device.name);

    fa::QtAwesome* awesome = new fa::QtAwesome(this);
    awesome->initFontAwesome();
    QVariantMap options;
    options.insert("color", QColor(255, 255, 255));
    QIcon screenshotIcon = awesome->icon(fa::fa_solid, fa::fa_camera, options).pixmap(14, 14);
    QIcon settingsIcon = awesome->icon(fa::fa_solid, fa::fa_bolt, options).pixmap(14, 14);
    QIcon navIcon = awesome->icon(fa::fa_solid, fa::fa_bars, options).pixmap(14, 14);

    QToolBar* toolBar = getTitleBar()->getToolBar();
    toolBar->setFloatable(false); // Don't allow dragging out as independent panel
    toolBar->addSeparator();
    toolBar->setIconSize(QSize(14, 14));

    // Add screenshot button to toolbar for quick access
    QAction* screenshotAction = toolBar->addAction(screenshotIcon, "");
    // Set click event callback
    connect(screenshotAction, &QAction::triggered, this, &ScreenWindow::takeScreenshot);
}

void ScreenWindow::showEvent(QShowEvent* event)
{
    qDebug() << "ScreenWindow::showEvent";
    m_streamReader->startCapture();
    FramelessWindow::showEvent(event);
}

void ScreenWindow::closeEvent(QCloseEvent* event)
{
    qDebug() << "ScreenWindow::closeEvent";
    m_streamReader->stopCapture();
    FramelessWindow::closeEvent(event);
}

void ScreenWindow::mousePressEvent(QMouseEvent* event)
{
    QPoint relativePos = screenLabel->mapFrom(this, event->pos());
    if (!screenLabel->rect().contains(relativePos))
    {
        return;
    }

    if (event->button() == Qt::LeftButton)
    {
        m_isDragging = false;
        m_pressTime = QDateTime::currentDateTime().toMSecsSinceEpoch();
        m_lastMousePos = relativePos;
    }
    else if (event->button() == Qt::RightButton)
    {
        // Show context menu on right click
        showContextMenu(event->globalPos());
    }
}

void ScreenWindow::wheelEvent(QWheelEvent* event)
{
    QPoint relativePos = screenLabel->mapFrom(this, event->position().toPoint());
    if (!screenLabel->rect().contains(relativePos))
    {
        return;
    }

    int delta = event->angleDelta().y();

    // Accumulate wheel delta for smoother scrolling
    m_accumulatedWheelDelta += delta;

    // Throttling mechanism: avoid too frequent events while allowing small increments to accumulate
    qint64 currentTime = QDateTime::currentDateTime().toMSecsSinceEpoch();
    if (currentTime - m_lastWheelTime < 20) // 20ms throttling for better responsiveness
    {
        return;
    }

    // Only execute swipe when accumulated delta is large enough
    if (qAbs(m_accumulatedWheelDelta) < 30) // Lower minimum swipe threshold for easier triggering
    {
        return;
    }

    m_lastWheelTime = currentTime;

    // Calculate swipe distance based on accumulated wheel delta
    int scrollDistance = qAbs(m_accumulatedWheelDelta) / 6; // Increase swipe amplitude by reducing divisor
    scrollDistance = qBound(50, scrollDistance, 200);       // Increase swipe range to 50-200 pixels

    QPoint startPos = relativePos;
    QPoint endPos = relativePos;
    endPos.setY(relativePos.y() + (m_accumulatedWheelDelta > 0
                                       ? -scrollDistance
                                       : scrollDistance)); // Upward scroll corresponds to upward swipe

    if (DeviceProxy* proxy = DeviceManager::instance().proxyForType(m_device.type))
    {
        // Use shorter duration to reduce stuttering
        proxy->sendSwipeEvent(m_device, startPos, endPos, 120);
    }

    // Reset accumulated delta
    m_accumulatedWheelDelta = 0;

    if (event->button() == Qt::LeftButton)
    {
        m_isDragging = false;
        m_pressTime = QDateTime::currentDateTime().toMSecsSinceEpoch();
        m_lastMousePos = relativePos;
    }
}

void ScreenWindow::mouseMoveEvent(QMouseEvent* event)
{
    QPoint relativePos = screenLabel->mapFrom(this, event->pos());
    if (!screenLabel->rect().contains(relativePos))
    {
        return;
    }
    QPoint delta = relativePos - m_lastMousePos;
    int distance = delta.manhattanLength();
    if (distance > 5)
    {
        m_isDragging = true;
    }
}

void ScreenWindow::mouseReleaseEvent(QMouseEvent* event)
{
    QPoint relativePos = screenLabel->mapFrom(this, event->pos());
    if (!screenLabel->rect().contains(relativePos))
    {
        return;
    }
    if (event->button() == Qt::LeftButton)
    {
        QPoint delta = relativePos - m_lastMousePos;
        int distance = delta.manhattanLength();
        int releaseTime = QDateTime::currentDateTime().toMSecsSinceEpoch();
        if (!m_isDragging && (releaseTime - m_pressTime < 300) && distance < 5)
        {
            // Click event
            DeviceProxy* proxy = DeviceManager::instance().proxyForType(m_device.type);
            if (proxy)
            {
                proxy->sendTouchEvent(m_device, relativePos);
            }
        }
        else if (m_isDragging)
        {
            // Drag event
            DeviceProxy* proxy = DeviceManager::instance().proxyForType(m_device.type);
            if (proxy)
            {
                proxy->sendSwipeEvent(m_device, m_lastMousePos, relativePos);
            }
        }
    }
}

void ScreenWindow::updateVideoFrame(const QImage& frame)
{
    QPixmap pix = QPixmap::fromImage(frame.scaled(screenLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    screenLabel->setPixmap(pix);
}

void ScreenWindow::onErrorOccurred(const QString& msg)
{
    qWarning() << "ScreenWindow:: Error occurred:" << msg;
    screenLabel->setText(msg);
    screenLabel->setStyleSheet("background:#222; color:#fff; font-size:14px;");
    getStatusBar()->setStatus(tr("Error occurred: please close the window and try again."));
}

QMenu* ScreenWindow::createNavMenu()
{
    fa::QtAwesome* awesome = new fa::QtAwesome(this);
    awesome->initFontAwesome();
    QVariantMap options;
    options.insert("color", QColor(255, 255, 255));
    QIcon homeIcon = awesome->icon(fa::fa_solid, fa::fa_home, options).pixmap(12, 12);
    QIcon backIcon = awesome->icon(fa::fa_solid, fa::fa_arrow_left, options).pixmap(12, 12);
    QIcon menuIcon = awesome->icon(fa::fa_solid, fa::fa_bars, options).pixmap(12, 12);

    QMenu* navMenu = new QMenu(this);
    navMenu->setObjectName("navMenu");
    DeviceProxy* proxy = DeviceManager::instance().proxyForType(m_device.type);
    if (!proxy)
        return navMenu;

    if (proxy->supportEvent(DeviceEvent::HOME))
    {
        navMenu->addAction(homeIcon, tr("Home"), this, [this]() {
            DeviceProxy* proxy = DeviceManager::instance().proxyForType(m_device.type);
            if (proxy)
                proxy->sendEvent(m_device, DeviceEvent::HOME);
        });
    }
    if (proxy->supportEvent(DeviceEvent::BACK))
    {
        navMenu->addAction(backIcon, tr("Back"), this, [this]() {
            DeviceProxy* proxy = DeviceManager::instance().proxyForType(m_device.type);
            if (proxy)
                proxy->sendEvent(m_device, DeviceEvent::BACK);
        });
    }
    if (proxy->supportEvent(DeviceEvent::MENU))
    {
        navMenu->addAction(menuIcon, tr("Menu"), this, [this]() {
            DeviceProxy* proxy = DeviceManager::instance().proxyForType(m_device.type);
            if (proxy)
                proxy->sendEvent(m_device, DeviceEvent::MENU);
        });
    }
    navMenu->setStyleSheet(R"(
                QMenu {
                    background-color: #008D4E;
                    border-radius: 6px;
                    padding: 4px 0;
                    color: white;
                }
                QMenu::item {
                    background-color: transparent;
                    padding: 4px 0px;
                    margin: 2px 4px;
                    font: bold 12px;
                    min-width: 70px;
                }
                QMenu::item:selected {
                    background-color: #00804E;
                }
                QMenu::separator {
                    height: 1px;
                    background: white;
                    margin: 4px 2px;
                }
    )");
    return navMenu;
}

QMenu* ScreenWindow::createShortcutMenu()
{
    fa::QtAwesome* awesome = new fa::QtAwesome(this);
    awesome->initFontAwesome();
    QVariantMap options;
    options.insert("color", QColor(255, 255, 255));
    QIcon wakeupIcon = awesome->icon(fa::fa_solid, fa::fa_sun, options).pixmap(12, 12);
    QIcon sleepIcon = awesome->icon(fa::fa_solid, fa::fa_moon, options).pixmap(12, 12);
    QIcon rotateIcon = awesome->icon(fa::fa_solid, fa::fa_rotate, options).pixmap(12, 12);
    QIcon unlockIcon = awesome->icon(fa::fa_solid, fa::fa_unlock, options).pixmap(12, 12);
    QIcon rebootIcon = awesome->icon(fa::fa_solid, fa::fa_sync, options).pixmap(12, 12);
    QIcon shutdownIcon = awesome->icon(fa::fa_solid, fa::fa_power_off, options).pixmap(12, 12);

    QMenu* shortcutMenu = new QMenu(this);
    shortcutMenu->setObjectName("shortcutMenu");
    DeviceProxy* proxy = DeviceManager::instance().proxyForType(m_device.type);
    if (!proxy)
        return shortcutMenu;

    if (proxy->supportEvent(DeviceEvent::WAKEUP))
    {
        shortcutMenu->addAction(wakeupIcon, tr("Wake"), this, [this]() {
            DeviceProxy* proxy = DeviceManager::instance().proxyForType(m_device.type);
            if (proxy)
                proxy->sendEvent(m_device, DeviceEvent::WAKEUP);
        });
    }
    if (proxy->supportEvent(DeviceEvent::SLEEP))
    {
        shortcutMenu->addAction(sleepIcon, tr("Sleep"), this, [this]() {
            DeviceProxy* proxy = DeviceManager::instance().proxyForType(m_device.type);
            if (proxy)
                proxy->sendEvent(m_device, DeviceEvent::SLEEP);
        });
    }
    if (proxy->supportEvent(DeviceEvent::ROTATE))
    {
        shortcutMenu->addAction(rotateIcon, tr("Rotate"), this, [this]() {
            DeviceProxy* proxy = DeviceManager::instance().proxyForType(m_device.type);
            if (proxy)
                proxy->sendEvent(m_device, DeviceEvent::ROTATE);
        });
    }
    if (proxy->supportEvent(DeviceEvent::UNLOCK))
    {
        shortcutMenu->addAction(unlockIcon, tr("Unlock"), this, [this]() {
            DeviceProxy* proxy = DeviceManager::instance().proxyForType(m_device.type);
            if (proxy)
            {
                proxy->sendEvent(m_device, DeviceEvent::WAKEUP);
                proxy->sendEvent(m_device, DeviceEvent::UNLOCK);
            }
        });
    }
    shortcutMenu->addSeparator();
    if (proxy->supportEvent(DeviceEvent::REBOOT))
    {
        shortcutMenu->addAction(rebootIcon, tr("Reboot"), this, [this]() {
            DeviceProxy* proxy = DeviceManager::instance().proxyForType(m_device.type);
            if (proxy)
                proxy->sendEvent(m_device, DeviceEvent::REBOOT);
        });
    }
    if (proxy->supportEvent(DeviceEvent::SHUTDOWN))
    {
        shortcutMenu->addAction(shutdownIcon, tr("Shutdown"), this, [this]() {
            DeviceProxy* proxy = DeviceManager::instance().proxyForType(m_device.type);
            if (proxy)
                proxy->sendEvent(m_device, DeviceEvent::SHUTDOWN);
        });
    }
    shortcutMenu->setStyleSheet(R"(
                QMenu {
                    background-color: #008D4E;
                    border-radius: 6px;
                    padding: 6px 6px;
                    color: white;
                }
                QMenu::item {
                    background-color: transparent;
                    padding: 6px 12px;
                    margin: 2px 4px;
                    font: bold 12px;
                }
                QMenu::item:selected {
                    background-color: #00804E;
                }
                QMenu::separator {
                    height: 1px;
                    background: white;
                    margin: 4px 8px;
                }
    )");
    return shortcutMenu;
}

void ScreenWindow::showContextMenu(const QPoint& globalPos)
{
    QMenu* contextMenu = createContextMenu();
    contextMenu->exec(globalPos);
    contextMenu->deleteLater();
}

QMenu* ScreenWindow::createContextMenu()
{
    fa::QtAwesome* awesome = new fa::QtAwesome(this);
    awesome->initFontAwesome();
    QVariantMap options;
    options.insert("color", QColor(255, 255, 255));

    // Create icons for context menu
    QIcon screenshotIcon = awesome->icon(fa::fa_solid, fa::fa_camera, options).pixmap(16, 16);
    QIcon navIcon = awesome->icon(fa::fa_solid, fa::fa_bars, options).pixmap(16, 16);
    QIcon shortcutIcon = awesome->icon(fa::fa_solid, fa::fa_bolt, options).pixmap(16, 16);

    QMenu* contextMenu = new QMenu(this);
    contextMenu->setObjectName("contextMenu");

    // Screenshot action
    QAction* screenshotAction = contextMenu->addAction(screenshotIcon, tr("Screenshot"));
    connect(screenshotAction, &QAction::triggered, this, &ScreenWindow::takeScreenshot);

    contextMenu->addSeparator();

    // Navigation submenu
    QMenu* navSubmenu = createNavMenu();
    navSubmenu->setTitle(tr("Navigation"));
    navSubmenu->setIcon(navIcon);
    contextMenu->addMenu(navSubmenu);

    // Shortcuts submenu
    QMenu* shortcutSubmenu = createShortcutMenu();
    shortcutSubmenu->setTitle(tr("Shortcuts"));
    shortcutSubmenu->setIcon(shortcutIcon);
    contextMenu->addMenu(shortcutSubmenu);

    // Apply styling
    contextMenu->setStyleSheet(R"(
        QMenu {
            background-color: #008D4E;
            border-radius: 6px;
            padding: 4px 0;
            color: white;
        }
        QMenu::item {
            background-color: transparent;
            padding: 8px 16px;
            margin: 2px 4px;
            font: bold 12px;
        }
        QMenu::item:selected {
            background-color: #00804E;
        }
        QMenu::item:disabled {
            color: #6e7681;
        }
        QMenu::separator {
            height: 1px;
            background: white;
            margin: 4px 8px;
        }
        QMenu::icon {
            padding-left: 6px;
        }
    )");

    return contextMenu;
}

void ScreenWindow::takeScreenshot()
{
    // If screenshot operation is already in progress, ignore new requests
    if (m_screenshotWatcher->isRunning())
    {
        return;
    }

    // Execute screenshot operation in background thread
    QFuture<QByteArray> future = QtConcurrent::run([this]() -> QByteArray {
        DeviceProxy* proxy = DeviceManager::instance().proxyForType(m_device.type);
        if (proxy)
        {
            QByteArray imageData;
            proxy->screenshot(m_device.serial, imageData);
            return imageData;
        }
        return QByteArray();
    });

    m_screenshotWatcher->setFuture(future);
}

QString ScreenWindow::saveScreenshotToFile(const QByteArray& imageData)
{
    if (imageData.isEmpty())
    {
        return QString();
    }

    // Generate filename
    QString fileExt = m_device.type == DeviceType::OHOS ? "jpg" : "png";
    QString fileName =
        QString("Screenshot_%1.%2").arg(QDateTime::currentDateTime().toString("yyyyMMddhhmmss")).arg(fileExt);

    // Get save directory
    QString saveDir = Settings::instance().screenshotDir();
    if (saveDir.isEmpty())
    {
        saveDir = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
    }

    // Save file
    QString filePath = QDir::cleanPath(QDir::fromNativeSeparators(QDir(saveDir).filePath(fileName)));
    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly))
    {
        file.write(imageData);
        file.close();
        return filePath;
    }

    return QString();
}

void ScreenWindow::onScreenshotCompleted()
{
    QByteArray imageData = m_screenshotWatcher->result();
    if (!imageData.isEmpty())
    {
        QString filePath = saveScreenshotToFile(imageData);
        if (!filePath.isEmpty())
        {
            QFileInfo fileInfo(filePath);
            qDebug() << "Screenshot saved to " << fileInfo.absoluteFilePath();
            sendSystemNotification(tr("Screenshot"), fileInfo.absoluteFilePath());
        }
        else
        {
            qWarning() << "Failed to save screenshot";
        }
    }
    else
    {
        qWarning() << "Screenshot failed: no image data received";
    }
}
