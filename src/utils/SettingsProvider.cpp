#include "utils/SettingsProvider.hpp"

SettingsProvider& SettingsProvider::instance() {
    static SettingsProvider s;
    return s;
}

SettingsProvider::SettingsProvider()
    : m_settings("PriceBell", "PriceBell")
{}

// ── Credential helpers ───────────────────────────────────────────────────────

QString SettingsProvider::encodeCredential(const QString& plain) const {
    return QString::fromUtf8(plain.toUtf8().toBase64());
}

QString SettingsProvider::decodeCredential(const QString& encoded) const {
    return QString::fromUtf8(QByteArray::fromBase64(encoded.toUtf8()));
}

// ── Udemy ────────────────────────────────────────────────────────────────────

QString SettingsProvider::udemyClientId() const {
    return decodeCredential(m_settings.value(kUdemyClientId).toString());
}

QString SettingsProvider::udemyClientSecret() const {
    return decodeCredential(m_settings.value(kUdemyClientSecret).toString());
}

void SettingsProvider::setUdemyClientId(const QString& id) {
    m_settings.setValue(kUdemyClientId, encodeCredential(id));
}

void SettingsProvider::setUdemyClientSecret(const QString& secret) {
    m_settings.setValue(kUdemyClientSecret, encodeCredential(secret));
}

// ── Amazon ───────────────────────────────────────────────────────────────────

QString SettingsProvider::amazonAccessKey() const {
    return decodeCredential(m_settings.value(kAmazonAccessKey).toString());
}

QString SettingsProvider::amazonSecretKey() const {
    return decodeCredential(m_settings.value(kAmazonSecretKey).toString());
}

QString SettingsProvider::amazonPartnerTag() const {
    return m_settings.value(kAmazonPartnerTag).toString();
}

void SettingsProvider::setAmazonAccessKey(const QString& key) {
    m_settings.setValue(kAmazonAccessKey, encodeCredential(key));
}

void SettingsProvider::setAmazonSecretKey(const QString& key) {
    m_settings.setValue(kAmazonSecretKey, encodeCredential(key));
}

void SettingsProvider::setAmazonPartnerTag(const QString& tag) {
    m_settings.setValue(kAmazonPartnerTag, tag);
}

// ── Polling ──────────────────────────────────────────────────────────────────

int SettingsProvider::defaultInterval() const {
    return m_settings.value(kDefaultIntervalKey, 3600).toInt();
}

void SettingsProvider::setDefaultInterval(int seconds) {
    m_settings.setValue(kDefaultIntervalKey, seconds);
}

// ── Plugins ──────────────────────────────────────────────────────────────────

QString SettingsProvider::pluginDirectory() const {
    return m_settings.value(kPluginDirectory).toString();
}

void SettingsProvider::setPluginDirectory(const QString& dir) {
    m_settings.setValue(kPluginDirectory, dir);
}

// ── UI ───────────────────────────────────────────────────────────────────────

QString SettingsProvider::language() const {
    return m_settings.value(kLanguage, "en").toString();
}

void SettingsProvider::setLanguage(const QString& lang) {
    m_settings.setValue(kLanguage, lang);
}

bool SettingsProvider::notificationSoundEnabled() const {
    return m_settings.value(kNotificationSound, true).toBool();
}

void SettingsProvider::setNotificationSoundEnabled(bool enabled) {
    m_settings.setValue(kNotificationSound, enabled);
}

QString SettingsProvider::notificationSoundPath() const {
    return m_settings.value(kNotificationSoundPath).toString();
}

void SettingsProvider::setNotificationSoundPath(const QString& path) {
    m_settings.setValue(kNotificationSoundPath, path);
}

bool SettingsProvider::checkUpdatesOnStartup() const {
    return m_settings.value(kCheckUpdates, true).toBool();
}

void SettingsProvider::setCheckUpdatesOnStartup(bool enabled) {
    m_settings.setValue(kCheckUpdates, enabled);
}

bool SettingsProvider::launchMinimized() const {
    return m_settings.value(kLaunchMinimized, false).toBool();
}

void SettingsProvider::setLaunchMinimized(bool enabled) {
    m_settings.setValue(kLaunchMinimized, enabled);
}

bool SettingsProvider::wasFullscreen() const {
    return m_settings.value(kWasFullscreen, false).toBool();
}

void SettingsProvider::setWasFullscreen(bool fullscreen) {
    m_settings.setValue(kWasFullscreen, fullscreen);
}

// ── Steam region ─────────────────────────────────────────────────────────────

QString SettingsProvider::steamCountryCode() const {
    return m_settings.value(kSteamCountryCode).toString();
}

void SettingsProvider::setSteamCountryCode(const QString& code) {
    m_settings.setValue(kSteamCountryCode, code);
}

// ── Layout mode ──────────────────────────────────────────────────────────────

SettingsProvider::LayoutMode SettingsProvider::layoutMode() const {
    // Default differs by user type:
    //   fresh install  → cards
    //   upgraded user  → table (their existing layout)
    QString def = m_settings.contains(kFirstSeenVersion) ? "table" : "cards";
    QString v = m_settings.value(kLayoutMode, def).toString();
    return v == "table" ? LayoutMode::Table : LayoutMode::Cards;
}

void SettingsProvider::setLayoutMode(LayoutMode mode) {
    m_settings.setValue(kLayoutMode, mode == LayoutMode::Table ? "table" : "cards");
}

// ── Announcements ────────────────────────────────────────────────────────────

bool SettingsProvider::isAnnouncementSeen(const QString& id) const {
    return m_settings.value(QString(kAnnouncementSeenPrefix) + id, false).toBool();
}

void SettingsProvider::markAnnouncementSeen(const QString& id) {
    m_settings.setValue(QString(kAnnouncementSeenPrefix) + id, true);
    m_settings.sync();
}

bool SettingsProvider::isFreshInstall() {
    bool fresh = !m_settings.contains(kFirstSeenVersion);
    if (fresh) {
        m_settings.setValue(kFirstSeenVersion, QString::fromLatin1(APP_VERSION));
        m_settings.sync();
    }
    return fresh;
}
