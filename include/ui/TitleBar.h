#pragma once
#include "AppConfig.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPushButton>
#include <QToolBar>
#include <QWidget>
#include <QtAwesome.h>

class TitleBar : public QWidget
{
public:
    TitleBar(QWidget* parent = nullptr);

    QToolBar* getToolBar();
    void setIconText(const QString& text);

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;

private:
    QLabel* iconLabel;
    QToolBar* toolBar;
    QPushButton* minButton;
    QPushButton* closeButton;
    QPoint dragPosition;
};
