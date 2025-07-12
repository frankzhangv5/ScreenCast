#include "ui/SettingsWindow.h"

#include "device/DeviceManager.h"

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
#include <QPixmap>
#include <QPushButton>
#include <QScrollArea>
#include <QStackedWidget>
#include <QStandardPaths>
#include <QThread>
#include <QUrl>
#include <QVBoxLayout>

// UpdateChecker implementation
UpdateChecker::UpdateChecker(QObject* parent) : QObject(parent)
{
    networkManager = new QNetworkAccessManager(this);
}

void UpdateChecker::checkForUpdates()
{
    QUrl url(AppConfig::UPDATE_URL);
    QNetworkReply* reply = networkManager->get(QNetworkRequest(url));

    connect(reply, &QNetworkReply::finished, [this, reply]() {
        if (reply->error() == QNetworkReply::NoError)
        {
            QByteArray data = reply->readAll();
            QJsonDocument doc = QJsonDocument::fromJson(data);
            if (doc.isNull())
            {
                qDebug() << "Error parsing update JSON:" << data;
                reply->deleteLater();
                return;
            }

            QJsonObject jsonObj = doc.object();
            QString platform = QString(APP_PLATFORM).toLower();
            QJsonObject obj = jsonObj[platform].toObject();
            if (obj.isEmpty())
            {
                qDebug() << "No update information for platform:" << platform;
                reply->deleteLater();
                return;
            }

            QString latestVersion = obj["version"].toString();
            QString downloadUrl = obj["download_url"].toString();
            qint64 fileSize = obj["file_size"].toInteger();
            QString fileMd5 = obj["file_md5"].toString();

            qDebug() << "Update information for platform:" << platform;
            qDebug() << "Latest version:" << latestVersion;
            qDebug() << "Download URL:" << downloadUrl;
            qDebug() << "File size:" << fileSize;
            qDebug() << "File MD5:" << fileMd5;

            // Check if new version is available
            QStringList currentParts = QString(APP_VERSION).split('.');
            QStringList latestParts = latestVersion.split('.');
            bool hasUpdate = false;

            int maxLength = qMax(currentParts.size(), latestParts.size());
            for (int i = 0; i < maxLength; ++i)
            {
                int current = (i < currentParts.size()) ? currentParts[i].toInt() : 0;
                int latest = (i < latestParts.size()) ? latestParts[i].toInt() : 0;

                if (latest > current)
                {
                    hasUpdate = true;
                    break;
                }
                if (latest < current)
                {
                    break;
                }
            }

            emit updateCheckFinished(hasUpdate, latestVersion, downloadUrl, fileSize, fileMd5, QString());
        }
        else
        {
            emit updateCheckFinished(false, QString(), QString(), 0, QString(), reply->errorString());
        }

        reply->deleteLater();
    });
}

SettingsWindow::SettingsWindow(QWidget* parent) : FramelessWindow(parent)
{
    setupMainLayout();
    applyStylesheet();

    // Initialize update checker in separate thread
    updateChecker = new UpdateChecker();
    updateThread = new QThread();
    updateChecker->moveToThread(updateThread);

    // Connect signals and slots
    connect(updateThread, &QThread::started, updateChecker, &UpdateChecker::checkForUpdates);
    connect(updateChecker, &UpdateChecker::updateCheckFinished, this, &SettingsWindow::onUpdateCheckFinished);
    connect(updateThread, &QThread::finished, updateChecker, &QObject::deleteLater);
    connect(updateThread, &QThread::finished, updateThread, &QObject::deleteLater);

    // Start update check
    startUpdateCheck();
}

void SettingsWindow::startUpdateCheck()
{
    updateThread->start();
}

