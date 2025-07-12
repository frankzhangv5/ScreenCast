#include "ui/FramelessWindow.h"

#include "ui/TrayManager.h"

#include <QVBoxLayout>
#include <QWidget>

FramelessWindow::FramelessWindow(QWidget* parent, int width) : QWidget(parent)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::Window);
    setAttribute(Qt::WA_TranslucentBackground, true);

    setFixedWidth(width);
    mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);
    setLayout(mainLayout);

    // Create various control parts
    titleBar = new TitleBar(this);
    page = new QWidget(this); // Blank page
    statusBar = new StatusBar(this);

    mainLayout->addWidget(titleBar);
    mainLayout->addWidget(page, 1);
    mainLayout->addWidget(statusBar);
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
    TrayManager::instance()->showNotification(title, message, timeout);
}
