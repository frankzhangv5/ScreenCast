#pragma once

#include <QFrame>
#include <QLabel>
#include <QTimer>

enum class StatusType
{
    Normal,
    Warning,
    Error
};

class StatusBar : public QFrame
{
    Q_OBJECT
public:
    explicit StatusBar(QWidget* parent = nullptr);

    void setStatusMessage(const QString& message);
    void setStatusIcon(StatusType type);
    void setNotification(const QString& notification);

private slots:
    void resetToDefaultStatus();

private:
    QLabel* m_statusMessageLabel;
    QLabel* m_statusIconLabel;
    QLabel* m_notificationLabel;
    QTimer* m_autoResetTimer;
    QString m_defaultStatusMessage;
    StatusType m_defaultStatusType;
};