#pragma once
#include "AppConfig.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPainter>
#include <QTimer>
#include <QToolBar>
#include <QWidget>

class StatusBar : public QWidget
{
    Q_OBJECT
public:
    explicit StatusBar(QWidget* parent = nullptr);

    void setStatus(const QString& text);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    void resetToDefaultStatus();

    QHBoxLayout* layout;
    QLabel* statusLabel;
    QTimer* autoResetTimer;
    QString defaultStatus = tr("Ready");
};
