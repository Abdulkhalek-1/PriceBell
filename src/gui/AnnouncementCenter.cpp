#include "gui/AnnouncementCenter.hpp"
#include "gui/AnnouncementToast.hpp"
#include "utils/SettingsProvider.hpp"

AnnouncementCenter::AnnouncementCenter(QObject* parent)
    : QObject(parent)
{}

void AnnouncementCenter::registerAnnouncement(const FeatureAnnouncement& a) {
    m_queue.append(a);
}

// Compare semantic version strings like "1.4.0". Returns true if a <= b.
// Naive split-on-dot is fine for our scheme (no pre-release suffixes).
static bool versionLessOrEqual(const QString& a, const QString& b) {
    QStringList ap = a.split('.');
    QStringList bp = b.split('.');
    int n = std::max(ap.size(), bp.size());
    for (int i = 0; i < n; ++i) {
        int av = i < ap.size() ? ap[i].toInt() : 0;
        int bv = i < bp.size() ? bp[i].toInt() : 0;
        if (av < bv) return true;
        if (av > bv) return false;
    }
    return true;
}

void AnnouncementCenter::registerDefaults() {
    // v1.4.0 features. Future versions append to this list — no other code
    // changes required, the SettingsProvider tracks seen-state per id.
    registerAnnouncement({
        QStringLiteral("v1.4.0_layout_switcher"),
        QStringLiteral("1.4.0"),
        tr("New: Card layout"),
        tr("PriceBell now has a friendlier card view in addition to the table. "
           "Switch any time from the layout button in the toolbar."),
        QStringLiteral(":/assets/icons/notification_bell.svg"),
        tr("Try it"),
        QStringLiteral("switch_to_cards"),
        tr("Got it"),
    });
    registerAnnouncement({
        QStringLiteral("v1.4.0_url_autodetect"),
        QStringLiteral("1.4.0"),
        tr("New: paste URL, get the rest"),
        tr("Pasting a product URL now auto-fills the source and product name. "
           "Try it from Add Product."),
        QStringLiteral(":/assets/icons/add.svg"),
        tr("Add product"),
        QStringLiteral("open_add_product"),
        tr("Got it"),
    });
    registerAnnouncement({
        QStringLiteral("v1.4.0_steam_region"),
        QStringLiteral("1.4.0"),
        tr("Steam region is now automatic"),
        tr("Steam prices now match your country's Steam store. Override it in "
           "Settings → General if needed."),
        QStringLiteral(":/assets/icons/source_steam.svg"),
        tr("Open settings"),
        QStringLiteral("open_settings"),
        tr("Got it"),
    });
}

void AnnouncementCenter::showPending() {
    auto& sp = SettingsProvider::instance();
    const QString currentVersion = QString::fromLatin1(APP_VERSION);

    // Calling this stamps firstSeenVersion the first time it returns true, so
    // we can use it as a proxy for "this is a brand-new install — don't blast
    // them with backlog announcements they never missed".
    bool fresh = sp.isFreshInstall();

    QList<FeatureAnnouncement> pending;
    for (const auto& a : m_queue) {
        if (sp.isAnnouncementSeen(a.id)) continue;
        if (!versionLessOrEqual(a.minVersion, currentVersion)) continue;
        if (fresh) {
            // Mark as seen without showing — fresh users get the polished
            // defaults; they don't need a tour of "what's new".
            sp.markAnnouncementSeen(a.id);
            continue;
        }
        pending.append(a);
    }
    m_queue = pending;
    showNext();
}

void AnnouncementCenter::showNext() {
    if (m_queue.isEmpty() || !m_anchor) return;

    FeatureAnnouncement a = m_queue.takeFirst();
    auto& sp = SettingsProvider::instance();
    sp.markAnnouncementSeen(a.id);   // mark first so the user can't get spammed by a crash loop

    auto* toast = new AnnouncementToast(a, m_anchor, m_anchor);
    m_currentToast = toast;
    connect(toast, &AnnouncementToast::primaryClicked,
            this, &AnnouncementCenter::primaryActionTriggered);
    connect(toast, &AnnouncementToast::dismissed, this, [this]() {
        // Show the next pending one after a short pause so they don't pile up.
        QMetaObject::invokeMethod(this, "showNext", Qt::QueuedConnection);
    });
    toast->showAnimated();
}