void SettingsWindow::onUpdateCheckFinished(bool hasUpdate,
                                           const QString& version,
                                           const QString& downloadUrl,
                                           const qint64& fileSize,
                                           const QString& fileMd5,
                                           const QString& errorMessage)
{
    if (!errorMessage.isEmpty())
    {
        updateStatusIcon->setText("❌");
        updateStatusLabel->setText(tr("Update check failed"));
        updateStatusLabel->setStyleSheet("color: #f44336;");
        return;
    }

    if (hasUpdate)
    {
        updateStatusIcon->setText("🆕");
        updateStatusLabel->setText(tr("New version: <a href='%1'>%2</a>").arg(downloadUrl).arg(version));
        updateStatusLabel->setStyleSheet("color: #4CAF50; font-weight: bold;");
        updateStatusLabel->setOpenExternalLinks(true);

        fileSizeLabel->setText(tr("📦 Size: %1 MB").arg(QString::number(fileSize / (1024.0 * 1024.0), 'f', 2)));
        fileSizeLabel->setVisible(true);

        fileMd5Label->setText(tr("🔒 MD5: %1").arg(fileMd5.left(16)));
        fileMd5Label->setVisible(true);
    }
    else
    {
        updateStatusIcon->setText("✅");
        updateStatusLabel->setText(tr("Already up to date"));
        updateStatusLabel->setStyleSheet("color: #666;");
    }
}

void SettingsWindow::setupMainLayout()
{
    QWidget* central = new QWidget(this);
    QVBoxLayout* mainLayout = new QVBoxLayout(central);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // Scroll area
    QScrollArea* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    QWidget* scrollWidget = new QWidget(scrollArea);
    QVBoxLayout* scrollLayout = new QVBoxLayout(scrollWidget);
    scrollLayout->setContentsMargins(16, 16, 16, 16);
    scrollLayout->setSpacing(16);

    // App settings title
    QLabel* title = new QLabel(tr("App Settings"), scrollWidget);
    title->setStyleSheet("font-size: 20px; font-weight: bold; color: #222;");
    scrollLayout->addWidget(title);

    // Add settings groups
    scrollLayout->addWidget(createUpdateSettingsGroup());
    scrollLayout->addWidget(createLogSettingsGroup());
    scrollLayout->addWidget(createDriverSettingsGroup());
    scrollLayout->addWidget(createMediaSettingsGroup());
    scrollLayout->addWidget(createLanguageSettingsGroup());
    scrollLayout->addWidget(createDonationSettingsGroup());
    scrollLayout->addWidget(createResetSettingsGroup());

    scrollLayout->addStretch();
    scrollWidget->setLayout(scrollLayout);
    scrollArea->setWidget(scrollWidget);

    mainLayout->addWidget(scrollArea);
    central->setLayout(mainLayout);
    central->setFixedHeight(AppConfig::PAGE_HEIGHT);

    // Correctly set content to FramelessWindow's page area
    getPage()->setLayout(new QVBoxLayout());
    getPage()->layout()->setContentsMargins(0, 0, 0, 0);
    getPage()->layout()->addWidget(central);
}

QGroupBox* SettingsWindow::createUpdateSettingsGroup()
{
    QGroupBox* updateGroup = new QGroupBox(tr("Software Update"));
    QVBoxLayout* updateLayout = new QVBoxLayout(updateGroup);
    updateLayout->setSpacing(8);

    // Current version section
    QHBoxLayout* versionLayout = new QHBoxLayout();
    QLabel* versionIcon = new QLabel("📦", updateGroup);
    versionIcon->setStyleSheet("font-size: 16px;");
    versionIcon->setFixedWidth(20);

    currentVersionLabel = new QLabel(tr("Current Version: %1").arg(APP_VERSION), updateGroup);
    currentVersionLabel->setStyleSheet("font-weight: bold; color: #333;");

    versionLayout->addWidget(versionIcon);
    versionLayout->addWidget(currentVersionLabel);
    versionLayout->addStretch();
    updateLayout->addLayout(versionLayout);

    // Update status section
    QHBoxLayout* statusLayout = new QHBoxLayout();
    updateStatusIcon = new QLabel("🔄", updateGroup);
    updateStatusIcon->setStyleSheet("font-size: 16px;");
    updateStatusIcon->setFixedWidth(20);

    updateStatusLabel = new QLabel(tr("Checking for updates..."), updateGroup);
    updateStatusLabel->setStyleSheet("color: #666;");

    statusLayout->addWidget(updateStatusIcon);
    statusLayout->addWidget(updateStatusLabel);
    statusLayout->addStretch();
    updateLayout->addLayout(statusLayout);

    // File info section (initially hidden)
    fileSizeLabel = new QLabel(updateGroup);
    fileSizeLabel->setVisible(false);
    fileSizeLabel->setStyleSheet("color: #666; font-size: 11px; padding-left: 20px;");

    fileMd5Label = new QLabel(updateGroup);
    fileMd5Label->setVisible(false);
    fileMd5Label->setStyleSheet("color: #666; font-size: 11px; padding-left: 20px;");

    updateLayout->addWidget(fileSizeLabel);
    updateLayout->addWidget(fileMd5Label);

    return updateGroup;
}

