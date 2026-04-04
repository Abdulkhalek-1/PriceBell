#include "gui/SettingsDialog.hpp"
#include "core/PluginManager.hpp"
#include "core/IPlugin2.hpp"
#include "utils/AutoStartManager.hpp"
#include "utils/SettingsProvider.hpp"

#include <QApplication>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QSettings>
#include <QFileDialog>
#include <QLabel>
#include <QMessageBox>
#include <QComboBox>
#include <QFile>
#include <QScrollArea>
#include <QSizePolicy>
#include <QSoundEffect>
#include <QUrl>

// Helper: wraps a widget in a QScrollArea so tabs with many groups don't clip.
static QWidget* scrollWrap(QWidget* content) {
    QScrollArea* scroll = new QScrollArea();
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setWidget(content);
    return scroll;
}

SettingsDialog::SettingsDialog(QWidget* parent, PluginManager* pluginManager)
    : QDialog(parent), m_pluginManager(pluginManager)
{
    setWindowTitle(tr("Settings"));
    resize(540, 520);
    setupUi();
    loadSettings();
}

void SettingsDialog::setupUi() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(8);

    m_tabWidget = new QTabWidget(this);
    m_tabWidget->addTab(createGeneralTab(),  tr("General"));
    m_tabWidget->addTab(createApiKeysTab(),  tr("API Keys"));
    m_tabWidget->addTab(createPluginsTab(),  tr("Plugins"));
    mainLayout->addWidget(m_tabWidget);

    QDialogButtonBox* buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    mainLayout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, this, &SettingsDialog::saveSettings);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

