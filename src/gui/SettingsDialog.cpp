#include "gui/SettingsDialog.hpp"
#include "utils/AutoStartManager.hpp"

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
    resize(480, 520);
    setupUi();
    loadSettings();
}

void SettingsDialog::setupUi() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(12, 14, 12, 12);

    // ── Startup ──────────────────────────────────────────────────────────────
    QGroupBox* startupGroup = new QGroupBox(tr("Startup"), this);
    QVBoxLayout* startupLayout = new QVBoxLayout(startupGroup);
    m_autoStartCheck = new QCheckBox(tr("Open on Startup"), this);
    startupLayout->addWidget(m_autoStartCheck);
    mainLayout->addWidget(startupGroup);

    // ── Udemy ─────────────────────────────────────────────────────────────────
    QGroupBox* udemyGroup = new QGroupBox(tr("Udemy API Credentials"), this);
    QFormLayout* udemyForm = new QFormLayout(udemyGroup);
    m_udemyClientId     = new QLineEdit(this);
    m_udemyClientSecret = new QLineEdit(this);
    m_udemyClientSecret->setEchoMode(QLineEdit::Password);
    udemyForm->addRow(tr("Client ID:"),     m_udemyClientId);
    udemyForm->addRow(tr("Client Secret:"), m_udemyClientSecret);
    mainLayout->addWidget(udemyGroup);

    // ── Amazon ────────────────────────────────────────────────────────────────
    QGroupBox* amazonGroup = new QGroupBox(tr("Amazon PA API Credentials"), this);
    QFormLayout* amazonForm = new QFormLayout(amazonGroup);
    m_amazonAccessKey  = new QLineEdit(this);
    m_amazonSecretKey  = new QLineEdit(this);
    m_amazonSecretKey->setEchoMode(QLineEdit::Password);
    m_amazonPartnerTag = new QLineEdit(this);
    amazonForm->addRow(tr("Access Key:"),  m_amazonAccessKey);
    amazonForm->addRow(tr("Secret Key:"),  m_amazonSecretKey);
    amazonForm->addRow(tr("Partner Tag:"), m_amazonPartnerTag);
    mainLayout->addWidget(amazonGroup);

    // ── Polling ───────────────────────────────────────────────────────────────
    QGroupBox* pollingGroup = new QGroupBox(tr("Polling"), this);
    QFormLayout* pollingForm = new QFormLayout(pollingGroup);
    m_defaultInterval = new QSpinBox(this);
    m_defaultInterval->setRange(30, 86400);
    m_defaultInterval->setSuffix(tr(" sec"));
    m_defaultInterval->setValue(3600);
    pollingForm->addRow(tr("Default Check Interval:"), m_defaultInterval);
    mainLayout->addWidget(pollingGroup);

    // ── Plugin directory ──────────────────────────────────────────────────────
    QGroupBox* pluginGroup = new QGroupBox(tr("Plugin Directory"), this);
    QHBoxLayout* pluginLayout = new QHBoxLayout(pluginGroup);
    m_pluginDir = new QLineEdit(this);
    QPushButton* browseBtn = new QPushButton(tr("Browse…"), this);
    pluginLayout->addWidget(m_pluginDir);
    pluginLayout->addWidget(browseBtn);
    mainLayout->addWidget(pluginGroup);
    connect(browseBtn, &QPushButton::clicked, this, [this]() {
        QString dir = QFileDialog::getExistingDirectory(this, tr("Select Plugin Directory"));
        if (!dir.isEmpty()) m_pluginDir->setText(dir);
    });

    // ── Language ──────────────────────────────────────────────────────────────
    QGroupBox* langGroup = new QGroupBox(tr("Language"), this);
    QFormLayout* langForm = new QFormLayout(langGroup);
    m_languageCombo = new QComboBox(this);
    m_languageCombo->addItem("English", "en");
    m_languageCombo->addItem(QString::fromUtf8("\330\247\331\204\330\271\330\261\330\250\331\212\330\251"), "ar");
    m_languageCombo->addItem(QString::fromUtf8("Fran\303\247ais"), "fr");
    m_languageLabel = new QLabel(this);
    langForm->addRow(tr("Language:"), m_languageCombo);
    mainLayout->addWidget(langGroup);

    // ── Buttons ───────────────────────────────────────────────────────────────
    QDialogButtonBox* buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    mainLayout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, this, &SettingsDialog::saveSettings);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void SettingsDialog::loadSettings() {
    QSettings s("PriceBell", "PriceBell");
    m_udemyClientId->setText(s.value("udemy/client_id").toString());
    m_udemyClientSecret->setText(s.value("udemy/client_secret").toString());
    m_amazonAccessKey->setText(s.value("amazon/access_key").toString());
    m_amazonSecretKey->setText(s.value("amazon/secret_key").toString());
    m_amazonPartnerTag->setText(s.value("amazon/partner_tag").toString());
    m_defaultInterval->setValue(s.value("polling/default_interval", 3600).toInt());
    m_pluginDir->setText(s.value("plugins/directory",
        QApplication::applicationDirPath() + "/plugins").toString());

    QString lang = s.value("language", "en").toString();
    int langIdx = m_languageCombo->findData(lang);
    if (langIdx >= 0) m_languageCombo->setCurrentIndex(langIdx);

    m_autoStartCheck->setChecked(AutoStartManager::isEnabled());
}

void SettingsDialog::saveSettings() {
    QSettings s("PriceBell", "PriceBell");
    s.setValue("udemy/client_id",           m_udemyClientId->text());
    s.setValue("udemy/client_secret",       m_udemyClientSecret->text());
    s.setValue("amazon/access_key",         m_amazonAccessKey->text());
    s.setValue("amazon/secret_key",         m_amazonSecretKey->text());
    s.setValue("amazon/partner_tag",        m_amazonPartnerTag->text());
    s.setValue("polling/default_interval",  m_defaultInterval->value());
    s.setValue("plugins/directory",         m_pluginDir->text());

    QString newLang = m_languageCombo->currentData().toString();
    QString oldLang = s.value("language", "en").toString();
    s.setValue("language", newLang);

    if (newLang != oldLang) {
        QMessageBox::information(this, tr("Language"),
            tr("Language change will take effect after restarting PriceBell."));
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
