#include "ui/StatusBar.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QTimer>
#include <QToolBar>
#include <QWidget>

StatusBar::StatusBar(QWidget* parent) : QWidget(parent)
{
    setFixedHeight(AppConfig::STATUS_BAR_HEIGHT);
    layout = new QHBoxLayout();
    layout->setContentsMargins(10, 0, 10, 0);

    // Left statusLabel
    statusLabel = new QLabel(defaultStatus, this);
    statusLabel->setStyleSheet("color: white; font-size: 12px;");
    layout->addWidget(statusLabel);

    layout->addStretch(); // Middle elastic

    setLayout(layout);

    // Initialize auto-reset timer
    autoResetTimer = new QTimer(this);
    autoResetTimer->setSingleShot(true);
    connect(autoResetTimer, &QTimer::timeout, this, &StatusBar::resetToDefaultStatus);
}

void StatusBar::setStatus(const QString& text)
{
    statusLabel->setText(text);

    // Stop any existing timer
    autoResetTimer->stop();

    // Start 5-second timer to reset to default status
    autoResetTimer->start(5000);
}

void StatusBar::resetToDefaultStatus()
{
    statusLabel->setText(defaultStatus);
}

void StatusBar::paintEvent(QPaintEvent* event)
{
    QPainter painter(this);
    painter.fillRect(rect(), QColor(0x008D4E));
    QWidget::paintEvent(event);
}
