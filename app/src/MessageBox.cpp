#include "MessageBox.h"

#include <QApplication>
#include <QGuiApplication>
#include <QIcon>
#include <QPixmap>
#include <QScreen>
#include <QStyle>

MessageBox::MessageBox(QWidget* parent) : QDialog(parent), m_result(0)
{
    // Set borderless window
    setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_StyledBackground);
    setAttribute(Qt::WA_DeleteOnClose);

    // Make the dialog modal
    setWindowModality(Qt::ApplicationModal);

    // Style will be applied from Green.qss

    // Ensure proper visibility and stacking
    setWindowOpacity(1.0);
    setFocusPolicy(Qt::StrongFocus);

    setupUI();
}

MessageBox::~MessageBox() {}

void MessageBox::setupUI()
{
    // Main layout
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(0);

    // Title bar - style will be applied from Green.qss
    m_titleBar = new TitleBar(this, "");
    m_mainLayout->addWidget(m_titleBar);

    // Content area - style will be applied from Green.qss
    QWidget* contentWidget = new QWidget(this);
    contentWidget->setObjectName("contentWidget"); // Set objectName for QSS
    QHBoxLayout* contentLayout = new QHBoxLayout(contentWidget);
    contentLayout->setContentsMargins(20, 20, 20, 20);
    contentLayout->setSpacing(15);

    // Icon label
    m_iconLabel = new QLabel(this);
    m_iconLabel->setFixedSize(48, 48);
    m_iconLabel->setAlignment(Qt::AlignCenter);

    contentLayout->addWidget(m_iconLabel);

    // Text label - style will be applied from Green.qss
    m_textLabel = new QLabel(this);
    m_textLabel->setObjectName("m_textLabel"); // Set objectName for QSS
    m_textLabel->setWordWrap(true);
    m_textLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    contentLayout->addWidget(m_textLabel, 1);

    m_mainLayout->addWidget(contentWidget);

    // Button area - style will be applied from Green.qss
    m_buttonWidget = new QWidget(this);
    m_buttonWidget->setObjectName("m_buttonWidget"); // Set objectName for QSS
    m_buttonLayout = new QHBoxLayout(m_buttonWidget);
    m_buttonLayout->setContentsMargins(20, 10, 20, 20);
    m_buttonLayout->setSpacing(10);

    m_mainLayout->addWidget(m_buttonWidget);

    // Connect signals
    connect(m_titleBar, &TitleBar::closeClicked, this, [this]() {
        m_result = QDialog::Rejected;
        close();
    });

    // Set default size
    setFixedSize(360, 200);
}

void MessageBox::setTitle(const QString& title)
{
    m_titleBar->setTitle(title);
}

void MessageBox::setText(const QString& text)
{
    m_textLabel->setText(text);
}

void MessageBox::setIcon(IconType icon)
{
    setupIcon(icon);
}

void MessageBox::setStandardButtons(ButtonType buttons)
{
    setupButtons(buttons);
}

int MessageBox::result() const
{
    return m_result;
}

void MessageBox::buttonClicked()
{
    QPushButton* button = qobject_cast<QPushButton*>(sender());
    if (button)
    {
        m_result = button->property("result").toInt();
        close();
    }
}

void MessageBox::setupButtons(ButtonType buttons)
{
    // Clear existing buttons
    while (QLayoutItem* item = m_buttonLayout->takeAt(0))
    {
        delete item->widget();
        delete item;
    }

    // Add spring
    m_buttonLayout->addStretch();

    // Add buttons based on button type
    switch (buttons)
    {
        case ButtonType::Ok:
            m_buttonLayout->addWidget(createButton(tr("OK"), QDialog::Accepted));
            break;

        case ButtonType::YesNo:
            m_buttonLayout->addWidget(createButton(tr("No"), QDialog::Rejected));
            m_buttonLayout->addWidget(createButton(tr("Yes"), QDialog::Accepted));
            break;

        case ButtonType::YesNoCancel:
            m_buttonLayout->addWidget(createButton(tr("Cancel"), QDialog::Rejected));
            m_buttonLayout->addWidget(createButton(tr("No"), QDialog::Rejected));
            m_buttonLayout->addWidget(createButton(tr("Yes"), QDialog::Accepted));
            break;

        case ButtonType::OkCancel:
            m_buttonLayout->addWidget(createButton(tr("Cancel"), QDialog::Rejected));
            m_buttonLayout->addWidget(createButton(tr("OK"), QDialog::Accepted));
            break;
    }
}

void MessageBox::setupIcon(IconType icon)
{
    QStyle* style = QApplication::style();
    QIcon iconPixmap;

    switch (icon)
    {
        case IconType::Information:
            iconPixmap = style->standardIcon(QStyle::SP_MessageBoxInformation);
            break;

        case IconType::Warning:
            iconPixmap = style->standardIcon(QStyle::SP_MessageBoxWarning);
            break;

        case IconType::Critical:
            iconPixmap = style->standardIcon(QStyle::SP_MessageBoxCritical);
            break;

        case IconType::Question:
            iconPixmap = style->standardIcon(QStyle::SP_MessageBoxQuestion);
            break;

        case IconType::None:
        default:
            m_iconLabel->clear();
            return;
    }

    m_iconLabel->setPixmap(iconPixmap.pixmap(48, 48));
}

QPushButton* MessageBox::createButton(const QString& text, int result)
{
    QPushButton* button = new QPushButton(text, this);
    button->setProperty("result", result);
    button->setFixedSize(80, 30);
    // Style will be applied from Green.qss
    connect(button, &QPushButton::clicked, this, &MessageBox::buttonClicked);
    return button;
}

int MessageBox::information(QWidget* parent, const QString& title, const QString& text)
{
    return custom(parent, title, text, ButtonType::Ok, IconType::Information);
}

int MessageBox::warning(QWidget* parent, const QString& title, const QString& text)
{
    return custom(parent, title, text, ButtonType::Ok, IconType::Warning);
}

int MessageBox::critical(QWidget* parent, const QString& title, const QString& text)
{
    return custom(parent, title, text, ButtonType::Ok, IconType::Critical);
}

int MessageBox::question(QWidget* parent, const QString& title, const QString& text)
{
    return custom(parent, title, text, ButtonType::YesNo, IconType::Question);
}

int MessageBox::custom(QWidget* parent, const QString& title, const QString& text, ButtonType buttons, IconType icon)
{
    MessageBox* messageBox = new MessageBox(parent);
    messageBox->setTitle(title);
    messageBox->setText(text);
    messageBox->setIcon(icon);
    messageBox->setStandardButtons(buttons);

    // Center display
    if (parent)
    {
        QPoint parentCenter = parent->mapToGlobal(parent->rect().center());
        QRect messageBoxRect = messageBox->geometry();
        messageBoxRect.moveCenter(parentCenter);
        messageBox->setGeometry(messageBoxRect);
    }
    else
    {
        // If no parent window, center on screen
        QScreen* screen = QGuiApplication::primaryScreen();
        if (screen)
        {
            QRect screenGeometry = screen->geometry();
            QRect messageBoxRect = messageBox->geometry();
            messageBoxRect.moveCenter(screenGeometry.center());
            messageBox->setGeometry(messageBoxRect);
        }
    }

    // Show window as modal dialog
    messageBox->exec();

    return messageBox->m_result;
}