QGroupBox* SettingsWindow::createLogSettingsGroup()
{
    QGroupBox* logGroup = new QGroupBox(tr("Log Settings"));
    QVBoxLayout* logLayout = new QVBoxLayout(logGroup);
    logLayout->setSpacing(8);

    // Log to file checkbox
    QHBoxLayout* logCheckLayout = new QHBoxLayout();
    QLabel* logIcon = new QLabel("📝", logGroup);
    logIcon->setStyleSheet("font-size: 16px;");
    logIcon->setFixedWidth(20);

    QCheckBox* logCheck = new QCheckBox(tr("Enable file logging"), logGroup);
    logCheck->setStyleSheet("color: #333;");

    logCheckLayout->addWidget(logIcon);
    logCheckLayout->addWidget(logCheck);
    logCheckLayout->addStretch();
    logLayout->addLayout(logCheckLayout);

    // Log directory section
    QHBoxLayout* logPathLayout = new QHBoxLayout();
    QLabel* folderIcon = new QLabel("📁", logGroup);
    folderIcon->setStyleSheet("font-size: 14px;");
    folderIcon->setFixedWidth(20);

    QLineEdit* logPathEdit = new QLineEdit(logGroup);
    logPathEdit->setPlaceholderText(tr("Select log directory..."));
    logPathEdit->setStyleSheet(R"(
        QLineEdit {
            background-color: #ffffff;
            color: #333333;
            border: 1px solid #d0d0d0;
            padding: 6px 8px;
            font-size: 10px;
            border-radius: 3px;
        }
        QLineEdit:focus {
            border-color: #008D4E;
        }
    )");

    QPushButton* logBrowseBtn = new QPushButton(tr("Browse"), logGroup);
    logBrowseBtn->setFixedWidth(64);
    logBrowseBtn->setStyleSheet(R"(
        QPushButton {
            background-color: #008D4E;
            color: white;
            border: none;
            padding: 6px 12px;
            border-radius: 3px;
            font-size: 11px;
        }
        QPushButton:hover {
            background-color: #00804E;
        }
        QPushButton:pressed {
            background-color: #006d3e;
        }
    )");

    logPathLayout->addWidget(folderIcon);
    logPathLayout->addWidget(logPathEdit, 1);
    logPathLayout->addWidget(logBrowseBtn);
    logLayout->addLayout(logPathLayout);

    // Read settings
    logCheck->setChecked(Settings::instance().logToFile());
    logPathEdit->setText(Settings::instance().logDir());

    // Save settings
    connect(logCheck, &QCheckBox::toggled, this, [](bool checked) { Settings::instance().setLogToFile(checked); });
    connect(logPathEdit, &QLineEdit::editingFinished, this, [logPathEdit] {
        Settings::instance().setLogDir(logPathEdit->text());
    });

    // Directory selection button functionality
    connect(logBrowseBtn, &QPushButton::clicked, this, [logPathEdit, this]() {
        QString dir = QFileDialog::getExistingDirectory(this, tr("Select Log Directory"));
        if (!dir.isEmpty())
            logPathEdit->setText(dir);
    });

    return logGroup;
}

