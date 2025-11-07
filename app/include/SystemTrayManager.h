#ifndef SYSTEMTRAYMANAGER_H
#define SYSTEMTRAYMANAGER_H

#include <QAction>
#include <QCloseEvent>
#include <QMenu>
#include <QObject>
#include <QSystemTrayIcon>
#include <QtAwesome.h>

class SystemTrayManager : public QObject
{
    Q_OBJECT

public:
    explicit SystemTrayManager(QObject* parent = nullptr);
    ~SystemTrayManager();

    // Show notification
    void showNotification(const QString& title, const QString& message, int timeout = 3000);

    // Get tray icon visibility status
    bool isVisible() const;

public slots:
    // Show main window
    void showMainWindow();

    // Quit application
    void quitApplication();

    // Handle tray icon activation
    void onTrayIconActivated(QSystemTrayIcon::ActivationReason reason);

signals:
    // Signal to request showing main window
    void showMainWindowRequested();

    // Signal to request application quit
    void quitRequested();

private:
    // Initialize system tray
    void setupSystemTray();

    // Member variables
    QSystemTrayIcon* m_trayIcon;
    QMenu* m_trayMenu;
    QAction* m_showMainWindowAction;
    QAction* m_quitAction;
    fa::QtAwesome* m_awesome;
};

#endif // SYSTEMTRAYMANAGER_H