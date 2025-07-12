#include "ui/TitleBar.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPushButton>
#include <QToolBar>
#include <QWidget>
#include <QtAwesome.h>

TitleBar::TitleBar(QWidget* parent) : QWidget(parent)
{
    setFixedHeight(AppConfig::TITLE_BAR_HEIGHT);
    setStyleSheet("background-color: #008D4E;");
    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(10, 0, 10, 0);

    // Left icon
    iconLabel = new QLabel(this);
    iconLabel->setPixmap(QIcon(":/icons/logo").pixmap(24, 24));
    layout->addWidget(iconLabel);

    // Middle title
    toolBar = new QToolBar(this);
    toolBar->setStyleSheet(R"(
        QToolBar::separator {
            background: white;
            width: 1px;
            margin: 2px 2px;
        }
    )");
    layout->addWidget(toolBar, 1); // Take up remaining space

    // Window buttons
    fa::QtAwesome* awesome = new fa::QtAwesome(this);
    awesome->initFontAwesome();
    QVariantMap options;
    options.insert("color", QColor(255, 255, 255));
    QIcon minIcon = awesome->icon(fa::fa_solid, fa::fa_window_minimize, options).pixmap(20, 20);
    QIcon closeIcon = awesome->icon(fa::fa_solid, fa::fa_xmark, options).pixmap(20, 20);
    minButton = new QPushButton(minIcon, "", this);
    minButton->setFixedSize(30, 30);
    closeButton = new QPushButton(closeIcon, "", this);
    closeButton->setFixedSize(30, 30);

    minButton->setStyleSheet(R"(
        QPushButton {
            background-color: transparent;
            border: none;
            font-size: 16px;
            color: white;
        }
        QPushButton:hover {
            background-color: rgba(255, 255, 255, 0.2);
        }
        QPushButton:pressed {
            background-color: rgba(255, 255, 255, 0.3);
        }
    )");
    closeButton->setStyleSheet(R"(
        QPushButton {
            background-color: transparent;
            border: none;
            font-size: 16px;
            color: white;
        }
        QPushButton:hover {
            background-color: rgba(255, 255, 255, 0.2);
        }
        QPushButton:pressed {
            background-color: rgba(255, 255, 255, 0.3);
        }
    )");

    layout->addWidget(minButton);
    layout->addWidget(closeButton);

    // Connect button signals
    connect(minButton, &QPushButton::clicked, [this]() {
        if (parentWidget())
            parentWidget()->showMinimized();
    });
    connect(closeButton, &QPushButton::clicked, [this]() {
        if (parentWidget())
            parentWidget()->close();
    });
}

QToolBar* TitleBar::getToolBar()
{
    return toolBar;
}

void TitleBar::setIconText(const QString& text)
{
    iconLabel->setPixmap(QPixmap());
    iconLabel->setText(text);
    iconLabel->setStyleSheet("color: white; font-size: 14px;");
}

void TitleBar::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton)
    {
        dragPosition = event->globalPosition().toPoint() - parentWidget()->frameGeometry().topLeft();
        event->accept();
    }
}

void TitleBar::mouseMoveEvent(QMouseEvent* event)
{
    if (event->buttons() & Qt::LeftButton && !dragPosition.isNull())
    {
        parentWidget()->move(event->globalPosition().toPoint() - dragPosition);
        event->accept();
    }
}