QGroupBox* SettingsWindow::createDriverSettingsGroup()
{
    QGroupBox* proxyGroup = new QGroupBox(tr("Driver Settings"));
    QVBoxLayout* proxyLayout = new QVBoxLayout(proxyGroup);
    proxyLayout->setSpacing(8);

    // Header
    QHBoxLayout* headerLayout = new QHBoxLayout();
    QLabel* headerIcon = new QLabel("🔧", proxyGroup);
    headerIcon->setStyleSheet("font-size: 16px;");
    headerIcon->setFixedWidth(20);

    QLabel* headerLabel = new QLabel(tr("Enable device drivers:"), proxyGroup);
    headerLabel->setStyleSheet("color: #333;");

    headerLayout->addWidget(headerIcon);
    headerLayout->addWidget(headerLabel);
    headerLayout->addStretch();
    proxyLayout->addLayout(headerLayout);

    // ADB driver
    QHBoxLayout* adbLayout = new QHBoxLayout();
    QLabel* adbIcon = new QLabel("📱", proxyGroup);
    adbIcon->setStyleSheet("font-size: 14px;");
    adbIcon->setFixedWidth(20);

    QCheckBox* adbCheck = new QCheckBox(tr("Android (adb)"), proxyGroup);
    adbCheck->setStyleSheet("color: #333;");

    QLabel* adbStatus = new QLabel(tr("(Not installed)"), proxyGroup);
    adbStatus->setStyleSheet("color: #666; font-size: 11px;");

    adbLayout->addWidget(adbIcon);
    adbLayout->addWidget(adbCheck);
    adbLayout->addWidget(adbStatus);
    adbLayout->addStretch();
    proxyLayout->addLayout(adbLayout);

    // HDC driver
    QHBoxLayout* hdcLayout = new QHBoxLayout();
    QLabel* hdcIcon = new QLabel("⚡", proxyGroup);
    hdcIcon->setStyleSheet("font-size: 14px;");
    hdcIcon->setFixedWidth(20);

    QCheckBox* hdcCheck = new QCheckBox(tr("OHOS (hdc)"), proxyGroup);
    hdcCheck->setStyleSheet("color: #333;");

    QLabel* hdcStatus = new QLabel(tr("(Not installed)"), proxyGroup);
    hdcStatus->setStyleSheet("color: #666; font-size: 11px;");

    hdcLayout->addWidget(hdcIcon);
    hdcLayout->addWidget(hdcCheck);
    hdcLayout->addWidget(hdcStatus);
    hdcLayout->addStretch();
    proxyLayout->addLayout(hdcLayout);

    // Check driver installation status
    bool adbInstalled = !QStandardPaths::findExecutable("adb").isEmpty();
    bool hdcInstalled = !QStandardPaths::findExecutable("hdc").isEmpty();

    if (!adbInstalled)
    {
        adbCheck->setEnabled(false);
    }
    if (!hdcInstalled)
    {
        hdcCheck->setEnabled(false);
    }

    adbStatus->setText(adbInstalled ? tr("(Installed)") : tr("(Not installed)"));
    adbStatus->setStyleSheet(adbInstalled ? "color: #4CAF50; font-size: 11px;" : "color: #666; font-size: 11px;");

    hdcStatus->setText(hdcInstalled ? tr("(Installed)") : tr("(Not installed)"));
    hdcStatus->setStyleSheet(hdcInstalled ? "color: #4CAF50; font-size: 11px;" : "color: #666; font-size: 11px;");

    // Driver download link
    QHBoxLayout* linkLayout = new QHBoxLayout();
    QLabel* linkIcon = new QLabel("🌐", proxyGroup);
    linkIcon->setStyleSheet("font-size: 14px;");
    linkIcon->setFixedWidth(20);

    QLabel* driverLink = new QLabel(tr("<a href='#' style='color: #2196F3; text-decoration: none; font-size: "
                                       "11px;'>Download drivers from official website</a>"),
                                    proxyGroup);
    driverLink->setOpenExternalLinks(false);

    linkLayout->addWidget(linkIcon);
    linkLayout->addWidget(driverLink);
    linkLayout->addStretch();
    proxyLayout->addLayout(linkLayout);

    // Read settings
    adbCheck->setChecked(Settings::instance().adbProxy());
    hdcCheck->setChecked(Settings::instance().hdcProxy());

    // Save settings
    connect(adbCheck, &QCheckBox::toggled, this, [](bool checked) { Settings::instance().setAdbProxy(checked); });
    connect(hdcCheck, &QCheckBox::toggled, this, [](bool checked) { Settings::instance().setHdcProxy(checked); });

    // Connect driver installation link signal
    QObject::connect(
        driverLink, &QLabel::linkActivated, [=] { QDesktopServices::openUrl(QUrl(AppConfig::DRIVER_DOWNLOAD_URL)); });

    return proxyGroup;
}

