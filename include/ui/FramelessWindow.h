#pragma once
#include "ui/StatusBar.h"
#include "ui/TitleBar.h"

#include <QVBoxLayout>
#include <QWidget>

class FramelessWindow : public QWidget
{
    Q_OBJECT
public:
    explicit FramelessWindow(QWidget* parent = nullptr, int width = AppConfig::WINDOW_WIDTH);

    QVBoxLayout* getMainLayout();
    TitleBar* getTitleBar() const;
    QWidget* getPage() const;
    StatusBar* getStatusBar() const;
    void sendSystemNotification(const QString& title, const QString& message, int timeout = 3000);

private:
    QVBoxLayout* mainLayout;
    TitleBar* titleBar;
    QWidget* page;
    StatusBar* statusBar;
};
