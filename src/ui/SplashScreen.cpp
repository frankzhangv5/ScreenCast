#include "ui/SplashScreen.h"

#include <QApplication>
#include <QFont>
#include <QLabel>
#include <QLinearGradient>
#include <QPainter>
#include <QProgressBar>
#include <QPropertyAnimation>
#include <QScreen>
#include <QSplashScreen>
#include <QTimer>

SplashScreen::SplashScreen(const QPixmap& pixmap)
    : QSplashScreen(pixmap, Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint)
{
    // Set fixed size
    setFixedSize(AppConfig::SPLASH_WIDTH, AppConfig::SPLASH_HEIGHT);
    // Set background transparent effect
    setAttribute(Qt::WA_TranslucentBackground);

    QScreen* primaryScreen = QGuiApplication::primaryScreen();
    QRect screenGeometry = primaryScreen->availableGeometry();

    // Calculate center position
    int x = (screenGeometry.width() - AppConfig::SPLASH_WIDTH) / 2;
    int y = (screenGeometry.height() - AppConfig::SPLASH_HEIGHT) / 2;

    // Move window to screen center
    move(x, y);

    createUI();
}

SplashScreen::~SplashScreen()
{
    delete progressBar;
    delete statusLabel;
    delete appNameLabel;
    delete appVersionLabel;
    delete copyrightLabel;
    delete progressAnimation;
}

void SplashScreen::setProgressValue(int value)
{
    if (value < 0)
        value = 0;
    if (value > 100)
        value = 100;
    progressBar->setValue(value);
    qApp->processEvents(); // Ensure UI updates
}

void SplashScreen::setStatusMessage(const QString& message)
{
    statusLabel->setText(message);
    qApp->processEvents(); // Ensure UI updates
}

void SplashScreen::setAppInfo(const QString& name, const QString& version)
{
    appNameLabel->setText(name);
    appVersionLabel->setText(tr("Version: %1").arg(version));
    repaint();
}

void SplashScreen::animateProgressBar(int startValue, int endValue, int duration)
{
    progressAnimation->stop();
    progressAnimation->setStartValue(startValue);
    progressAnimation->setEndValue(endValue);
    progressAnimation->setDuration(duration);
    progressAnimation->start();
}

void SplashScreen::createUI()
{
    // Layout constant definitions (golden ratio 500x309)
    const int topPadding = 32; // Top margin 28px

    // 1. App icon (after top margin)
    QLabel* iconLabel = new QLabel(this);
    iconLabel->setAlignment(Qt::AlignCenter);
    int iconSize = 60;
    iconLabel->setGeometry((width() - iconSize) / 2, topPadding, iconSize, iconSize);
    iconLabel->setPixmap(
        QPixmap(":/icons/splash").scaled(iconSize, iconSize, Qt::KeepAspectRatio, Qt::SmoothTransformation));

    // 2. App name (below icon)
    appNameLabel = new QLabel(this);
    appNameLabel->setAlignment(Qt::AlignCenter);
    appNameLabel->setGeometry(0, topPadding + iconSize + 16, width(), 28); // Height 28px
    appNameLabel->setStyleSheet("color: white; font-size: 16px; font-weight: bold;");

    // 3. Version info (below name)
    appVersionLabel = new QLabel(this);
    appVersionLabel->setAlignment(Qt::AlignCenter);
    appVersionLabel->setGeometry(0, topPadding + iconSize + 16 + 28 + 4, width(), 20); // Height 20px
    appVersionLabel->setStyleSheet("color: rgba(255, 255, 255, 200); font-size: 11px;");

    // 4. Status label (middle)
    statusLabel = new QLabel(this);
    statusLabel->setAlignment(Qt::AlignCenter);
    statusLabel->setGeometry(width() * 0.1, 215, width() * 0.8, 22); // Height 22px
    statusLabel->setStyleSheet("color: white; font-size: 11px;");

    // 5. Progress bar (40px from bottom)
    progressBar = new QProgressBar(this);
    progressBar->setGeometry(width() * 0.1, 240, width() * 0.8, 5); // Height 5px
    progressBar->setRange(0, 100);
    progressBar->setTextVisible(false);
    progressBar->setStyleSheet(R"(
            QProgressBar {
                border: none;
                border-radius: 2px;
                background: rgba(30, 30, 30, 80);
            }
            QProgressBar::chunk {
                background: qlineargradient(spread:pad, x1:0, y1:0, x2:1, y2:0,
                    stop:0 rgba(255, 255, 255, 240),
                    stop:1 rgba(255, 255, 255, 240));
                border-radius: 2px;
            }
    )");

    // 6. Copyright info (18px from bottom)
    copyrightLabel = new QLabel(this);
    copyrightLabel->setAlignment(Qt::AlignCenter);
    copyrightLabel->setGeometry(0, 270, width(), 16); // Height 16px
    copyrightLabel->setStyleSheet("color: rgba(255, 255, 255, 160); font-size: 10px;");
    copyrightLabel->setText(tr("© 2025 ZhangFeng. All Rights Reserved."));

    // 7. Animation system
    progressAnimation = new QPropertyAnimation(progressBar, "value", this);
    progressAnimation->setEasingCurve(QEasingCurve::InOutQuad);
}

void SplashScreen::drawContents(QPainter* painter)
{
    QSplashScreen::drawContents(painter);

    // Draw background color - #008D4E
    painter->fillRect(rect(), QColor(0, 0x8D, 0x4E));
    // Add visual effects - gradient mask
    QLinearGradient gradient(0, 0, width(), height());
    gradient.setColorAt(0, QColor(0, 0x8D, 0x4E, 220));
    gradient.setColorAt(0.5, QColor(0, 0x9E, 0x60, 200));
    gradient.setColorAt(1, QColor(0, 0x7D, 0x40, 240));
    painter->fillRect(rect(), gradient);
}
