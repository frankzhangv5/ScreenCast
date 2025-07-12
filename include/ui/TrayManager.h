#pragma once
#include <QMenu>
#include <QObject>
#include <QString>
#include <QSystemTrayIcon>

class TrayManager : public QObject
{
    Q_OBJECT
public:
    static TrayManager* instance();
    void showNotification(const QString& title, const QString& message, int timeout = 3000);
    void setMainWindow(QWidget* window); // Set main window pointer

signals:
    void trayIconLeftClicked();

private slots:
    void onTrayIconActivated(QSystemTrayIcon::ActivationReason reason);
    void onShowMainWindow();
    void onQuit();

private:
    explicit TrayManager(QObject* parent = nullptr);
    void setupTrayMenu();
    QSystemTrayIcon* trayIcon;
    QMenu* trayMenu;
    QWidget* mainWindow = nullptr;
};