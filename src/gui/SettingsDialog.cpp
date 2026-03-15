#include "gui/SettingsDialog.hpp"
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

SettingsDialog::SettingsDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Settings"));
    resize(480, 560);
    setupUi();
    loadSettings();
}

void SettingsDialog::setupUi() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(12, 14, 12, 12);

    // -- Startup ------------------------------------------------------------------
    QGroupBox* startupGroup = new QGroupBox(tr("Startup"), this);
    QVBoxLayout* startupLayout = new QVBoxLayout(startupGroup);
    m_autoStartCheck = new QCheckBox(tr("Open on Startup"), this);
    m_autoStartCheck->setToolTip(tr("Automatically launch PriceBell when you log in"));
    startupLayout->addWidget(m_autoStartCheck);
    mainLayout->addWidget(startupGroup);

    // -- Updates ------------------------------------------------------------------
    QGroupBox* updateGroup = new QGroupBox(tr("Updates"), this);
    QVBoxLayout* updateLayout = new QVBoxLayout(updateGroup);
    m_autoUpdateCheck = new QCheckBox(tr("Check for updates on startup"), this);
    m_autoUpdateCheck->setToolTip(tr("Automatically check for new versions when the app starts"));
    updateLayout->addWidget(m_autoUpdateCheck);
    mainLayout->addWidget(updateGroup);

    // -- Notifications ------------------------------------------------------------
    QGroupBox* notifGroup = new QGroupBox(tr("Notifications"), this);
    QVBoxLayout* notifLayout = new QVBoxLayout(notifGroup);
    m_notificationSoundCheck = new QCheckBox(tr("Play notification sound"), this);
    m_notificationSoundCheck->setToolTip(tr("Play a sound when a price alert is triggered"));
    notifLayout->addWidget(m_notificationSoundCheck);
    mainLayout->addWidget(notifGroup);

    // -- Udemy --------------------------------------------------------------------
    QGroupBox* udemyGroup = new QGroupBox(tr("Udemy API Credentials"), this);
    QFormLayout* udemyForm = new QFormLayout(udemyGroup);
    m_udemyClientId     = new QLineEdit(this);
    m_udemyClientId->setToolTip(tr("Required for Udemy price tracking"));
    m_udemyClientSecret = new QLineEdit(this);
    m_udemyClientSecret->setEchoMode(QLineEdit::Password);
    m_udemyClientSecret->setToolTip(tr("Required for Udemy price tracking"));
    udemyForm->addRow(tr("Client ID:"),     m_udemyClientId);
    udemyForm->addRow(tr("Client Secret:"), m_udemyClientSecret);
    mainLayout->addWidget(udemyGroup);

    // -- Amazon -------------------------------------------------------------------
    QGroupBox* amazonGroup = new QGroupBox(tr("Amazon PA API Credentials"), this);
    QFormLayout* amazonForm = new QFormLayout(amazonGroup);
    m_amazonAccessKey  = new QLineEdit(this);
    m_amazonAccessKey->setToolTip(tr("Required for Amazon price tracking"));
    m_amazonSecretKey  = new QLineEdit(this);
    m_amazonSecretKey->setEchoMode(QLineEdit::Password);
    m_amazonSecretKey->setToolTip(tr("Required for Amazon price tracking"));
    m_amazonPartnerTag = new QLineEdit(this);
    m_amazonPartnerTag->setToolTip(tr("Required for Amazon price tracking"));
    amazonForm->addRow(tr("Access Key:"),  m_amazonAccessKey);
    amazonForm->addRow(tr("Secret Key:"),  m_amazonSecretKey);
    amazonForm->addRow(tr("Partner Tag:"), m_amazonPartnerTag);
    mainLayout->addWidget(amazonGroup);

    // -- Polling ------------------------------------------------------------------
    QGroupBox* pollingGroup = new QGroupBox(tr("Polling"), this);
    QFormLayout* pollingForm = new QFormLayout(pollingGroup);
    m_defaultInterval = new QSpinBox(this);
    m_defaultInterval->setRange(30, 86400);
    m_defaultInterval->setSuffix(tr(" sec"));
    m_defaultInterval->setValue(3600);
    m_defaultInterval->setToolTip(tr("Default interval for new products (30s – 24h)"));
    pollingForm->addRow(tr("Default Check Interval:"), m_defaultInterval);
    mainLayout->addWidget(pollingGroup);

    // -- Plugin directory ---------------------------------------------------------
    QGroupBox* pluginGroup = new QGroupBox(tr("Plugin Directory"), this);
    QHBoxLayout* pluginLayout = new QHBoxLayout(pluginGroup);
    m_pluginDir = new QLineEdit(this);
    m_pluginDir->setToolTip(tr("Directory to scan for native price handler plugins"));
    QPushButton* browseBtn = new QPushButton(tr("Browse…"), this);
    browseBtn->setToolTip(tr("Choose plugin directory"));
    pluginLayout->addWidget(m_pluginDir);
    pluginLayout->addWidget(browseBtn);
    mainLayout->addWidget(pluginGroup);
    connect(browseBtn, &QPushButton::clicked, this, [this]() {
        QString dir = QFileDialog::getExistingDirectory(this, tr("Select Plugin Directory"));
        if (!dir.isEmpty()) m_pluginDir->setText(dir);
    });

    // -- Language -----------------------------------------------------------------
    QGroupBox* langGroup = new QGroupBox(tr("Language"), this);
    QFormLayout* langForm = new QFormLayout(langGroup);
    m_languageCombo = new QComboBox(this);
    m_languageCombo->setToolTip(tr("Application display language (requires restart)"));
    m_languageCombo->addItem("English", "en");
    m_languageCombo->addItem(QString::fromUtf8("\330\247\331\204\330\271\330\261\330\250\331\212\330\251"), "ar");
    m_languageCombo->addItem(QString::fromUtf8("Fran\303\247ais"), "fr");
    m_languageLabel = new QLabel(this);
    langForm->addRow(tr("Language:"), m_languageCombo);
    mainLayout->addWidget(langGroup);

    // -- Buttons ------------------------------------------------------------------
    QDialogButtonBox* buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    mainLayout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, this, &SettingsDialog::saveSettings);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
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
