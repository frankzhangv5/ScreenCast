#pragma once

#include "TitleBar.h"

#include <QDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSpacerItem>
#include <QVBoxLayout>

class MessageBox : public QDialog
{
    Q_OBJECT

public:
    enum class ButtonType
    {
        Ok,
        YesNo,
        YesNoCancel,
        OkCancel
    };

    enum class IconType
    {
        None,
        Information,
        Warning,
        Critical,
        Question
    };

    explicit MessageBox(QWidget* parent = nullptr);
    ~MessageBox();

    static int information(QWidget* parent, const QString& title, const QString& text);
    static int warning(QWidget* parent, const QString& title, const QString& text);
    static int critical(QWidget* parent, const QString& title, const QString& text);
    static int question(QWidget* parent, const QString& title, const QString& text);
    static int custom(QWidget* parent,
                      const QString& title,
                      const QString& text,
                      ButtonType buttons = ButtonType::Ok,
                      IconType icon = IconType::None);

    void setTitle(const QString& title);
    void setText(const QString& text);
    void setIcon(IconType icon);
    void setStandardButtons(ButtonType buttons);

    int result() const;

private slots:
    void buttonClicked();

private:
    void setupUI();
    void setupButtons(ButtonType buttons);
    void setupIcon(IconType icon);
    QPushButton* createButton(const QString& text, int result);

    TitleBar* m_titleBar;
    QLabel* m_iconLabel;
    QLabel* m_textLabel;
    QWidget* m_buttonWidget;
    QHBoxLayout* m_buttonLayout;
    QVBoxLayout* m_mainLayout;
    int m_result;
};