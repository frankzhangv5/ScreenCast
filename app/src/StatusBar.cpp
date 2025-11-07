#include "StatusBar.h"

#include "App.h"

#include <QHBoxLayout>

StatusBar::StatusBar(QWidget* parent) : QFrame(parent)
{
    setFixedHeight(UI::STATUS_BAR_HEIGHT);
    QHBoxLayout* hLayout = new QHBoxLayout(this);
    hLayout->setContentsMargins(10, 5, 10, 5);

    // Add status icon
    m_statusIconLabel = new QLabel(this);
    m_statusIconLabel->setObjectName("statusIcon");
    hLayout->addWidget(m_statusIconLabel);

    // Add status message label
    m_statusMessageLabel = new QLabel(this);
    hLayout->addWidget(m_statusMessageLabel);

    hLayout->addStretch();

    // Add notification label and icon
    m_notificationLabel = new QLabel(this);
    m_notificationLabel->setObjectName("notification");
    hLayout->addWidget(m_notificationLabel);

    setProperty("class", "StatusBar");

    // Initialize auto-reset timer
    m_autoResetTimer = new QTimer(this);
    m_autoResetTimer->setSingleShot(true);
    connect(m_autoResetTimer, &QTimer::timeout, this, &StatusBar::resetToDefaultStatus);

    // Initialize status information
    setStatusIcon(StatusType::Normal);
    m_statusMessageLabel->setText(tr("Ready"));

    // Save default status
    m_defaultStatusMessage = tr("Ready");
    m_defaultStatusType = StatusType::Normal;
}

void StatusBar::setStatusMessage(const QString& message)
{
    m_statusMessageLabel->setText(message);

    // Stop any existing timer
    m_autoResetTimer->stop();

    // Start 5-second timer to reset to default status
    m_autoResetTimer->start(5000);
}

void StatusBar::setStatusIcon(StatusType type)
{
    switch (type)
    {
        case StatusType::Normal:
            m_statusIconLabel->setText(QString(QChar(0x2714))); // Check mark icon
            break;
        case StatusType::Warning:
            m_statusIconLabel->setText(QString(QChar(0x26A0))); // Warning icon
            break;
        case StatusType::Error:
            m_statusIconLabel->setText(QString(QChar(0x274C))); // Error icon
            break;
    }
}

void StatusBar::setNotification(const QString& notification)
{
    if (notification.isEmpty())
    {
        m_notificationLabel->setText("");
        m_notificationLabel->hide();
    }
    else
    {
        m_notificationLabel->setText(notification);
        m_notificationLabel->show();
    }
}

void StatusBar::resetToDefaultStatus()
{
    m_statusMessageLabel->setText(m_defaultStatusMessage);
    setStatusIcon(m_defaultStatusType);
}