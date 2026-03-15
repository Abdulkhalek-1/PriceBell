#pragma once

#include <QDialog>
#include <QCheckBox>
#include <QLineEdit>
#include <QSpinBox>
#include <QComboBox>
#include <QLabel>

class SettingsDialog : public QDialog {
    Q_OBJECT
public:
    explicit SettingsDialog(QWidget* parent = nullptr);
    bool isRestartNeeded() const { return m_restartNeeded; }

private slots:
    void saveSettings();

private:
    void setupUi();
    void loadSettings();

    bool m_restartNeeded = false;

    // Steam (no credentials needed — public API)
    // Udemy
    QLineEdit* m_udemyClientId;
    QLineEdit* m_udemyClientSecret;
    // Amazon PA API
    QLineEdit* m_amazonAccessKey;
    QLineEdit* m_amazonSecretKey;
    QLineEdit* m_amazonPartnerTag;
    // Polling
    QSpinBox*  m_defaultInterval;
    // Plugin directory
    QLineEdit* m_pluginDir;
    // Language
    QComboBox* m_languageCombo;
    QLabel*    m_languageLabel;
    // Open on Startup
    QCheckBox* m_autoStartCheck;
    // Updates
    QCheckBox* m_autoUpdateCheck;
};