QWidget* SettingsDialog::createGeneralTab() {
    QWidget* page = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(page);
    layout->setContentsMargins(10, 14, 10, 10);
    layout->setSpacing(12);

    // -- Startup ------------------------------------------------------------------
    QGroupBox* startupGroup = new QGroupBox(tr("Startup"), page);
    QVBoxLayout* startupLayout = new QVBoxLayout(startupGroup);
    startupLayout->setContentsMargins(10, 8, 10, 8);
    m_autoStartCheck = new QCheckBox(tr("Open on Startup"), page);
    m_autoStartCheck->setToolTip(tr("Automatically launch PriceBell when you log in"));
    startupLayout->addWidget(m_autoStartCheck);
    m_launchMinimizedCheck = new QCheckBox(tr("Launch minimized to tray"), page);
    m_launchMinimizedCheck->setToolTip(tr("Start in background without showing the main window"));
    startupLayout->addWidget(m_launchMinimizedCheck);
    layout->addWidget(startupGroup);

    // -- Updates ------------------------------------------------------------------
    QGroupBox* updateGroup = new QGroupBox(tr("Updates"), page);
    QVBoxLayout* updateLayout = new QVBoxLayout(updateGroup);
    updateLayout->setContentsMargins(10, 8, 10, 8);
    m_autoUpdateCheck = new QCheckBox(tr("Check for updates on startup"), page);
    m_autoUpdateCheck->setToolTip(tr("Automatically check for new versions when the app starts"));
    updateLayout->addWidget(m_autoUpdateCheck);
    layout->addWidget(updateGroup);

    // -- Notifications ------------------------------------------------------------
    QGroupBox* notifGroup = new QGroupBox(tr("Notifications"), page);
    QVBoxLayout* notifLayout = new QVBoxLayout(notifGroup);
    notifLayout->setContentsMargins(10, 8, 10, 8);
    notifLayout->setSpacing(6);

    m_notificationSoundCheck = new QCheckBox(tr("Play notification sound"), page);
    m_notificationSoundCheck->setToolTip(tr("Play a sound when a price alert is triggered"));
    notifLayout->addWidget(m_notificationSoundCheck);

    // Sound file path — full-width line edit
    m_soundPathEdit = new QLineEdit(page);
    m_soundPathEdit->setPlaceholderText(tr("Default (built-in bell)"));
    m_soundPathEdit->setToolTip(tr("Path to a .wav file, or leave empty for built-in sound"));
    notifLayout->addWidget(m_soundPathEdit);

    // Compact button row below the path
    QHBoxLayout* soundBtnRow = new QHBoxLayout();
    soundBtnRow->setSpacing(4);
    soundBtnRow->setContentsMargins(0, 0, 0, 0);
    QPushButton* browseSoundBtn = new QPushButton(tr("Browse"), page);
    QPushButton* testSoundBtn   = new QPushButton(tr("Test"), page);
    QPushButton* resetSoundBtn  = new QPushButton(tr("Reset"), page);
    browseSoundBtn->setToolTip(tr("Choose a .wav sound file"));
    testSoundBtn->setToolTip(tr("Play the selected notification sound"));
    resetSoundBtn->setToolTip(tr("Reset to default built-in sound"));
    for (auto* btn : {browseSoundBtn, testSoundBtn, resetSoundBtn}) {
        btn->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        btn->setMaximumHeight(26);
    }
    soundBtnRow->addWidget(browseSoundBtn);
    soundBtnRow->addWidget(testSoundBtn);
    soundBtnRow->addWidget(resetSoundBtn);
    soundBtnRow->addStretch();
    notifLayout->addLayout(soundBtnRow);

    // Enable/disable sound controls based on checkbox
    auto updateSoundControls = [=]() {
        bool enabled = m_notificationSoundCheck->isChecked();
        m_soundPathEdit->setEnabled(enabled);
        browseSoundBtn->setEnabled(enabled);
        testSoundBtn->setEnabled(enabled);
        resetSoundBtn->setEnabled(enabled);
    };
    connect(m_notificationSoundCheck, &QCheckBox::toggled, this, updateSoundControls);

    connect(browseSoundBtn, &QPushButton::clicked, this, [this]() {
        QString file = QFileDialog::getOpenFileName(this, tr("Select Sound File"),
            QString(), tr("WAV files (*.wav)"));
        if (!file.isEmpty()) {
            m_soundPathEdit->setText(file);
        }
    });

    connect(testSoundBtn, &QPushButton::clicked, this, [this]() {
        QUrl soundUrl;
        QString path = m_soundPathEdit->text().trimmed();
        if (path.isEmpty()) {
            soundUrl = QUrl("qrc:/assets/sounds/alert.wav");
        } else {
            if (!QFile::exists(path)) {
                QMessageBox::warning(this, tr("Sound Error"),
                    tr("File not found: %1").arg(path));
                return;
            }
            if (!path.endsWith(".wav", Qt::CaseInsensitive)) {
                QMessageBox::warning(this, tr("Sound Error"),
                    tr("Only .wav files are supported."));
                return;
            }
            soundUrl = QUrl::fromLocalFile(path);
        }
        QSoundEffect* sound = new QSoundEffect(this);
        sound->setSource(soundUrl);
        sound->play();
        connect(sound, &QSoundEffect::playingChanged, this, [sound]() {
            if (!sound->isPlaying()) sound->deleteLater();
        });
    });

    connect(resetSoundBtn, &QPushButton::clicked, this, [this]() {
        m_soundPathEdit->clear();
    });

    layout->addWidget(notifGroup);

    // -- Polling ------------------------------------------------------------------
    QGroupBox* pollingGroup = new QGroupBox(tr("Polling"), page);
    QFormLayout* pollingForm = new QFormLayout(pollingGroup);
    pollingForm->setContentsMargins(10, 8, 10, 8);
    m_defaultInterval = new QSpinBox(page);
    m_defaultInterval->setRange(30, 86400);
    m_defaultInterval->setSuffix(tr(" sec"));
    m_defaultInterval->setValue(3600);
    m_defaultInterval->setToolTip(tr("Default interval for new products (30s – 24h)"));
    pollingForm->addRow(tr("Default Check Interval:"), m_defaultInterval);
    layout->addWidget(pollingGroup);

    // -- Language -----------------------------------------------------------------
    QGroupBox* langGroup = new QGroupBox(tr("Language"), page);
    QFormLayout* langForm = new QFormLayout(langGroup);
    langForm->setContentsMargins(10, 8, 10, 8);
    m_languageCombo = new QComboBox(page);
    m_languageCombo->setToolTip(tr("Application display language (requires restart)"));
    m_languageCombo->addItem("English", "en");
    m_languageCombo->addItem(QString::fromUtf8("\330\247\331\204\330\271\330\261\330\250\331\212\330\251"), "ar");
    m_languageCombo->addItem(QString::fromUtf8("Fran\303\247ais"), "fr");
    m_languageLabel = new QLabel(page);
    langForm->addRow(tr("Language:"), m_languageCombo);
    layout->addWidget(langGroup);

    layout->addStretch();
    return scrollWrap(page);
}

