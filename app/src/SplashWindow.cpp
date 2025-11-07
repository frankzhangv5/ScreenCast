#include "SplashWindow.h"

#include "App.h"

#include <QApplication>
#include <QDebug>
#include <QFile>
#include <QLabel>
#include <QProgressBar>
#include <QScreen>
#include <QVBoxLayout>

SplashWindow::SplashWindow() : QWidget(nullptr, Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint)
{
    setFixedSize(UI::SPLASH_WIDTH, UI::SPLASH_HEIGHT);
    setObjectName("SplashWindow"); // 添加对象名，确保样式选择器能够匹配
    setProperty("class", "SplashWindow");

    // 确保窗口支持样式表
    setAttribute(Qt::WA_StyledBackground, true);

    // Center display
    QScreen* screen = QApplication::primaryScreen();
    QRect screenGeometry = screen->geometry();
    int x = (screenGeometry.width() - UI::SPLASH_WIDTH) / 2;
    int y = (screenGeometry.height() - UI::SPLASH_HEIGHT) / 2;
    move(x, y);

    // 创建UI元素
    createUI();

    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &SplashWindow::hideSplash);
}

SplashWindow::~SplashWindow()
{
    delete m_timer;
    delete m_logoLabel;
    delete m_appNameLabel;
    delete m_versionLabel;
    delete m_loadingLabel;
    delete m_progressBar;
    delete m_copyrightLabel;
    delete m_layout;
}

void SplashWindow::createUI()
{
    m_layout = new QVBoxLayout(this);
    m_layout->setSpacing(10);                     // Set control spacing
    m_layout->setContentsMargins(20, 20, 20, 20); // Set margins

    // App icon
    m_logoLabel = new QLabel(this);
    m_logoLabel->setAlignment(Qt::AlignCenter);
    QPixmap logoPixmap(":/icon/splash/#ffffff.svg");
    m_logoLabel->setPixmap(logoPixmap.scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation));

    // App name
    m_appNameLabel = new QLabel(this);
    m_appNameLabel->setAlignment(Qt::AlignCenter);
    m_appNameLabel->setProperty("class", "SplashWindowAppName");
    m_appNameLabel->setStyleSheet("color: white; font-size: 16px; font-weight: bold;");
    m_appNameLabel->setText(QApplication::applicationName());

    // Application version
    m_versionLabel = new QLabel(tr("Version: ") + QApplication::applicationVersion(), this);
    m_versionLabel->setAlignment(Qt::AlignCenter);
    m_versionLabel->setProperty("class", "SplashWindowVersion");

    // Loading message
    m_loadingLabel = new QLabel(tr("Starting up..."), this);
    m_loadingLabel->setAlignment(Qt::AlignCenter);
    m_loadingLabel->setProperty("class", "SplashWindowLoading");

    // Progress bar
    m_progressBar = new QProgressBar(this);
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(10);
    m_progressBar->setTextVisible(false);

    // 为进度条也设置类属性，确保样式应用
    m_progressBar->setProperty("class", "SplashWindowProgressBar");

    // Copyright info
    m_copyrightLabel = new QLabel(this);
    m_copyrightLabel->setAlignment(Qt::AlignCenter);
    m_copyrightLabel->setProperty("class", "SplashWindowCopyright");
    m_copyrightLabel->setStyleSheet("color: rgba(255, 255, 255, 160); font-size: 10px;");
    m_copyrightLabel->setText(App::COPYRIGHT);

    // Add controls to layout
    m_layout->addWidget(m_logoLabel, 0, Qt::AlignCenter);
    m_layout->addWidget(m_appNameLabel, 0, Qt::AlignCenter);
    m_layout->addWidget(m_versionLabel, 0, Qt::AlignCenter);
    m_layout->addWidget(m_loadingLabel, 0, Qt::AlignCenter);
    m_layout->addWidget(m_progressBar);
    m_layout->addWidget(m_copyrightLabel, 0, Qt::AlignCenter);

    setLayout(m_layout);
}

void SplashWindow::showSplash()
{
    show();
    raise();
    activateWindow();

    // 设置默认持续时间为3000毫秒
    const int defaultDuration = 3000;

    // 启动计时器，在指定时间后隐藏启动画面
    m_timer->start(defaultDuration);
}

void SplashWindow::hideSplash()
{
    m_timer->stop();

    // 在隐藏启动画面前将进度条设置到100%
    if (m_progressBar)
    {
        m_progressBar->setValue(100);
    }

    emit splashFinished();
    hide();
}

void SplashWindow::setLoadingMessage(const QString& message)
{
    if (m_loadingLabel)
    {
        m_loadingLabel->setText(message);
    }
}

void SplashWindow::setProgress(int value, int interval)
{
    if (!m_progressBar)
    {
        return;
    }

    // 获取当前进度值
    int currentValue = m_progressBar->value();

    // 如果目标值与当前值相同，无需更新
    if (value == currentValue)
    {
        return;
    }

    // 限制值在有效范围内
    if (value < 0)
        value = 0;
    if (value > 100)
        value = 100;

    // 计算进度增量
    int progressDiff = value - currentValue;

    // 创建临时计时器来实现平滑进度更新
    QTimer* progressTimer = new QTimer(this);

    connect(progressTimer,
            &QTimer::timeout,
            this,
            [this, progressTimer, currentValue, value, progressDiff, interval]() mutable {
                // 更新当前进度值
                static int stepCount = 0;
                stepCount++;

                // 计算步进增量，确保能在约1秒内完成动画
                int totalSteps = qMin(20, qAbs(progressDiff)); // 最多20步
                if (totalSteps == 0)
                    totalSteps = 1;

                int stepIncrement = progressDiff / totalSteps;

                // 如果是最后一步，直接设置到目标值
                if (stepCount >= totalSteps)
                {
                    m_progressBar->setValue(value);
                    progressTimer->stop();
                    progressTimer->deleteLater();
                    stepCount = 0;
                }
                else
                {
                    // 否则增加一个步进值
                    int newValue = currentValue + stepIncrement * stepCount;

                    // 确保不超过目标值
                    if ((progressDiff > 0 && newValue > value) || (progressDiff < 0 && newValue < value))
                    {
                        newValue = value;
                        progressTimer->stop();
                        progressTimer->deleteLater();
                        stepCount = 0;
                    }

                    m_progressBar->setValue(newValue);
                }
            });

    // 启动进度更新计时器
    progressTimer->start(interval);
}