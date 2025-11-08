#include "SettingsWindow.h"

#include "App.h"
#include "MainWindow.h"

#include <QApplication>
#include <QCheckBox>
#include <QCloseEvent>
#include <QComboBox>
#include <QDebug>
#include <QDesktopServices>
#include <QDialog>
#include <QFileDialog>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QLocale>
#include <QMessageBox>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QPixmap>
#include <QPushButton>
#include <QScreen>
#include <QScrollArea>
#include <QSettings>
#include <QStackedWidget>
#include <QStandardPaths>
#include <QSysInfo>
#include <QTimer>
#include <QVBoxLayout>
#include <manager/PluginManager.h>
#include <plugin/IDevicePlugin.h>
#include <settings/Settings.h>

// 初始化静态语言映射表
const QMap<QString, QString> SettingsWindow::m_langMap = {
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

SettingsWindow::SettingsWindow(QWidget* parent) : FramelessWindow(parent)
{
    // Set window size same as main window
    QScreen* screen = QApplication::primaryScreen();
    QRect screenGeometry = screen->geometry();
    int width = UI::WINDOW_WIDTH;
    int height = UI::WINDOW_HEIGHT;
    int x = (screenGeometry.width() - width) / 2;
    int y = (screenGeometry.height() - height) / 2;
    setGeometry(x, y, width, height);

    // 初始化网络管理器
    m_networkManager = new QNetworkAccessManager(this);
    connect(m_networkManager, &QNetworkAccessManager::finished, this, &SettingsWindow::onUpdateChecked);

    setupUI();
}

SettingsWindow::~SettingsWindow() {}

void SettingsWindow::loadSettings()
{
    // 添加空指针检查
    if (!m_logCheckBox || !m_logDirEdit || !m_screenshotDirEdit || !m_updateCheckBox || !m_languageCombo)
    {
        qWarning() << "SettingsWindow: UI elements not properly initialized";
        return;
    }

    try
    {
        // 使用Settings类管理所有设置
        Settings& settings = Settings::instance();

        // 动态加载插件设置
        for (auto it = m_pluginCheckBoxes.constBegin(); it != m_pluginCheckBoxes.constEnd(); ++it)
        {
            const QString& pluginName = it.key();
            QCheckBox* checkBox = it.value();
            if (checkBox)
            {
                // 直接使用isPluginEnabled方法检查插件是否启用
                checkBox->setChecked(settings.isPluginEnabled(pluginName));
            }
        }

        // Load log settings
        m_logCheckBox->setChecked(settings.logToFile());
        m_logDirEdit->setText(settings.logDir());

        // Load screenshot directory settings
        m_screenshotDirEdit->setText(settings.screenshotDir());

        // Load update settings
        m_updateCheckBox->setChecked(settings.autoCheckUpdates());

        // Load language settings
        QString savedLang = settings.language();
        if (savedLang.isEmpty())
        {
            // 系统默认语言
            m_languageCombo->setCurrentIndex(0);
        }
        else
        {
            // 查找匹配的语言代码
            int index = m_languageCombo->findData(savedLang);
            if (index >= 0)
            {
                m_languageCombo->setCurrentIndex(index);
            }
        }
    }
    catch (const std::exception& e)
    {
        qWarning() << "SettingsWindow: Error loading settings:" << e.what();
    }
}

// 保存日志设置
void SettingsWindow::saveLogSettings()
{
    if (!m_logCheckBox || !m_logDirEdit)
    {
        qWarning() << "SettingsWindow: Log UI elements not properly initialized";
        return;
    }

    try
    {
        // 获取当前日志设置状态
        bool logEnabled = m_logCheckBox->isChecked();
        QString logDir = m_logDirEdit->text();

        // 保存日志设置

        // 首先保存日志目录，然后再设置日志启用状态
        Settings& settings = Settings::instance();
        settings.setLogDir(logDir);

        // 检查目录是否有效
        if (logEnabled)
        {
            QDir dir(logDir);
            if (!dir.exists())
            {
                qWarning() << "SettingsWindow: Log directory does not exist:" << logDir;
                // 可以选择不启用日志或创建目录
                // 这里我们仍然尝试启用日志，让Settings类自己处理错误
            }
        }

        // 关键操作：设置日志启用状态
        try
        {
            settings.setLogToFile(logEnabled);
            // 日志设置保存成功

            // 显示保存成功通知
            showSaveNotification("Log settings saved");
        }
        catch (const std::exception& innerException)
        {
            qWarning() << "SettingsWindow: Error in setLogToFile:" << innerException.what();
            // 显示错误通知但不崩溃
            if (StatusBar* statusBar = this->getStatusBar())
            {
                statusBar->setNotification("Failed to update log settings");
            }
        }
    }
    catch (const std::exception& e)
    {
        qWarning() << "SettingsWindow: Error saving log settings:" << e.what();
        // 显示错误通知
        if (StatusBar* statusBar = this->getStatusBar())
        {
            statusBar->setNotification("Failed to save log settings");
        }
    }
    catch (...)
    {
        // 捕获所有异常，包括非标准C++异常
        qWarning() << "SettingsWindow: Unknown exception in saveLogSettings";
        if (StatusBar* statusBar = this->getStatusBar())
        {
            statusBar->setNotification("Unknown error saving log settings");
        }
    }
}

// 保存截图设置
void SettingsWindow::saveScreenshotSettings()
{
    if (!m_screenshotDirEdit)
    {
        qWarning() << "SettingsWindow: Screenshot UI elements not properly initialized";
        return;
    }

    try
    {
        Settings& settings = Settings::instance();
        settings.setScreenshotDir(m_screenshotDirEdit->text());
    }
    catch (const std::exception& e)
    {
        qWarning() << "SettingsWindow: Error saving screenshot settings:" << e.what();
    }
}

// 保存更新设置
void SettingsWindow::saveUpdateSettings()
{
    if (!m_updateCheckBox)
    {
        qWarning() << "SettingsWindow: Update UI elements not properly initialized";
        return;
    }

    try
    {
        Settings& settings = Settings::instance();
        bool autoCheck = m_updateCheckBox->isChecked();
        settings.setAutoCheckUpdates(autoCheck);

        // 当勾选自动检测更新时，立即检查更新
        if (autoCheck)
        {
            checkForUpdates();
        }
    }
    catch (const std::exception& e)
    {
        qWarning() << "SettingsWindow: Error saving update settings:" << e.what();
    }
}

// 保存语言设置
void SettingsWindow::saveLanguageSettings()
{
    if (!m_languageCombo)
    {
        qWarning() << "SettingsWindow: Language UI elements not properly initialized";
        return;
    }

    try
    {
        // 获取选中的语言代码
        QString langCode = m_languageCombo->currentData().toString();
        QString langText = m_languageCombo->currentText();

        Settings& settings = Settings::instance();
        QString currentLang = settings.language();

        // 只有当语言确实发生变化时，才提示重启
        bool languageChanged = (currentLang != (langCode.isEmpty() ? "" : langCode));

        // 保存新的语言设置
        settings.setLanguage(langCode.isEmpty() ? "" : langCode);

        if (languageChanged)
        {
            qDebug() << "Language changed to:" << langText << "(code:" << langCode << ")";

            // 显示保存成功通知
            showSaveNotification("Language setting saved.");

            // 弹出确认对话框，询问用户是否重启应用
            int reply =
                MessageBox::question(this,
                                     tr("Restart Required"),
                                     tr("Language changes require application restart to take effect. Restart now?"));

            if (reply == QDialog::Accepted)
            {
                // 用户确认后，使用特殊退出码退出应用，main函数会检测此退出码并重启应用
                qApp->exit(10086);
            }
        }
    }
    catch (const std::exception& e)
    {
        qWarning() << "SettingsWindow: Error saving language settings:" << e.what();
    }
}

// 保存插件设置
void SettingsWindow::savePluginSettings()
{
    try
    {
        Settings& settings = Settings::instance();
        QStringList enabledPlugins;
        for (auto it = m_pluginCheckBoxes.constBegin(); it != m_pluginCheckBoxes.constEnd(); ++it)
        {
            const QString& pluginName = it.key();
            QCheckBox* checkBox = it.value();
            if (checkBox && checkBox->isChecked())
            {
                enabledPlugins << pluginName;
            }
        }
        settings.setEnabledPlugins(enabledPlugins);
    }
    catch (const std::exception& e)
    {
        qWarning() << "SettingsWindow: Error saving plugin settings:" << e.what();
    }
}

// 显示保存通知
void SettingsWindow::showSaveNotification(const QString& message)
{
    if (StatusBar* statusBar = this->getStatusBar())
    {
        statusBar->setNotification(message);
    }
}

// 主保存函数，在关闭窗口时调用
void SettingsWindow::saveSettings()
{
    try
    {
        // 保存所有设置
        saveLogSettings();
        saveScreenshotSettings();
        saveUpdateSettings();
        saveLanguageSettings();
        savePluginSettings();

        // 显示保存成功通知
        showSaveNotification("Settings saved");
    }
    catch (const std::exception& e)
    {
        qWarning() << "SettingsWindow: Error in main save function:" << e.what();
        showSaveNotification("Failed to save settings");
    }
}

void SettingsWindow::closeEvent(QCloseEvent* event)
{
    // 在关闭窗口前，确保设置已保存
    saveSettings();

    QWidget::closeEvent(event);
}

void SettingsWindow::resetSettings()
{
    try
    {
        // 使用Settings类重置所有设置
        Settings& settings = Settings::instance();
        settings.resetToDefault();

        // 根据驱动是否可用设置插件默认状态
        PluginManager& pluginManager = PluginManager::instance();
        QStringList enabledPlugins;
        for (IDevicePlugin* plugin : pluginManager.getAllPlugins())
        {
            if (plugin && plugin->checkDriverAvailable())
            {
                enabledPlugins << plugin->pluginName();
            }
        }

        // 使用Settings类设置默认值
        settings.setEnabledPlugins(enabledPlugins);
        settings.setAutoCheckUpdates(true);
        // 使用系统默认语言
        settings.setLanguage(QLocale::system().name());

        // 清理 device_data.json 文件
        QString deviceDataPath =
            QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/device_data.json";
        if (QFile::exists(deviceDataPath))
        {
            QFile::remove(deviceDataPath);
            qDebug() << "Device data file cleaned up:" << deviceDataPath;
        }

        // 先重建插件复选框，这样在设置UI控件状态时不会触发信号
        rebuildPluginCheckboxes();

        // 添加空指针检查后重置UI控件状态
        if (m_logCheckBox)
            m_logCheckBox->setChecked(settings.logToFile());
        if (m_logDirEdit)
            m_logDirEdit->setText(settings.logDir());
        if (m_screenshotDirEdit)
            m_screenshotDirEdit->setText(settings.screenshotDir());
        if (m_updateCheckBox)
            m_updateCheckBox->setChecked(true);
        if (m_languageCombo)
            m_languageCombo->setCurrentText("English");

        // 显示重置通知
        if (StatusBar* statusBar = this->getStatusBar())
        {
            statusBar->setNotification("Settings reset to default");
        }
    }
    catch (const std::exception& e)
    {
        qWarning() << "SettingsWindow: Error resetting settings:" << e.what();
        // 显示错误通知
        if (StatusBar* statusBar = this->getStatusBar())
        {
            statusBar->setNotification("Failed to reset settings");
        }
    }
}

// 简化的插件复选框重建方法
void SettingsWindow::rebuildPluginCheckboxes()
{
    try
    {
        // 首先断开所有现有复选框的信号连接，避免信号发送到已删除的对象
        for (auto it = m_pluginCheckBoxes.constBegin(); it != m_pluginCheckBoxes.constEnd(); ++it)
        {
            QCheckBox* checkBox = it.value();
            if (checkBox)
            {
                checkBox->disconnect(this); // 断开所有连接到当前对象的信号
            }
        }

        // 查找Plugin Settings组框和其布局
        QGroupBox* pluginGroup = findChild<QGroupBox*>("pluginSettingsGroup");
        if (!pluginGroup)
        {
            // 如果找不到具名组框，尝试通过层次结构查找
            QScrollArea* scrollArea = findChild<QScrollArea*>();
            if (scrollArea && scrollArea->widget())
            {
                pluginGroup = scrollArea->widget()->findChild<QGroupBox*>();
            }
        }

        if (!pluginGroup)
            return;

        // 获取并清空现有布局
        QVBoxLayout* pluginLayout = qobject_cast<QVBoxLayout*>(pluginGroup->layout());
        if (pluginLayout)
        {
            // 清空布局但保持布局对象
            while (QLayoutItem* item = pluginLayout->takeAt(0))
            {
                if (item->widget())
                {
                    delete item->widget();
                }
                delete item;
            }

            // 清除复选框映射
            m_pluginCheckBoxes.clear();

            // 重新创建复选框
            createPluginCheckboxes(pluginLayout, pluginGroup);
        }
    }
    catch (const std::exception& e)
    {
        qWarning() << "SettingsWindow: Error rebuilding plugin checkboxes:" << e.what();
    }
}

// 从PluginManager动态创建插件复选框
void SettingsWindow::createPluginCheckboxes(QVBoxLayout* pluginLayout, QWidget* parent)
{
    try
    {
        // 从PluginManager获取所有插件
        PluginManager& pluginManager = PluginManager::instance();
        QList<IDevicePlugin*> plugins = pluginManager.getAllPlugins();

        // 读取当前启用的插件设置
        Settings& settings = Settings::instance();

        // 为每个插件创建复选框
        for (IDevicePlugin* plugin : plugins)
        {
            if (!plugin)
                continue;

            QString pluginName = plugin->pluginName();
            QString driverName = plugin->getDriverName();
            bool isDriverAvailable = plugin->checkDriverAvailable();

            // 创建包含插件名和驱动名的文本
            QString checkboxText = QString("%1 (%2)").arg(pluginName).arg(driverName);
            QCheckBox* checkBox = new QCheckBox(checkboxText, parent);

            // 设置复选框状态：直接使用Settings类的isPluginEnabled方法检查插件是否启用
            bool shouldBeChecked = settings.isPluginEnabled(pluginName);

            // 如果驱动不可用，无论设置如何都设为未选中，并保存这个状态
            if (!isDriverAvailable)
            {
                // 插件驱动不可用，设置为未选中
                shouldBeChecked = false;
                // 将未选中状态保存到设置中
                settings.setPluginEnabled(pluginName, false);
            }
            checkBox->setChecked(shouldBeChecked);

            // 如果驱动不可用，禁用复选框（样式将从Green.qss中应用）
            if (!isDriverAvailable)
            {
                checkBox->setDisabled(true);
                // 安全地设置tooltip，确保driverName不为空
                QString tooltipText;
                if (!driverName.isEmpty())
                {
                    tooltipText = QString("Driver '%1' not found. Please install it first.").arg(driverName);
                }
                else
                {
                    tooltipText = "Required driver not found. Please install it first.";
                }
                checkBox->setToolTip(tooltipText);
            }

            // 将复选框添加到布局
            pluginLayout->addWidget(checkBox);

            // 存储复选框引用
            m_pluginCheckBoxes[pluginName] = checkBox;

            // 连接信号到专门的插件设置保存函数
            if (isDriverAvailable)
            { // 只连接可用驱动的信号
                connect(checkBox, &QCheckBox::toggled, this, &SettingsWindow::savePluginSettings);
            }
        }

        // 如果没有插件，添加一个占位符
        if (m_pluginCheckBoxes.isEmpty())
        {
            QLabel* placeholder = new QLabel("No plugins available", parent);
            placeholder->setAlignment(Qt::AlignCenter);
            pluginLayout->addWidget(placeholder);
        }

        // 添加驱动下载链接（位于PluginGroup内部）
        QLabel* driverDownloadLink = new QLabel(parent);
        driverDownloadLink->setObjectName("driverDownloadLink");
        driverDownloadLink->setTextFormat(Qt::RichText);
        driverDownloadLink->setText(
            QString("<a href=\"%1\">%2</a>").arg(App::DRIVER_DOWNLOAD_URL).arg(tr("Download required drivers")));
        driverDownloadLink->setTextInteractionFlags(Qt::TextBrowserInteraction);
        driverDownloadLink->setOpenExternalLinks(true);
        driverDownloadLink->setAlignment(Qt::AlignCenter);
        pluginLayout->addWidget(driverDownloadLink);
    }
    catch (const std::exception& e)
    {
        qWarning() << "SettingsWindow: Error creating plugin checkboxes:" << e.what();
    }
}

QGroupBox* SettingsWindow::createLanguageSettingsGroup(QWidget* parent)
{
    QGroupBox* languageGroup = new QGroupBox(tr("Language Settings"), parent);
    QVBoxLayout* languageLayout = new QVBoxLayout(languageGroup);

    // Create language combo box
    m_languageCombo = new QComboBox(parent);

    m_languageCombo->addItem(tr("System default"), QLocale::system().name());
    for (auto it = m_langMap.constBegin(); it != m_langMap.constEnd(); ++it)
    {
        m_languageCombo->addItem(it.value(), it.key());
    }

    // Add description label
    QLabel* languageDescLabel = new QLabel(parent);
    languageDescLabel->setObjectName("donationDescription"); // Reuse style from donation description
    languageDescLabel->setText(tr("Need restart the application after changing language setting."));
    languageDescLabel->setWordWrap(true);

    // Layout the components
    languageLayout->addWidget(m_languageCombo);
    languageLayout->addWidget(languageDescLabel);

    return languageGroup;
}

QGroupBox* SettingsWindow::createDonationSettingsGroup()
{
    QGroupBox* donationGroup = new QGroupBox(tr("Support Development"));
    QVBoxLayout* donationLayout = new QVBoxLayout(donationGroup);
    donationLayout->setSpacing(10);

    // Description
    QLabel* descLabel = new QLabel(
        tr("If you find this application useful, please consider supporting its development."), donationGroup);
    descLabel->setObjectName("donationDescription");
    descLabel->setWordWrap(true);
    donationLayout->addWidget(descLabel);

    // Payment methods
    QLabel* paymentLabel = new QLabel(tr("Payment methods:"), donationGroup);
    paymentLabel->setObjectName("paymentLabel");
    donationLayout->addWidget(paymentLabel);

    // 使用QTabWidget替代按钮组实现左右切换
    QTabWidget* paymentTabs = new QTabWidget(donationGroup);
    paymentTabs->setObjectName("paymentTabs");   // 添加对象名便于调试
    paymentTabs->setFixedSize(200, 200);         // 总宽度与二维码图片相匹配
    paymentTabs->setUsesScrollButtons(false);    // 禁用滚动按钮
    paymentTabs->tabBar()->setExpanding(false);  // 禁用自动扩展
    paymentTabs->tabBar()->setMinimumWidth(200); // 设置tab bar最小宽度

    // 样式已移至Green.qss文件中

    // 设置固定尺寸用于图片缩放
    QSize qrSize(180, 180);

    // WeChat Pay QR code tab
    QWidget* wechatTabWidget = new QWidget(paymentTabs);
    QVBoxLayout* wechatLayout = new QVBoxLayout(wechatTabWidget);
    wechatLayout->setAlignment(Qt::AlignCenter);

    QLabel* wechatQR = new QLabel(wechatTabWidget);
    wechatQR->setObjectName("wechatQR");
    wechatQR->setAlignment(Qt::AlignCenter);

    QString wechatPath = ":/icon/donate/wechatpay.png";
    QPixmap wechatPixmap;
    bool wechatLoaded = wechatPixmap.load(wechatPath);

    if (!wechatLoaded || wechatPixmap.isNull())
    {
        wechatQR->setText(tr("WeChat Pay QR Code (Image failed to load)"));
        wechatQR->setProperty("failed", true);
    }
    else
    {
        wechatQR->setPixmap(wechatPixmap.scaled(qrSize, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }

    wechatLayout->addWidget(wechatQR);
    paymentTabs->addTab(wechatTabWidget, tr("WeChat Pay"));

    // Alipay QR code tab
    QWidget* alipayTabWidget = new QWidget(paymentTabs);
    QVBoxLayout* alipayLayout = new QVBoxLayout(alipayTabWidget);
    alipayLayout->setAlignment(Qt::AlignCenter);

    QLabel* alipayQR = new QLabel(alipayTabWidget);
    alipayQR->setObjectName("alipayQR");
    alipayQR->setAlignment(Qt::AlignCenter);

    QString alipayPath = ":/icon/donate/alipay.png";
    QPixmap alipayPixmap;
    bool alipayLoaded = alipayPixmap.load(alipayPath);

    if (!alipayLoaded || alipayPixmap.isNull())
    {
        alipayQR->setText(tr("Alipay QR Code (Image failed to load)"));
        alipayQR->setProperty("failed", true);
    }
    else
    {
        alipayQR->setPixmap(alipayPixmap.scaled(qrSize, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }

    alipayLayout->addWidget(alipayQR);
    paymentTabs->addTab(alipayTabWidget, tr("Alipay"));

    // Center the tabs
    QHBoxLayout* tabsLayout = new QHBoxLayout();
    tabsLayout->addStretch();
    tabsLayout->addWidget(paymentTabs);
    tabsLayout->addStretch();
    donationLayout->addLayout(tabsLayout);

    // 默认选中第一个标签页（微信支付）
    paymentTabs->setCurrentIndex(0);

    // Donation link
    QHBoxLayout* linkLayout = new QHBoxLayout();
    QLabel* linkIcon = new QLabel("🌐", donationGroup);
    linkIcon->setObjectName("linkIcon");
    linkIcon->setFixedWidth(20);

    QLabel* donationLink = new QLabel("<a href='#'>" + tr("Visit donation page") + "</a>", donationGroup);
    donationLink->setOpenExternalLinks(false);

    linkLayout->addWidget(linkIcon);
    linkLayout->addWidget(donationLink);
    linkLayout->addStretch();
    donationLayout->addLayout(linkLayout);

    // Connect donation link signal
    QObject::connect(donationLink, &QLabel::linkActivated, [=] { QDesktopServices::openUrl(QUrl(App::DONATION_URL)); });

    return donationGroup;
}

void SettingsWindow::donate()
{
    // Create a dialog to show donation information
    QDialog* donateDialog = new QDialog(this);
    donateDialog->setWindowTitle(tr("Support Development"));
    donateDialog->setMinimumSize(320, 400);
    donateDialog->setMaximumSize(320, 400);
    donateDialog->setModal(true);

    QVBoxLayout* mainLayout = new QVBoxLayout(donateDialog);
    mainLayout->setSpacing(15);
    mainLayout->setContentsMargins(20, 20, 20, 20);

    // Add the donation group directly to the dialog
    mainLayout->addWidget(createDonationSettingsGroup());

    // Close button
    QPushButton* closeButton = new QPushButton(tr("Close"), donateDialog);
    closeButton->setObjectName("donateCloseButton");
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    buttonLayout->addWidget(closeButton);
    mainLayout->addLayout(buttonLayout);

    connect(closeButton, &QPushButton::clicked, donateDialog, &QDialog::accept);

    // Show the dialog
    donateDialog->exec();
    delete donateDialog;
}

// 检查更新
void SettingsWindow::checkForUpdates()
{
    if (!m_updateInfoLabel)
        return;

    m_updateInfoLabel->setText(tr("Checking for updates..."));

    // 发送网络请求检查更新
    QNetworkRequest request((QUrl(App::UPDATE_URL)));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    m_networkManager->get(request);
}

// 处理更新检查响应
void SettingsWindow::onUpdateChecked(QNetworkReply* reply)
{
    if (!m_updateInfoLabel)
        return;

    // 检查网络错误
    if (reply->error() != QNetworkReply::NoError)
    {
        m_updateInfoLabel->setText(tr("Failed to check for updates. Please try again later."));
        reply->deleteLater();
        return;
    }

    QByteArray data = reply->readAll();
    // 打印收到的reply
    // 更新回复原始数据已处理

    QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
    if (!jsonDoc.isObject())
    {
        m_updateInfoLabel->setText(tr("Invalid update information format."));
        reply->deleteLater();
        return;
    }

    QJsonObject jsonObj = jsonDoc.object();

    // 根据当前运行平台获取对应的更新信息
    QString platformKey;
    QString productType = QSysInfo::productType();

    // 根据不同平台设置对应的键
    if (productType.contains("windows", Qt::CaseInsensitive))
    {
        platformKey = "windows";
    }
    else if (productType.contains("linux", Qt::CaseInsensitive))
    {
        platformKey = "linux";
    }
    else if (productType.contains("osx", Qt::CaseInsensitive) || productType.contains("macos", Qt::CaseInsensitive))
    {
        platformKey = "macos";
    }
    else
    {
        // 默认使用windows作为后备选项
        platformKey = "windows";
        qWarning() << "Unknown platform detected, using Windows update information as fallback.";
    }

    // 获取当前平台的更新信息
    if (!jsonObj.contains(platformKey) || !jsonObj[platformKey].isObject())
    {
        m_updateInfoLabel->setText(tr("No update information available for your platform (%1).").arg(platformKey));
        reply->deleteLater();
        return;
    }

    QJsonObject platformObj = jsonObj[platformKey].toObject();

    // 检查必要字段 - 只需要version字段
    if (!platformObj.contains("version"))
    {
        m_updateInfoLabel->setText(tr("Incomplete update information - missing version."));
        reply->deleteLater();
        return;
    }

    QString latestVersion = platformObj["version"].toString();
    QString currentVersion = QApplication::applicationVersion();
    QString downloadUrl = platformObj.contains("download_url") ? platformObj["download_url"].toString() : "";

    // 检查版本是否为空
    if (latestVersion.isEmpty())
    {
        m_updateInfoLabel->setText(tr("Invalid version information."));
        reply->deleteLater();
        return;
    }

    // 版本比较
    if (latestVersion == currentVersion)
    {
        m_updateInfoLabel->setText(tr("You are using the latest version."));
    }
    else
    {
        // 显示新版本信息
        QString updateInfo = QString(tr("New version available: %1<br>")).arg(latestVersion);

        // 如果有更新日志，显示它
        if (platformObj.contains("change_log"))
        {
            QString changeLog = platformObj["change_log"].toString();
            updateInfo += QString("<br>Changes:<br>%1<br>").arg(changeLog.replace("\n", "<br>").replace("\r", ""));
        }

        // 只有当有下载链接时才显示下载按钮
        if (!downloadUrl.isEmpty())
        {
            updateInfo += QString(
                              "<br><a href='%1' style='color: #0066cc; text-decoration: underline; font-weight: "
                              "bold;'>%2</a>")
                              .arg(downloadUrl, tr("Download update"));
        }

        m_updateInfoLabel->setText(updateInfo);
    }

    reply->deleteLater();
}

void SettingsWindow::selectLogDirectory()
{
    QString dir = QFileDialog::getExistingDirectory(this, tr("Select log directory"), m_logDirEdit->text());
    if (!dir.isEmpty())
    {
        m_logDirEdit->setText(dir);
        saveLogSettings();
        showSaveNotification(tr("Log settings saved"));
    }
}

void SettingsWindow::selectScreenshotDirectory()
{
    QString dir =
        QFileDialog::getExistingDirectory(this, tr("Select screenshot directory"), m_screenshotDirEdit->text());
    if (!dir.isEmpty())
    {
        m_screenshotDirEdit->setText(dir);
        saveScreenshotSettings();
        showSaveNotification(tr("Screenshot settings saved"));
    }
}

void SettingsWindow::setupUI()
{
    getTitleBar()->setTitle(tr("Settings"));
    connect(getTitleBar(), &TitleBar::closeClicked, this, &SettingsWindow::close);
    connect(getTitleBar(), &TitleBar::minimizeClicked, this, &SettingsWindow::showMinimized);

    // Get page container from base class
    QWidget* pageContainer = getPage();
    QVBoxLayout* pageLayout = new QVBoxLayout(pageContainer);
    pageLayout->setContentsMargins(0, 0, 0, 0);

    // Create scroll area
    QScrollArea* scrollArea = new QScrollArea(this);

    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    // Create content container
    QWidget* contentWidget = new QWidget();

    QVBoxLayout* contentLayout = new QVBoxLayout(contentWidget);
    contentLayout->setSpacing(10);

    // Plugin settings group
    QGroupBox* pluginGroup = new QGroupBox(tr("Plugin Settings"), pageContainer);

    QVBoxLayout* pluginLayout = new QVBoxLayout(pluginGroup);

    // 动态创建插件复选框
    createPluginCheckboxes(pluginLayout, pageContainer);

    contentLayout->addWidget(pluginGroup);

    // Log settings group
    QGroupBox* logGroup = new QGroupBox(tr("Log Settings"), pageContainer);

    QVBoxLayout* logLayout = new QVBoxLayout(logGroup);
    m_logCheckBox = new QCheckBox(tr("Enable log writing to file"), pageContainer);

    logLayout->addWidget(m_logCheckBox);

    // Log directory selection
    QHBoxLayout* logDirLayout = new QHBoxLayout();
    m_logDirEdit = new QLineEdit(pageContainer);

    m_logDirEdit->setPlaceholderText(tr("Log directory"));
    m_logDirButton = new QPushButton(tr("Select"), pageContainer);

    logDirLayout->addWidget(m_logDirEdit);
    logDirLayout->addWidget(m_logDirButton);
    logLayout->addLayout(logDirLayout);
    contentLayout->addWidget(logGroup);

    // Screenshot settings group
    QGroupBox* screenshotGroup = new QGroupBox(tr("Screenshot Settings"), pageContainer);

    QVBoxLayout* screenshotLayout = new QVBoxLayout(screenshotGroup);

    // Screenshot directory selection
    QHBoxLayout* screenshotDirLayout = new QHBoxLayout();
    m_screenshotDirEdit = new QLineEdit(pageContainer);

    m_screenshotDirEdit->setPlaceholderText(tr("Screenshot directory"));
    m_screenshotDirButton = new QPushButton(tr("Select"), pageContainer);

    screenshotDirLayout->addWidget(m_screenshotDirEdit);
    screenshotDirLayout->addWidget(m_screenshotDirButton);
    screenshotLayout->addLayout(screenshotDirLayout);
    contentLayout->addWidget(screenshotGroup);

    // Update settings group
    QGroupBox* updateGroup = new QGroupBox(tr("Update Settings"), pageContainer);

    QVBoxLayout* updateLayout = new QVBoxLayout(updateGroup);
    m_updateCheckBox = new QCheckBox(tr("Automatically check for updates"), pageContainer);

    updateLayout->addWidget(m_updateCheckBox);

    // 添加更新信息标签
    m_updateInfoLabel = new QLabel("", pageContainer);
    m_updateInfoLabel->setWordWrap(true);
    m_updateInfoLabel->setOpenExternalLinks(true);
    m_updateInfoLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
    m_updateInfoLabel->setObjectName("updateInfoLabel");
    m_updateInfoLabel->setAlignment(Qt::AlignLeft);
    updateLayout->addWidget(m_updateInfoLabel);

    contentLayout->addWidget(updateGroup);

    // Language settings group
    contentLayout->addWidget(createLanguageSettingsGroup(pageContainer));

    // Other settings group
    QGroupBox* otherGroup = new QGroupBox(tr("Other Settings"), pageContainer);

    QVBoxLayout* otherLayout = new QVBoxLayout(otherGroup);
    QPushButton* resetButton = new QPushButton(tr("Reset Settings"), pageContainer);
    resetButton->setObjectName("resetButton");

    otherLayout->addWidget(resetButton);
    QPushButton* donateButton = new QPushButton(tr("Donate"), pageContainer);

    otherLayout->addWidget(donateButton);
    contentLayout->addWidget(otherGroup);

    contentLayout->addStretch();
    scrollArea->setWidget(contentWidget);
    pageLayout->addWidget(scrollArea);

    // Set up status bar
    getStatusBar()->setStatusMessage(tr("Ready"));

    // Load settings
    loadSettings();

    // Connect signals and slots to specific save functions
    connect(m_logCheckBox, &QCheckBox::toggled, this, &SettingsWindow::saveLogSettings);
    connect(m_logDirButton, &QPushButton::clicked, this, &SettingsWindow::selectLogDirectory);
    connect(m_screenshotDirButton, &QPushButton::clicked, this, &SettingsWindow::selectScreenshotDirectory);
    connect(m_updateCheckBox, &QCheckBox::toggled, this, &SettingsWindow::saveUpdateSettings);
    connect(m_languageCombo, &QComboBox::currentTextChanged, this, &SettingsWindow::saveLanguageSettings);

    connect(resetButton, &QPushButton::clicked, this, &SettingsWindow::resetSettings);
    connect(donateButton, &QPushButton::clicked, this, &SettingsWindow::donate);

    // 如果设置中已启用自动检查更新，则在UI加载完成后检查更新
    if (m_updateCheckBox && m_updateCheckBox->isChecked())
    {
        QTimer::singleShot(100, this, &SettingsWindow::checkForUpdates);
    }
}