QWidget* SettingsDialog::createApiKeysTab() {
    QWidget* page = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(page);
    layout->setContentsMargins(10, 14, 10, 10);
    layout->setSpacing(12);

    // -- Udemy --------------------------------------------------------------------
    QGroupBox* udemyGroup = new QGroupBox(tr("Udemy API Credentials"), page);
    QFormLayout* udemyForm = new QFormLayout(udemyGroup);
    udemyForm->setContentsMargins(10, 8, 10, 8);
    udemyForm->setSpacing(8);
    m_udemyClientId = new QLineEdit(page);
    m_udemyClientId->setToolTip(tr("Required for Udemy price tracking"));
    m_udemyClientSecret = new QLineEdit(page);
    m_udemyClientSecret->setEchoMode(QLineEdit::Password);
    m_udemyClientSecret->setToolTip(tr("Required for Udemy price tracking"));
    udemyForm->addRow(tr("Client ID:"),     m_udemyClientId);
    udemyForm->addRow(tr("Client Secret:"), m_udemyClientSecret);
    layout->addWidget(udemyGroup);

    // -- Amazon -------------------------------------------------------------------
    QGroupBox* amazonGroup = new QGroupBox(tr("Amazon PA API Credentials"), page);
    QFormLayout* amazonForm = new QFormLayout(amazonGroup);
    amazonForm->setContentsMargins(10, 8, 10, 8);
    amazonForm->setSpacing(8);
    m_amazonAccessKey = new QLineEdit(page);
    m_amazonAccessKey->setToolTip(tr("Required for Amazon price tracking"));
    m_amazonSecretKey = new QLineEdit(page);
    m_amazonSecretKey->setEchoMode(QLineEdit::Password);
    m_amazonSecretKey->setToolTip(tr("Required for Amazon price tracking"));
    m_amazonPartnerTag = new QLineEdit(page);
    m_amazonPartnerTag->setToolTip(tr("Required for Amazon price tracking"));
    amazonForm->addRow(tr("Access Key:"),  m_amazonAccessKey);
    amazonForm->addRow(tr("Secret Key:"),  m_amazonSecretKey);
    amazonForm->addRow(tr("Partner Tag:"), m_amazonPartnerTag);
    layout->addWidget(amazonGroup);

    layout->addStretch();
    return scrollWrap(page);
}

