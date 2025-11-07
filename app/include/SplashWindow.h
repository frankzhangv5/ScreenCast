#pragma once

#include <QLabel>
#include <QProgressBar>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

class SplashWindow : public QWidget
{
    Q_OBJECT

public:
    explicit SplashWindow();
    ~SplashWindow();

    void showSplash();
    void hideSplash();

    // New public functions
    void setLoadingMessage(const QString& message);
    void setProgress(int value, int interval = 50);

signals:
    void splashFinished();

private:
    QVBoxLayout* m_layout;
    QLabel* m_logoLabel;
    QLabel* m_appNameLabel;
    QLabel* m_versionLabel;
    QLabel* m_loadingLabel;
    QProgressBar* m_progressBar;
    QLabel* m_copyrightLabel;
    QTimer* m_timer;

    void createUI();
};