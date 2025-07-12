#pragma once
#include "core/Settings.h"
#include "ui/FramelessWindow.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDesktopServices>
#include <QFileDialog>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QLocale>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QPushButton>
#include <QScrollArea>
#include <QStandardPaths>
#include <QThread>
#include <QUrl>
#include <QVBoxLayout>

class UpdateChecker : public QObject
{
    Q_OBJECT
public:
    explicit UpdateChecker(QObject* parent = nullptr);

public slots:
    void checkForUpdates();

signals:
    void updateCheckFinished(bool hasUpdate,
                             const QString& version,
                             const QString& downloadUrl,
                             const qint64& fileSize,
                             const QString& fileMd5,
                             const QString& errorMessage);

private:
    QNetworkAccessManager* networkManager;
};

class SettingsWindow : public FramelessWindow
{
    Q_OBJECT
public:
    explicit SettingsWindow(QWidget* parent = nullptr);

private slots:
    void onUpdateCheckFinished(bool hasUpdate,
                               const QString& version,
                               const QString& downloadUrl,
                               const qint64& fileSize,
                               const QString& fileMd5,
                               const QString& errorMessage);

private:
    // UI creation methods
    void setupMainLayout();
    QGroupBox* createUpdateSettingsGroup();
    QGroupBox* createLogSettingsGroup();
    QGroupBox* createDriverSettingsGroup();
    QGroupBox* createMediaSettingsGroup();
    QGroupBox* createLanguageSettingsGroup();
    QGroupBox* createDonationSettingsGroup();
    QGroupBox* createResetSettingsGroup();
    void applyStylesheet();
    void startUpdateCheck();

    QLabel* currentVersionLabel;
    QLabel* updateStatusLabel;
    QLabel* fileMd5Label;
    QLabel* fileSizeLabel;
    QLabel* updateStatusIcon;

    UpdateChecker* updateChecker;
    QThread* updateThread;
};
