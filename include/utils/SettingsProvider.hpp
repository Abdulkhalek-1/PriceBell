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
    bool launchMinimized() const;
    void setLaunchMinimized(bool enabled);
    bool wasFullscreen() const;
    void setWasFullscreen(bool fullscreen);

    // Steam region (ISO-3166 alpha-2). Empty = "auto" → derive from system locale.
    QString steamCountryCode() const;
    void    setSteamCountryCode(const QString& code);

    // Layout: "cards" or "table". Default decided per-call (fresh installs get
    // "cards", upgraders default to "table" so the UI doesn't shift under them).
    enum class LayoutMode { Cards, Table };
    LayoutMode layoutMode() const;
    void       setLayoutMode(LayoutMode mode);

    // Feature announcement tracking (per-install, persisted via QSettings).
    bool isAnnouncementSeen(const QString& id) const;
    void markAnnouncementSeen(const QString& id);
    // True before any announcement state has been recorded — used so brand-new
    // installs don't get blasted with backlog announcements from prior versions.
    // First call also stamps the firstSeenVersion key as a side effect.
    bool isFreshInstall();

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
    static constexpr const char* kLaunchMinimized       = "startup/launchMinimized";
    static constexpr const char* kWasFullscreen          = "window/wasFullscreen";
    static constexpr const char* kSteamCountryCode       = "steam/country_code";
    static constexpr const char* kLayoutMode             = "ui/layout_mode";
    static constexpr const char* kAnnouncementSeenPrefix = "announcements/seen/";
    static constexpr const char* kFirstSeenVersion       = "announcements/firstSeenVersion";
};
