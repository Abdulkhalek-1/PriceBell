#pragma once

#include "core/DataStructs.hpp"
#include <QObject>

// Receives price-updated signals from PricePoller, evaluates conditions,
// persists alert events, and notifies the GUI via Qt signals.
class AlertManager : public QObject {
    Q_OBJECT
public:
    explicit AlertManager(QObject* parent = nullptr);

public slots:
    // Called by PricePoller when a fresh FetchResult arrives for a product.
    void onPriceUpdated(Product product, FetchResult result);

signals:
    // Emitted when all conditions for a product are met.
    void alertTriggered(AlertEvent event);
};
