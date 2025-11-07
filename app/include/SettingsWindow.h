#pragma once

#include "FramelessWindow.h"

#include <QCheckBox>
#include <QComboBox>
#include <QGroupBox>
#include <QJsonDocument>
#include <QLabel>
#include <QLineEdit>
#include <QMap>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QPushButton>

class SettingsWindow : public FramelessWindow
{
    Q_OBJECT

public:
    explicit SettingsWindow(QWidget* parent = nullptr);
    ~SettingsWindow();

private slots:
    void setupUI();
    void loadSettings();
    void saveSettings();
    void resetSettings();
    void donate();
    void selectLogDirectory();
    void selectScreenshotDirectory();
    void onUpdateChecked(QNetworkReply* reply);
    void checkForUpdates();

protected:
    // Override close event to notify main window to update plugin log settings when window closes
    void closeEvent(QCloseEvent* event) override;

private:
    // Setting control widgets
    QMap<QString, QCheckBox*> m_pluginCheckBoxes; // Use plugin name as key
    QPushButton* m_logDirButton;
    QCheckBox* m_logCheckBox;
    QLineEdit* m_logDirEdit;
    QPushButton* m_screenshotDirButton;
    QLineEdit* m_screenshotDirEdit;
    QCheckBox* m_updateCheckBox;
    QComboBox* m_languageCombo;
    QLabel* m_updateInfoLabel;
    QNetworkAccessManager* m_networkManager;

    // Static language mapping table to avoid repeated creation
    static const QMap<QString, QString> m_langMap;

    // File log handler

    // Dynamically create plugin checkboxes from PluginManager
    void createPluginCheckboxes(QVBoxLayout* pluginLayout, QWidget* parent);

    // Simplified method to rebuild plugin checkboxes
    void rebuildPluginCheckboxes();

    // Separate setting saving methods
    void saveLogSettings();
    void saveScreenshotSettings();
    void saveUpdateSettings();
    void saveLanguageSettings();
    void savePluginSettings();
    void showSaveNotification(const QString& message);

    // Create language settings group
    QGroupBox* createLanguageSettingsGroup(QWidget* parent);

    // Create donation settings group
    QGroupBox* createDonationSettingsGroup();
};