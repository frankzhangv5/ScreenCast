#include "App.h"
#include "MainWindow.h"
#include "SplashWindow.h"
#include "SystemTrayManager.h"
#include "WindowHook.h"
#include "log/Logger.h"
#include "settings/Settings.h"

#include <QApplication>
#include <QDebug>
#include <QFile>
#include <QLocalServer>
#include <QLocalSocket>
#include <QLocale>
#include <QLoggingCategory>
#include <QStringConverter>
#include <QStyleFactory>
#include <QTranslator>
#include <iostream>

// Server name for single instance detection
#define SERVER_NAME APP_NAME

// Load application language settings
void loadApplicationLanguage(QApplication& app)
{
    // Load language settings
    QString lang = Settings::instance().language();
    // Create QTranslator object and ensure its lifecycle matches the application
    static QTranslator translator;

    // Uninstall previous translator if it exists
    app.removeTranslator(&translator);

    if (lang.isEmpty())
    {
        // Use system default language
        lang = QLocale::system().name();
        qDebug() << "Using system default language:" << lang;
    }
    else
    {
        qDebug() << "Loading language:" << lang;
    }

    // First try to load translation file from embedded resources (embedded via embed_translations feature)
    // Resource path format: :/i18n/{lang_code}
    QString resourcePath = QString(":/i18n/%1").arg(lang);

    bool loaded = translator.load(resourcePath);

    if (!loaded)
    {
        // If direct loading fails, try using base language code (e.g., zh_CN -> zh)
        QString baseLang = lang.split("_").first();
        resourcePath = QString(":/i18n/%1").arg(baseLang);
        loaded = translator.load(resourcePath);
    }

    if (loaded)
    {
        app.installTranslator(&translator);
        qDebug() << "Successfully loaded embedded translation file:" << resourcePath;
    }
    else
    {
        qDebug() << "Translation file not found or could not be loaded. Resource:" << resourcePath;
    }

    // Set application locale
    QLocale::setDefault(QLocale(lang.isEmpty() ? QLocale::system().name() : lang));
}

// Set application style
void setupApplicationStyle(QApplication& app)
{
    // Load Qt theme stylesheet
    QFile styleFile(":/theme/Green.qss");
    if (styleFile.open(QFile::ReadOnly))
    {
        QString style = QLatin1String(styleFile.readAll());
        app.setStyleSheet(style);
        styleFile.close();
    }

    // Set application style
    app.setStyle(QStyleFactory::create("Fusion"));
    app.setQuitOnLastWindowClosed(false);

    // Install window hook
    WindowHook::installHook();
}

// Create and set up system tray
SystemTrayManager* createSystemTrayManager(QApplication& app, MainWindow* mainWindow)
{
    // Create system tray manager
    SystemTrayManager* trayManager = new SystemTrayManager(&app);

    // Connect system tray manager signals
    // When main window display is requested
    QObject::connect(trayManager, &SystemTrayManager::showMainWindowRequested, mainWindow, [mainWindow]() {
        mainWindow->showNormal();
        mainWindow->activateWindow();
        mainWindow->raise();
    });

    // When application quit is requested
    QObject::connect(trayManager, &SystemTrayManager::quitRequested, &app, []() {
        // Cleanup operations can be added here
        QApplication::quit();
    });

    // When main window is closed, hide it instead of destroying the entire application
    QObject::connect(mainWindow, &MainWindow::destroyed, [trayManager]() {});

    // Connect main window's notification signal to system tray manager's show notification method
    QObject::connect(
        mainWindow, &FramelessWindow::notificationRequested, trayManager, &SystemTrayManager::showNotification);

    return trayManager;
}

// Check if an instance is already running, activate existing instance if present
bool checkSingleInstance()
{
    qDebug() << "Checking for existing instance..." << SERVER_NAME;
    // Try to connect to existing server
    QLocalSocket socket;
    socket.connectToServer(SERVER_NAME);

    // If connection succeeds, an instance is already running
    if (socket.waitForConnected(100))
    {
        qDebug() << "Another instance is already running. Activating...";
        socket.write("show_window");
        socket.waitForBytesWritten();
        socket.disconnectFromServer();
        return true;
    }

    // Connection failed, it's the first instance, create server
    QLocalServer::removeServer(SERVER_NAME); // Remove possibly existing old server
    QLocalServer* server = new QLocalServer();
    if (!server->listen(SERVER_NAME))
    {
        qWarning() << "Failed to create local server:" << server->errorString();
        delete server;
        return false;
    }

    // Connect new connection signal to handle requests from subsequently started instances
    QObject::connect(server, &QLocalServer::newConnection, [server]() {
        if (!server)
            return;

        QLocalSocket* client = server->nextPendingConnection();
        if (!client)
            return;

        // Read client message
        QObject::connect(client, &QLocalSocket::readyRead, [client]() {
            QString message = QString::fromUtf8(client->readAll());
            if (message == "show_window")
            {
                // Find main window and show it
                for (QWidget* widget : QApplication::allWidgets())
                {
                    if (qobject_cast<MainWindow*>(widget))
                    {
                        widget->showNormal();
                        widget->activateWindow();
                        widget->raise();
                        break;
                    }
                }
            }
            client->deleteLater();
        });
    });

    return false;
}

int main(int argc, char* argv[])
{
    // Check if an instance is already running
    if (checkSingleInstance())
    {
        return 0;
    }

    int exitCode = 0;
    do
    {
        // Create application instance
        QApplication app(argc, argv);

        // Initialize logging system
        Logger::setLogFileName(QApplication::applicationName());
        Logger::install();
        Settings::instance();

        qDebug() << "Starting " << QApplication::applicationName() << " Build ID:" << App::BUILD_ID;

        // 加载应用程序语言设置
        loadApplicationLanguage(app);

        // 设置应用程序样式
        setupApplicationStyle(app);

        // Show splash screen
        SplashWindow splashWindow;
        // Set application information to splash window
        splashWindow.setProgress(90, 250);
        splashWindow.showSplash();

        // Create main window but don't show it yet
        MainWindow* w = new MainWindow();

        // Connect splash finished signal to show main window
        QObject::connect(&splashWindow, &SplashWindow::splashFinished, w, [w]() { w->show(); });

        // 创建和设置系统托盘
        SystemTrayManager* trayManager = createSystemTrayManager(app, w);

        exitCode = app.exec();

        // Cleanup resources to avoid memory leaks
        delete trayManager; // Cleanup system tray manager
        delete w;           // Cleanup main window

    } while (exitCode == 10086); // 10086 is a special exit code for restarting the application

    return exitCode;
}