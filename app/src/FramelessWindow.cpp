#include "FramelessWindow.h"

#include <QTimer>

FramelessWindow::FramelessWindow(QWidget* parent, int width)
    : QWidget(parent), mainLayout(nullptr), titleBar(nullptr), page(nullptr), statusBar(nullptr)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowSystemMenuHint | Qt::WindowMinimizeButtonHint);
    setMinimumWidth(width);

    setupUI();
}

void FramelessWindow::setupUI()
{
    // 创建主布局
    mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // 创建标题栏
    titleBar = new TitleBar(this);
    mainLayout->addWidget(titleBar);

    // 创建页面容器
    page = new QWidget(this);
    page->setObjectName("pageContainer");
    mainLayout->addWidget(page, 1); // 1表示拉伸因子

    // 创建状态栏
    statusBar = new StatusBar(this);
    mainLayout->addWidget(statusBar);

    // 连接信号
    connect(titleBar, &TitleBar::closeClicked, this, &QWidget::close);
    connect(titleBar, &TitleBar::minimizeClicked, this, &QWidget::showMinimized);
}

QVBoxLayout* FramelessWindow::getMainLayout()
{
    return mainLayout;
}

TitleBar* FramelessWindow::getTitleBar() const
{
    return titleBar;
}

QWidget* FramelessWindow::getPage() const
{
    return page;
}

StatusBar* FramelessWindow::getStatusBar() const
{
    return statusBar;
}

void FramelessWindow::sendSystemNotification(const QString& title, const QString& message, int timeout)
{
    // 发射系统通知信号
    emit notificationRequested(title, message, timeout);
}