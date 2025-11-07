#pragma once
#include "App.h"
#include "Context.h"
#include "StatusBar.h"
#include "TitleBar.h"

#include <QVBoxLayout>
#include <QWidget>

class FramelessWindow : public QWidget, public Context
{
    Q_OBJECT
public:
    explicit FramelessWindow(QWidget* parent = nullptr, int width = UI::WINDOW_WIDTH);

    QVBoxLayout* getMainLayout();
    TitleBar* getTitleBar() const;
    QWidget* getPage() const;
    StatusBar* getStatusBar() const;
    void sendSystemNotification(const QString& title, const QString& message, int timeout = 3000);

signals:
    // System notification signal
    void notificationRequested(const QString& title, const QString& message, int timeout);

protected:
    void setupUI();

private:
    QVBoxLayout* mainLayout;
    TitleBar* titleBar;
    QWidget* page;
    StatusBar* statusBar;
};