QGroupBox* SettingsWindow::createMediaSettingsGroup()
{
    QGroupBox* mediaGroup = new QGroupBox(tr("Media Settings"));
    QVBoxLayout* mediaLayout = new QVBoxLayout(mediaGroup);
    mediaLayout->setSpacing(8);

    // Header
    QHBoxLayout* headerLayout = new QHBoxLayout();
    QLabel* headerIcon = new QLabel("📸", mediaGroup);
    headerIcon->setStyleSheet("font-size: 16px;");
    headerIcon->setFixedWidth(20);

    QLabel* headerLabel = new QLabel(tr("Screenshot settings:"), mediaGroup);
    headerLabel->setStyleSheet("color: #333;");

    headerLayout->addWidget(headerIcon);
    headerLayout->addWidget(headerLabel);
    headerLayout->addStretch();
    mediaLayout->addLayout(headerLayout);

    // Screenshot save directory
    QHBoxLayout* screenshotLayout = new QHBoxLayout();
    QLabel* folderIcon = new QLabel("📁", mediaGroup);
    folderIcon->setStyleSheet("font-size: 14px;");
    folderIcon->setFixedWidth(20);

    QLineEdit* screenshotEdit = new QLineEdit(mediaGroup);
    screenshotEdit->setPlaceholderText(tr("Select screenshot directory..."));
    screenshotEdit->setStyleSheet(R"(
        QLineEdit {
            background-color: #ffffff;
            color: #333333;
            border: 1px solid #d0d0d0;
            padding: 6px 8px;
            font-size: 10px;
            border-radius: 3px;
        }
        QLineEdit:focus {
            border-color: #008D4E;
        }
    )");

    QPushButton* screenshotBrowseBtn = new QPushButton(tr("Browse"), mediaGroup);
    screenshotBrowseBtn->setFixedWidth(64);
    screenshotBrowseBtn->setStyleSheet(R"(
        QPushButton {
            background-color: #008D4E;
            color: white;
            border: none;
            padding: 6px 12px;
            border-radius: 3px;
            font-size: 11px;
        }
        QPushButton:hover {
            background-color: #00804E;
        }
        QPushButton:pressed {
            background-color: #006d3e;
        }
    )");

    screenshotLayout->addWidget(folderIcon);
    screenshotLayout->addWidget(screenshotEdit, 1);
    screenshotLayout->addWidget(screenshotBrowseBtn);
    mediaLayout->addLayout(screenshotLayout);

    // Read settings
    screenshotEdit->setText(Settings::instance().screenshotDir());

    // Save settings
    connect(screenshotEdit, &QLineEdit::editingFinished, this, [screenshotEdit] {
        Settings::instance().setScreenshotDir(screenshotEdit->text());
    });

    // Directory selection button functionality
    connect(screenshotBrowseBtn, &QPushButton::clicked, this, [screenshotEdit, this]() {
        QString dir = QFileDialog::getExistingDirectory(this, tr("Select Screenshot Directory"));
        if (!dir.isEmpty())
            screenshotEdit->setText(dir);
    });

    return mediaGroup;
}

