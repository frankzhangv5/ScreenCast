#include "TitleBar.h"

#include "App.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QStyle>

TitleBar::TitleBar(QWidget* parent, const QString& title) : QWidget(parent), m_parent(parent)
{
    setupUI();
    setTitle(title);
}

void TitleBar::setupUI()
{
    // Set title bar size
    setFixedHeight(UI::TITLE_BAR_HEIGHT);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    // Set title bar style
    setProperty("class", "TitleBar");

    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setAlignment(Qt::AlignCenter);
    layout->setContentsMargins(10, 0, 10, 0);
    layout->setSpacing(0);
    setAttribute(Qt::WA_StyledBackground);
    setAutoFillBackground(true);

    // Title label
    m_titleLabel = new QLabel(this);
    layout->addWidget(m_titleLabel);
    layout->setSpacing(5);

    // Create toolbar, hidden by default
    m_toolBar = new QToolBar(this);
    m_toolBar->setVisible(false); // Hidden by default
    m_toolBar->setFloatable(false);
    layout->addWidget(m_toolBar, 1); // Toolbar takes remaining space
    layout->addStretch();

    // Minimize and close buttons
    std::map<QString, QLabel**> buttonMap = {{"─", &m_minimizeButton}, {"X", &m_closeButton}};

    // Create buttons in a loop
    for (const auto& pair : buttonMap)
    {
        auto button = new QLabel(pair.first, this);
        button->setMouseTracking(true);
        button->setFixedSize(20, 20);
        button->setAlignment(Qt::AlignCenter);
        button->installEventFilter(this);
        button->setProperty("class", "TitleBarButton");
        // Style will be applied from Green.qss
        *pair.second = button;
    }
    layout->addWidget(m_minimizeButton);

    layout->addWidget(m_closeButton);

    setLayout(layout);
}

void TitleBar::setTitle(const QString& title)
{
    m_titleLabel->setText(title);
}

void TitleBar::setTitle(const QPixmap& pixmap)
{
    m_titleLabel->setPixmap(pixmap);
}

QString TitleBar::title() const
{
    return m_titleLabel->text();
}

QToolBar* TitleBar::getToolBar() const
{
    return m_toolBar;
}

void TitleBar::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && m_parent)
    {
        m_dragPosition = event->globalPosition().toPoint() - m_parent->frameGeometry().topLeft();
        event->accept();
    }
}

void TitleBar::mouseMoveEvent(QMouseEvent* event)
{
    if (event->buttons() & Qt::LeftButton && m_parent)
    {
        m_parent->move(event->globalPosition().toPoint() - m_dragPosition);
        event->accept();
    }
}

bool TitleBar::eventFilter(QObject* obj, QEvent* event)
{
    if (event->type() == QEvent::MouseButtonPress)
    {
        QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent->button() == Qt::LeftButton)
        {
            if (obj == m_minimizeButton)
            {
                emit minimizeClicked();
                return true;
            }
            else if (obj == m_closeButton)
            {
                emit closeClicked();
                return true;
            }
        }
    }
    return QWidget::eventFilter(obj, event);
}