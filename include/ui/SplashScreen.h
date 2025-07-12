// SplashScreen.h
#pragma once

#include "AppConfig.h"

#include <QApplication>
#include <QFont>
#include <QLabel>
#include <QLinearGradient>
#include <QPainter>
#include <QProgressBar>
#include <QPropertyAnimation>
#include <QScreen>
#include <QSplashScreen>
#include <QTimer>

class SplashScreen : public QSplashScreen
{
    Q_OBJECT
public:
    explicit SplashScreen(const QPixmap& pixmap = QPixmap());
    ~SplashScreen();

    void setProgressValue(int value);
    void setStatusMessage(const QString& message);
    void setAppInfo(const QString& name, const QString& version);
    void animateProgressBar(int startValue, int endValue, int duration);

protected:
    void createUI();
    void drawContents(QPainter* painter) override;

private:
    QProgressBar* progressBar;
    QLabel* statusLabel;
    QLabel* appNameLabel;
    QLabel* appVersionLabel;
    QLabel* copyrightLabel;
    QPropertyAnimation* progressAnimation;
};
