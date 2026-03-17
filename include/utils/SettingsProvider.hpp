#pragma once
#include <QString>
#include <QSettings>

class SettingsProvider {
public:
    static SettingsProvider& instance();

    // Udemy
    QString udemyClientId() const;
    QString udemyClientSecret() const;
    void setUdemyClientId(const QString& id);
    void setUdemyClientSecret(const QString& secret);

    // Amazon
    QString amazonAccessKey() const;
    QString amazonSecretKey() const;
    QString amazonPartnerTag() const;
    void setAmazonAccessKey(const QString& key);
    void setAmazonSecretKey(const QString& key);
    void setAmazonPartnerTag(const QString& tag);

    // Polling
    int defaultInterval() const;
    void setDefaultInterval(int seconds);

    // Plugins
    QString pluginDirectory() const;
    void setPluginDirectory(const QString& dir);

    // UI
    QString language() const;
    void setLanguage(const QString& lang);
    bool notificationSoundEnabled() const;
    void setNotificationSoundEnabled(bool enabled);
    QString notificationSoundPath() const;
    void setNotificationSoundPath(const QString& path);
    bool checkUpdatesOnStartup() const;
    void setCheckUpdatesOnStartup(bool enabled);

private:
    SettingsProvider();
    QSettings m_settings;

    // Base64 encode/decode for credential storage
    QString encodeCredential(const QString& plain) const;
    QString decodeCredential(const QString& encoded) const;

    // Key constants
    static constexpr const char* kUdemyClientId     = "udemy/client_id";
    static constexpr const char* kUdemyClientSecret  = "udemy/client_secret";
    static constexpr const char* kAmazonAccessKey    = "amazon/access_key";
    static constexpr const char* kAmazonSecretKey    = "amazon/secret_key";
    static constexpr const char* kAmazonPartnerTag   = "amazon/partner_tag";
    static constexpr const char* kDefaultIntervalKey = "polling/default_interval";
    static constexpr const char* kPluginDirectory    = "plugins/directory";
    static constexpr const char* kLanguage           = "language";
    static constexpr const char* kNotificationSound     = "notifications/sound_enabled";
    static constexpr const char* kNotificationSoundPath = "notifications/sound_path";
    static constexpr const char* kCheckUpdates          = "updates/check_on_startup";
};
