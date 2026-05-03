#pragma once

#include "core/DataStructs.hpp"
#include <QObject>
#include <QSet>

// Receives price-updated signals from PricePoller, evaluates conditions,
// persists alert events, and notifies the GUI via Qt signals.
//
// v1.4.0: notification deduplication. The DB always records every triggered
// event, but `alertTriggered` only fires the FIRST time a product matches in
// this session — so the user gets one tray notification per product per app
// launch instead of one per poll tick.
class AlertManager : public QObject {
    Q_OBJECT
public:
    explicit AlertManager(QObject* parent = nullptr);

public slots:
    // Called by PricePoller when a fresh FetchResult arrives for a product.
    void onPriceUpdated(Product product, FetchResult result);

    // Re-arm notifications for a single product. Called when the user dismisses
    // the alert in AlertHistoryDialog or edits the product's conditions.
    void resetNotificationFor(int productId);

    // Re-arm all products. Useful for manual "show me again" flows.
    void resetAllNotifications();

signals:
    // Emitted when all conditions for a product are met.
    // firstThisSession: true the first time, false on subsequent matches —
    // GUI uses it to decide whether to fire the tray notification.
    void alertTriggered(AlertEvent event, bool firstThisSession);

private:
    QSet<int> m_notifiedThisSession;
};
