#pragma once

#include "gui/FeatureAnnouncement.hpp"

#include <QList>
#include <QObject>
#include <QPointer>
#include <QString>

class QWidget;
class AnnouncementToast;

// Surfaces one-time feature-introduction toasts after an upgrade.
//
// Usage in MainWindow:
//   m_announcements = new AnnouncementCenter(this);
//   m_announcements->setAnchor(this);
//   m_announcements->registerDefaults();
//   connect(m_announcements, &AnnouncementCenter::primaryActionTriggered,
//           this, &MainWindow::onAnnouncementAction);
//   QTimer::singleShot(1500, m_announcements, &AnnouncementCenter::showPending);
//
// Filtering rules:
//   - Skip if the user is on a fresh install (don't blast new users with
//     backlog announcements they never missed).
//   - Skip if SettingsProvider says the id is already seen.
//   - Skip if the announcement's minVersion > current APP_VERSION.
class AnnouncementCenter : public QObject {
    Q_OBJECT
public:
    explicit AnnouncementCenter(QObject* parent = nullptr);

    // Window the toasts attach to (positioned bottom-right of this widget).
    void setAnchor(QWidget* anchor) { m_anchor = anchor; }

    // Append one announcement to the queue (rarely needed — defaults are
    // populated by registerDefaults()).
    void registerAnnouncement(const FeatureAnnouncement& a);

    // Populate the v1.4.0 announcements (and any future ones bundled here).
    void registerDefaults();

    // Show all unseen announcements one at a time, oldest minVersion first.
    // Safe to call before/after the main window is shown.
    void showPending();

signals:
    // primaryActionId values are recognised by MainWindow:
    //   "switch_to_cards", "open_add_product", "open_settings"
    void primaryActionTriggered(QString actionId);

private slots:
    // Slot so QMetaObject::invokeMethod can chain to the next toast safely
    // across the queued connection from a closing toast.
    void showNext();

private:

    QList<FeatureAnnouncement>   m_queue;
    QPointer<QWidget>            m_anchor;
    QPointer<AnnouncementToast>  m_currentToast;
};