QWidget* SettingsDialog::createPluginsTab() {
    QWidget* page = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(page);
    layout->setContentsMargins(10, 14, 10, 10);
    layout->setSpacing(12);

    // -- Plugin directory ---------------------------------------------------------
    QGroupBox* dirGroup = new QGroupBox(tr("Plugin Directory"), page);
    QVBoxLayout* dirGroupLayout = new QVBoxLayout(dirGroup);
    dirGroupLayout->setContentsMargins(10, 8, 10, 8);
    dirGroupLayout->setSpacing(6);

    QHBoxLayout* dirRow = new QHBoxLayout();
    dirRow->setSpacing(4);
    m_pluginDir = new QLineEdit(page);
    m_pluginDir->setPlaceholderText(tr("plugins/"));
    m_pluginDir->setToolTip(tr("Directory to scan for native price handler plugins"));
    QPushButton* browseBtn = new QPushButton(tr("Browse"), page);
    browseBtn->setToolTip(tr("Choose plugin directory"));
    browseBtn->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    browseBtn->setMaximumHeight(26);
    dirRow->addWidget(m_pluginDir, 1);
    dirRow->addWidget(browseBtn);
    dirGroupLayout->addLayout(dirRow);
    layout->addWidget(dirGroup);

    connect(browseBtn, &QPushButton::clicked, this, [this]() {
        QString dir = QFileDialog::getExistingDirectory(this, tr("Select Plugin Directory"));
        if (!dir.isEmpty()) m_pluginDir->setText(dir);
    });

    // -- Loaded handlers ----------------------------------------------------------
    QGroupBox* handlersGroup = new QGroupBox(tr("Registered Handlers"), page);
    QVBoxLayout* handlersLayout = new QVBoxLayout(handlersGroup);
    handlersLayout->setContentsMargins(6, 8, 6, 8);
    handlersLayout->setSpacing(0);

    auto addHandlerRow = [&](const QString& iconPath, const QString& name,
                             const QString& id, const QString& badge) {
        QFrame* row = new QFrame(page);
        row->setStyleSheet(
            "QFrame { background: #313244; border-radius: 6px; }"
            "QFrame:hover { background: #45475a; }");
        row->setFixedHeight(44);

        QHBoxLayout* rowLayout = new QHBoxLayout(row);
        rowLayout->setContentsMargins(10, 4, 10, 4);
        rowLayout->setSpacing(10);

        // Icon
        QLabel* icon = new QLabel(page);
        icon->setPixmap(QIcon(iconPath).pixmap(20, 20));
        icon->setFixedSize(20, 20);
        rowLayout->addWidget(icon);

        // Name + ID stacked
        QVBoxLayout* textCol = new QVBoxLayout();
        textCol->setSpacing(0);
        textCol->setContentsMargins(0, 0, 0, 0);
        QLabel* nameLabel = new QLabel(name, page);
        nameLabel->setStyleSheet("font-weight: bold; font-size: 12px; color: #cdd6f4; background: transparent;");
        QLabel* idLabel = new QLabel(id, page);
        idLabel->setStyleSheet("font-size: 10px; color: #6c7086; background: transparent;");
        textCol->addWidget(nameLabel);
        textCol->addWidget(idLabel);
        rowLayout->addLayout(textCol, 1);

        // Badge
        QLabel* badgeLabel = new QLabel(badge, page);
        badgeLabel->setStyleSheet(
            "background: #45475a; color: #a6adc8; border-radius: 4px;"
            " padding: 2px 8px; font-size: 10px;");
        badgeLabel->setFixedHeight(18);
        rowLayout->addWidget(badgeLabel);

        handlersLayout->addWidget(row);
        // Spacer between rows
        handlersLayout->addSpacing(4);
    };

    if (!m_pluginManager) {
        QLabel* empty = new QLabel(tr("No handler information available"), page);
        empty->setAlignment(Qt::AlignCenter);
        empty->setStyleSheet("color: #6c7086; padding: 20px;");
        handlersLayout->addWidget(empty);
    } else {
        auto sources = m_pluginManager->availableSources();
        if (sources.empty()) {
            QLabel* empty = new QLabel(tr("No handlers registered"), page);
            empty->setAlignment(Qt::AlignCenter);
            empty->setStyleSheet("color: #6c7086; padding: 20px;");
            handlersLayout->addWidget(empty);
        } else {
            for (const auto& src : sources) {
                QString name = QString::fromStdString(src.name);
                QString id   = QString::fromStdString(src.id);
                QString badge = src.isDeveloperPlugin ? tr("Plugin") : tr("Built-in");

                // Pick source icon
                QString iconPath = ":/assets/icons/source_generic.svg";
                if (id == "steam")       iconPath = ":/assets/icons/source_steam.svg";
                else if (id == "udemy")  iconPath = ":/assets/icons/source_udemy.svg";
                else if (id == "amazon") iconPath = ":/assets/icons/source_amazon.svg";
                else if (src.isDeveloperPlugin) iconPath = ":/assets/icons/source_plugin.svg";

                addHandlerRow(iconPath, name, id, badge);

                // For IPlugin2 plugins, embed their settings widget
                if (src.isDeveloperPlugin) {
                    IPriceHandler* handler = m_pluginManager->handlerFor(src.id);
                    IPlugin2* p2 = dynamic_cast<IPlugin2*>(handler);
                    if (p2) {
                        QWidget* sw = p2->settingsWidget();
                        if (sw) {
                            sw->setParent(this);
                            handlersLayout->addWidget(sw);
                            handlersLayout->addSpacing(4);
                        }
                    }
                }
            }
        }
    }

    // Handler count footer
    if (m_pluginManager) {
        auto sources = m_pluginManager->availableSources();
        int builtIn = 0, plugins = 0;
        for (const auto& s : sources) {
            if (s.isDeveloperPlugin) ++plugins; else ++builtIn;
        }
        QLabel* footer = new QLabel(
            tr("%1 built-in, %2 plugin(s)").arg(builtIn).arg(plugins), page);
        footer->setStyleSheet("color: #6c7086; font-size: 10px; padding-top: 4px;");
        footer->setAlignment(Qt::AlignRight);
        handlersLayout->addWidget(footer);
    }

    layout->addWidget(handlersGroup);

    layout->addStretch();
    return scrollWrap(page);
}