QGroupBox* SettingsWindow::createLanguageSettingsGroup()
{
    QGroupBox* langGroup = new QGroupBox(tr("Language Settings"));
    QVBoxLayout* langLayout = new QVBoxLayout(langGroup);
    langLayout->setSpacing(8);

    // Header
    QHBoxLayout* headerLayout = new QHBoxLayout();
    QLabel* headerIcon = new QLabel("🌐", langGroup);
    headerIcon->setStyleSheet("font-size: 16px;");
    headerIcon->setFixedWidth(20);

    QLabel* headerLabel = new QLabel(tr("Application language:"), langGroup);
    headerLabel->setStyleSheet("color: #333;");

    headerLayout->addWidget(headerIcon);
    headerLayout->addWidget(headerLabel);
    headerLayout->addStretch();
    langLayout->addLayout(headerLayout);

    // Language selection section
    QHBoxLayout* langSelectionLayout = new QHBoxLayout();
    QLabel* langIcon = new QLabel("📝", langGroup);
    langIcon->setStyleSheet("font-size: 14px;");
    langIcon->setFixedWidth(20);

    QComboBox* langCombo = new QComboBox(langGroup);
    langCombo->setStyleSheet(R"(
        QComboBox {
            background-color: #ffffff;
            color: #333333;
            border: 1px solid #d0d0d0;
            padding: 6px 8px;
            font-size: 11px;
            border-radius: 3px;
            min-width: 200px;
        }
        QComboBox:focus {
            border-color: #008D4E;
        }
        QComboBox::drop-down {
            border: none;
            width: 20px;
        }
    )");

    // Use map to store language codes and display names (key is code, value is display name)
    QMap<QString, QString> langMap = {
        {"zh_CN", "简体中文（中国）"},
        {"zh_TW", "繁體中文（台灣）"},
        {"zh_HK", "繁體中文（香港）"},
        {"en_US", "English(United States)"},
        {"en_GB", "English(United Kingdom)"},
        {"ko_KR", "한국어(대한민국)"},
        {"th_TH", "ไทย(ประเทศไทย)"},
        {"vi_VN", "Tiếng Việt(Việt Nam)"},
        {"hi_IN", "हिन्दी(भारत)"},
        {"fr_FR", "Français(Français)"},
        {"ja_JP", "日本語(日本)"},
        {"ru_RU", "Русский(Россия)"},
        {"de_DE", "Deutsch(Deutschland)"},
    };

    langCombo->addItem(tr("System default"), QLocale::system().name());
    for (auto it = langMap.constBegin(); it != langMap.constEnd(); ++it)
    {
        langCombo->addItem(it.value(), it.key());
    }

    langSelectionLayout->addWidget(langIcon);
    langSelectionLayout->addWidget(langCombo);
    langSelectionLayout->addStretch();
    langLayout->addLayout(langSelectionLayout);

    // Apply button section
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();

    QPushButton* applyBtn = new QPushButton(tr("Apply"), langGroup);
    applyBtn->setFixedWidth(80);
    applyBtn->setEnabled(false); // Disabled by default
    applyBtn->setStyleSheet(R"(
        QPushButton {
            background-color: #008D4E;
            color: white;
            border: none;
            padding: 6px 12px;
            border-radius: 3px;
            font-size: 11px;
        }
        QPushButton:hover {
            background-color: #00804E;
        }
        QPushButton:pressed {
            background-color: #006d3e;
        }
        QPushButton:disabled {
            background-color: #cccccc;
            color: #666666;
        }
    )");

    buttonLayout->addWidget(applyBtn);
    langLayout->addLayout(buttonLayout);

    // Read settings
    int langIdx = langCombo->findData(Settings::instance().language());
    if (langIdx >= 0)
        langCombo->setCurrentIndex(langIdx);

    // Language settings: save only when "Apply" button is clicked
    connect(langCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [applyBtn](int) {
        applyBtn->setEnabled(true);
    });
    connect(applyBtn, &QPushButton::clicked, this, [langCombo, applyBtn, this] {
        Settings::instance().setLanguage(langCombo->currentData().toString());
        QMessageBox::information(
            this, tr("Language change"), tr("Requires restarting the application to take effect."));
        qApp->exit(10086);
        applyBtn->setEnabled(false); // Disable again after applying
    });

    return langGroup;
}

