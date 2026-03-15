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

SettingsDialog::SettingsDialog(QWidget* parent, PluginManager* pluginManager)
    : QDialog(parent), m_pluginManager(pluginManager)
{
    setWindowTitle(tr("Settings"));
    resize(520, 480);
    setupUi();
    loadSettings();
}

void SettingsDialog::setupUi() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(12, 14, 12, 12);

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
    layout->setContentsMargins(8, 18, 8, 8);

    // -- Startup ------------------------------------------------------------------
    QGroupBox* startupGroup = new QGroupBox(tr("Startup"), page);
    QVBoxLayout* startupLayout = new QVBoxLayout(startupGroup);
    m_autoStartCheck = new QCheckBox(tr("Open on Startup"), page);
    m_autoStartCheck->setToolTip(tr("Automatically launch PriceBell when you log in"));
    startupLayout->addWidget(m_autoStartCheck);
    layout->addWidget(startupGroup);

    // -- Updates ------------------------------------------------------------------
    QGroupBox* updateGroup = new QGroupBox(tr("Updates"), page);
    QVBoxLayout* updateLayout = new QVBoxLayout(updateGroup);
    m_autoUpdateCheck = new QCheckBox(tr("Check for updates on startup"), page);
    m_autoUpdateCheck->setToolTip(tr("Automatically check for new versions when the app starts"));
    updateLayout->addWidget(m_autoUpdateCheck);
    layout->addWidget(updateGroup);

    // -- Notifications ------------------------------------------------------------
    QGroupBox* notifGroup = new QGroupBox(tr("Notifications"), page);
    QVBoxLayout* notifLayout = new QVBoxLayout(notifGroup);
    m_notificationSoundCheck = new QCheckBox(tr("Play notification sound"), page);
    m_notificationSoundCheck->setToolTip(tr("Play a sound when a price alert is triggered"));
    notifLayout->addWidget(m_notificationSoundCheck);
    layout->addWidget(notifGroup);

    // -- Polling ------------------------------------------------------------------
    QGroupBox* pollingGroup = new QGroupBox(tr("Polling"), page);
    QFormLayout* pollingForm = new QFormLayout(pollingGroup);
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
    m_languageCombo = new QComboBox(page);
    m_languageCombo->setToolTip(tr("Application display language (requires restart)"));
    m_languageCombo->addItem("English", "en");
    m_languageCombo->addItem(QString::fromUtf8("\330\247\331\204\330\271\330\261\330\250\331\212\330\251"), "ar");
    m_languageCombo->addItem(QString::fromUtf8("Fran\303\247ais"), "fr");
    m_languageLabel = new QLabel(page);
    langForm->addRow(tr("Language:"), m_languageCombo);
    layout->addWidget(langGroup);

    layout->addStretch();
    return page;
}

QWidget* SettingsDialog::createApiKeysTab() {
    QWidget* page = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(page);
    layout->setContentsMargins(8, 18, 8, 8);

    // -- Udemy --------------------------------------------------------------------
    QGroupBox* udemyGroup = new QGroupBox(tr("Udemy API Credentials"), page);
    QFormLayout* udemyForm = new QFormLayout(udemyGroup);
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
    return page;
}

QWidget* SettingsDialog::createPluginsTab() {
    QWidget* page = new QWidget(this);
    QVBoxLayout* layout = new QVBoxLayout(page);
    layout->setContentsMargins(8, 18, 8, 8);

    // -- Plugin directory ---------------------------------------------------------
    QGroupBox* dirGroup = new QGroupBox(tr("Plugin Directory"), page);
    QHBoxLayout* dirLayout = new QHBoxLayout(dirGroup);
    m_pluginDir = new QLineEdit(page);
    m_pluginDir->setToolTip(tr("Directory to scan for native price handler plugins"));
    QPushButton* browseBtn = new QPushButton(tr("Browse…"), page);
    browseBtn->setToolTip(tr("Choose plugin directory"));
    dirLayout->addWidget(m_pluginDir);
    dirLayout->addWidget(browseBtn);
    layout->addWidget(dirGroup);
    connect(browseBtn, &QPushButton::clicked, this, [this]() {
        QString dir = QFileDialog::getExistingDirectory(this, tr("Select Plugin Directory"));
        if (!dir.isEmpty()) m_pluginDir->setText(dir);
    });

    // -- Loaded plugins -----------------------------------------------------------
    QGroupBox* pluginsGroup = new QGroupBox(tr("Loaded Plugins"), page);
    QVBoxLayout* pluginsLayout = new QVBoxLayout(pluginsGroup);

    if (!m_pluginManager) {
        QLabel* noAccess = new QLabel(tr("No plugins loaded"), page);
        noAccess->setAlignment(Qt::AlignCenter);
        pluginsLayout->addWidget(noAccess);
    } else {
        auto sources = m_pluginManager->availableSources();
        if (sources.empty()) {
            QLabel* emptyLabel = new QLabel(tr("No plugins loaded"), page);
            emptyLabel->setAlignment(Qt::AlignCenter);
            pluginsLayout->addWidget(emptyLabel);
        } else {
            for (const auto& src : sources) {
                QString name = QString::fromStdString(src.name);
                QString id   = QString::fromStdString(src.id);
                QString type = src.isDeveloperPlugin ? tr("Plugin") : tr("Built-in");

                QGroupBox* entry = new QGroupBox(name, page);
                QVBoxLayout* entryLayout = new QVBoxLayout(entry);

                QLabel* info = new QLabel(
                    tr("ID: %1  |  Type: %2").arg(id, type), page);
                entryLayout->addWidget(info);

                // For IPlugin2 plugins, embed their settings widget if available
                if (src.isDeveloperPlugin) {
                    IPriceHandler* handler = m_pluginManager->handlerFor(src.id);
                    IPlugin2* p2 = dynamic_cast<IPlugin2*>(handler);
                    if (p2) {
                        QWidget* sw = p2->settingsWidget();
                        if (sw) {
                            sw->setParent(this);
                            entryLayout->addWidget(sw);
                        }
                    }
                }

                pluginsLayout->addWidget(entry);
            }
        }
    }
    layout->addWidget(pluginsGroup);

    layout->addStretch();
    return page;
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

    QString lang = sp.language();
    int langIdx = m_languageCombo->findData(lang);
    if (langIdx >= 0) m_languageCombo->setCurrentIndex(langIdx);

    m_autoStartCheck->setChecked(AutoStartManager::isEnabled());
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

    accept();
}