void SettingsDialog::loadSettings() {
    auto& sp = SettingsProvider::instance();

    m_udemyClientId->setText(sp.udemyClientId());
    m_udemyClientSecret->setText(sp.udemyClientSecret());
    m_amazonAccessKey->setText(sp.amazonAccessKey());
    m_amazonSecretKey->setText(sp.amazonSecretKey());
    m_amazonPartnerTag->setText(sp.amazonPartnerTag());
    m_defaultInterval->setValue(sp.defaultInterval());
    m_pluginDir->setText(sp.pluginDirectory());
    m_notificationSoundCheck->setChecked(sp.notificationSoundEnabled());
    m_soundPathEdit->setText(sp.notificationSoundPath());
    m_soundPathEdit->setEnabled(sp.notificationSoundEnabled());

    QString lang = sp.language();
    int langIdx = m_languageCombo->findData(lang);
    if (langIdx >= 0) m_languageCombo->setCurrentIndex(langIdx);

    m_autoStartCheck->setChecked(AutoStartManager::isEnabled());
    m_launchMinimizedCheck->setChecked(sp.launchMinimized());
    m_autoUpdateCheck->setChecked(sp.checkUpdatesOnStartup());
}

void SettingsDialog::saveSettings() {
    auto& sp = SettingsProvider::instance();

    sp.setUdemyClientId(m_udemyClientId->text());
    sp.setUdemyClientSecret(m_udemyClientSecret->text());
    sp.setAmazonAccessKey(m_amazonAccessKey->text());
    sp.setAmazonSecretKey(m_amazonSecretKey->text());
    sp.setAmazonPartnerTag(m_amazonPartnerTag->text());
    sp.setDefaultInterval(m_defaultInterval->value());
    sp.setPluginDirectory(m_pluginDir->text());
    sp.setCheckUpdatesOnStartup(m_autoUpdateCheck->isChecked());
    sp.setNotificationSoundEnabled(m_notificationSoundCheck->isChecked());

    // Validate and save custom sound path
    QString soundPath = m_soundPathEdit->text().trimmed();
    if (!soundPath.isEmpty()) {
        if (!QFile::exists(soundPath)) {
            QMessageBox::warning(this, tr("Sound Error"),
                tr("Sound file not found: %1").arg(soundPath));
            return;
        }
        if (!soundPath.endsWith(".wav", Qt::CaseInsensitive)) {
            QMessageBox::warning(this, tr("Sound Error"),
                tr("Only .wav files are supported."));
            return;
        }
    }
    sp.setNotificationSoundPath(soundPath);

    QString newLang = m_languageCombo->currentData().toString();
    QString oldLang = sp.language();
    sp.setLanguage(newLang);

    if (newLang != oldLang) {
        m_restartNeeded = true;
    }

    bool wantAutoStart = m_autoStartCheck->isChecked();
    if (wantAutoStart != AutoStartManager::isEnabled()) {
        if (!AutoStartManager::setEnabled(wantAutoStart)) {
            QMessageBox::warning(this, tr("Auto-start"),
                tr("Failed to update auto-start setting."));
        }
    }
    sp.setLaunchMinimized(m_launchMinimizedCheck->isChecked());

    accept();
}
