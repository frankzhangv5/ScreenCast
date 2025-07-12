#include "core/Settings.h"
#include "device/Device.h"
#include "ui/MainWindow.h"
#include "ui/SplashScreen.h"
#include "ui/TrayManager.h"
#include "util/Log.h"

#include <QApplication>
#include <QFile>
#include <QLocale>
#include <QPixmap>
#include <QSplashScreen>
#include <QTimer>
#include <QTranslator>

int main(int argc, char* argv[])
{
    int exitCode = 0;
    do
    {
        LogHandler::install();

        qDebug() << "Settings file at:" << Settings::settingsFilePath();

        QApplication app(argc, argv);
        app.setStyle("Fusion");
        app.setQuitOnLastWindowClosed(false);

        QTranslator translator;
        QString langCode = Settings::instance().language();
        QString qmFile = QString(":/i18n/%1").arg(langCode);
        if (!translator.load(qmFile))
        {
            // Use English if loading fails
            (void) translator.load(":/i18n/en_US");
        }
        app.installTranslator(&translator);

        // Create and display splash screen
        SplashScreen splash;
        splash.setAppInfo(QCoreApplication::tr("ScreenCast"), APP_VERSION);
        splash.show();
        qApp->processEvents(); // Ensure display

        // Simulate initialization process
        splash.animateProgressBar(0, 20, 500);
        splash.setStatusMessage(QCoreApplication::tr("Reading configuration file..."));
        QThread::msleep(500);

        splash.animateProgressBar(20, 60, 500);
        splash.setStatusMessage(QCoreApplication::tr("Scanning connected devices..."));
        // Initialize device manager
        DeviceManager::instance().startMonitor();

        splash.animateProgressBar(60, 80, 500);
        splash.setStatusMessage(QCoreApplication::tr("Preparing user interface..."));
        QThread::msleep(500);
        MainWindow w;
        TrayManager::instance()->setMainWindow(&w);

        // Set timer to close splash screen
        QTimer::singleShot(1500, &splash, [&] {
            splash.animateProgressBar(80, 100, 300);
            splash.setStatusMessage(QCoreApplication::tr("Startup complete..."));

            // Complete startup
            splash.finish(&w);
            splash.setProgressValue(100);

            // Temporarily stay on top
            Qt::WindowFlags oldFlags = w.windowFlags();
            w.setWindowFlags(oldFlags | Qt::WindowStaysOnTopHint);
            w.show();
            w.raise();
            w.activateWindow();
            QTimer::singleShot(200, [&w, oldFlags] {
                w.setWindowFlags(oldFlags);
                w.show();
                w.raise();
                w.activateWindow();
                w.setWindowState((w.windowState() & ~Qt::WindowMinimized) | Qt::WindowActive);
            });
        });
        exitCode = app.exec();
    } while (exitCode == 10086); // 10086 is restart flag
    return exitCode;
}