QGroupBox* SettingsWindow::createDonationSettingsGroup()
{
    QGroupBox* donationGroup = new QGroupBox(tr("Support Development"));
    donationGroup->setMaximumWidth(300); // Allow wider group for QR codes
    QVBoxLayout* donationLayout = new QVBoxLayout(donationGroup);
    donationLayout->setSpacing(12);

    // Header
    QHBoxLayout* headerLayout = new QHBoxLayout();
    QLabel* headerIcon = new QLabel("💝", donationGroup);
    headerIcon->setStyleSheet("font-size: 16px;");
    headerIcon->setFixedWidth(20);

    QLabel* headerLabel = new QLabel(tr("Support the development:"), donationGroup);
    headerLabel->setStyleSheet("color: #333;");

    headerLayout->addWidget(headerIcon);
    headerLayout->addWidget(headerLabel);
    headerLayout->addStretch();
    donationLayout->addLayout(headerLayout);

    // Description
    QLabel* descLabel =
        new QLabel(tr("If you find this software helpful, please consider supporting the development."), donationGroup);
    descLabel->setStyleSheet("color: #666; font-size: 11px;");
    descLabel->setWordWrap(true);
    donationLayout->addWidget(descLabel);

    // Tab buttons - centered with equal width
    QHBoxLayout* tabLayout = new QHBoxLayout();
    tabLayout->setSpacing(0);

    QPushButton* wechatTab = new QPushButton(tr("WeChat Pay"), donationGroup);
    wechatTab->setCheckable(true);
    wechatTab->setChecked(true);
    wechatTab->setFixedHeight(32);
    wechatTab->setFixedWidth(100);
    wechatTab->setStyleSheet(R"(
        QPushButton {
            background-color: #07C160;
            color: white;
            border: none;
            padding: 8px 16px;
            font-size: 11px;
            border-radius: 4px 0px 0px 4px;
        }
        QPushButton:hover {
            background-color: #06AD56;
        }
        QPushButton:pressed {
            background-color: #059B4C;
        }
        QPushButton:checked {
            background-color: #07C160;
        }
    )");

    QPushButton* alipayTab = new QPushButton(tr("Alipay"), donationGroup);
    alipayTab->setCheckable(true);
    alipayTab->setFixedHeight(32);
    alipayTab->setFixedWidth(100);
    alipayTab->setStyleSheet(R"(
        QPushButton {
            background-color: #E0E0E0;
            color: #666;
            border: none;
            padding: 8px 16px;
            font-size: 11px;
            border-radius: 0px 4px 4px 0px;
        }
        QPushButton:hover {
            background-color: #D0D0D0;
        }
        QPushButton:pressed {
            background-color: #C0C0C0;
        }
        QPushButton:checked {
            background-color: #1677FF;
            color: white;
        }
    )");

    tabLayout->addStretch();
    tabLayout->addWidget(wechatTab);
    tabLayout->addWidget(alipayTab);
    tabLayout->addStretch();
    donationLayout->addLayout(tabLayout);

    // QR Code container
    QStackedWidget* qrStack = new QStackedWidget(donationGroup);
    qrStack->setFixedSize(160, 160);
    qrStack->setStyleSheet(R"(
        QStackedWidget {
            background-color: white;
            border: 1px solid #e0e0e0;
            border-radius: 8px;
            padding: 10px;
        }
    )");

    // WeChat QR Code page
    QWidget* wechatPage = new QWidget();
    QVBoxLayout* wechatLayout = new QVBoxLayout(wechatPage);
    wechatLayout->setContentsMargins(0, 0, 0, 0);
    wechatLayout->setSpacing(0);

    QLabel* wechatQR = new QLabel(wechatPage);
    wechatQR->setFixedSize(140, 140);
    wechatQR->setPixmap(QPixmap(":/icons/wechatpay").scaled(140, 140, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    wechatQR->setAlignment(Qt::AlignCenter);

    wechatLayout->addWidget(wechatQR);
    wechatLayout->addStretch();
    wechatPage->setLayout(wechatLayout);
    qrStack->addWidget(wechatPage);

    // Alipay QR Code page
    QWidget* alipayPage = new QWidget();
    QVBoxLayout* alipayLayout = new QVBoxLayout(alipayPage);
    alipayLayout->setContentsMargins(0, 0, 0, 0);
    alipayLayout->setSpacing(0);

    QLabel* alipayQR = new QLabel(alipayPage);
    alipayQR->setFixedSize(140, 140);
    alipayQR->setPixmap(QPixmap(":/icons/alipay").scaled(140, 140, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    alipayQR->setAlignment(Qt::AlignCenter);

    alipayLayout->addWidget(alipayQR);
    alipayLayout->addStretch();
    alipayPage->setLayout(alipayLayout);
    qrStack->addWidget(alipayPage);

    // Center the QR stack
    QHBoxLayout* qrContainerLayout = new QHBoxLayout();
    qrContainerLayout->addStretch();
    qrContainerLayout->addWidget(qrStack);
    qrContainerLayout->addStretch();
    donationLayout->addLayout(qrContainerLayout);

    // Connect tab buttons to switch pages
    connect(wechatTab, &QPushButton::toggled, this, [wechatTab, alipayTab, qrStack](bool checked) {
        if (checked)
        {
            alipayTab->setChecked(false);
            qrStack->setCurrentIndex(0);
        }
    });

    connect(alipayTab, &QPushButton::toggled, this, [wechatTab, alipayTab, qrStack](bool checked) {
        if (checked)
        {
            wechatTab->setChecked(false);
            qrStack->setCurrentIndex(1);
        }
    });

    // Donation link
    QHBoxLayout* linkLayout = new QHBoxLayout();
    QLabel* linkIcon = new QLabel("🌐", donationGroup);
    linkIcon->setStyleSheet("font-size: 14px;");
    linkIcon->setFixedWidth(20);

    QLabel* donationLink = new QLabel(tr("<a href='#' style='color: #2196F3; text-decoration: none; font-size: "
                                         "11px;'>Visit donation page</a>"),
                                      donationGroup);
    donationLink->setOpenExternalLinks(false);

    linkLayout->addWidget(linkIcon);
    linkLayout->addWidget(donationLink);
    linkLayout->addStretch();
    donationLayout->addLayout(linkLayout);

    // Connect donation link signal
    QObject::connect(
        donationLink, &QLabel::linkActivated, [=] { QDesktopServices::openUrl(QUrl(AppConfig::DONATION_URL)); });

    return donationGroup;
}

QGroupBox* SettingsWindow::createResetSettingsGroup()
{
    QGroupBox* resetGroup = new QGroupBox(tr("Reset Settings"));
    QVBoxLayout* resetLayout = new QVBoxLayout(resetGroup);

    QLabel* resetDescLabel =
        new QLabel(tr("This will reset all settings to default values and clear device data."), resetGroup);
    resetDescLabel->setStyleSheet("color: #666; font-size: 11px;");
    resetDescLabel->setWordWrap(true);

    QPushButton* resetBtn = new QPushButton(tr("Reset to Default"), resetGroup);
    resetBtn->setFixedWidth(140);
    resetBtn->setStyleSheet(R"(
        QPushButton {
            background-color: #f44336;
            color: white;
            border: none;
            padding: 8px 16px;
            border-radius: 4px;
            font-size: 12px;
        }
        QPushButton:hover {
            background-color: #d32f2f;
        }
        QPushButton:pressed {
            background-color: #b71c1c;
        }
    )");

    resetLayout->addWidget(resetDescLabel);

    // Center the button
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    buttonLayout->addWidget(resetBtn);
    buttonLayout->addStretch();
    resetLayout->addLayout(buttonLayout);

    // Reset settings with confirmation dialog
    connect(resetBtn, &QPushButton::clicked, this, [this] {
        QMessageBox::StandardButton reply = QMessageBox::question(
            this,
            tr("Confirm Reset"),
            tr("This will reset all settings to default values and clear all device data (aliases, pinned status, "
               "cache).\n\nThis action cannot be undone. Are you sure you want to continue?"),
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No);

        if (reply == QMessageBox::Yes)
        {
            Settings::instance().resetToDefault();

            // Clear DeviceManager memory cache
            DeviceManager::instance().clearCache();

            QMessageBox::information(this,
                                     tr("Reset Complete"),
                                     tr("All settings have been reset to default values.\n\nThe application will "
                                        "restart to apply the changes."));
            qApp->exit(10086);
        }
    });

    return resetGroup;
}

void SettingsWindow::applyStylesheet()
{
    setStyleSheet(R"(
        QGroupBox {
            max-width: 264px;
            background-color: #f5f5f5;
            border: 1px solid #d0d0d0;
            border-radius: 5px;
            margin-top: 1.2em;
            color: #333333;
            font-weight: bold;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            subcontrol-position: top left;
            padding: 0 5px;
            background-color: #f5f5f5;
        }
        QCheckBox, QLabel {
            color: #333333;
        }
        QGroupBox > QLabel,QCheckBox {
            color: #333333;
            font-size:12px;
        }
        QLineEdit {
            background-color: #ffffff;
            color: #333333;
            border: 1px solid #d0d0d0;
            padding: 5px;
            font-size: 11px;
            font-weight: bold;
        }
        QPushButton {
            color: white;
            background-color: #008D4E;
            border: none;
            padding: 5px 10px;
            border-radius: 4px;
            font-size: 12px;
        }
        QPushButton:hover {
            background: rgba(0,128,78,0.85)
        }
        QPushButton:pressed {
            background: rgba(0,128,78,0.95)
        }
        QPushButton:disabled {
            background-color: #cccccc;
            color: #666666;
        }
        QComboBox {
            background-color: #ffffff;
            color: #333333;
            border: 1px solid #d0d0d0;
            padding: 5px;
            font-size: 11px;
            font-weight: bold;
        }
        QComboBox::drop-down {
            border: none;
        }
    )");
}
