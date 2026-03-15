#pragma once

#include "core/DataStructs.hpp"
#include "core/PluginManager.hpp"

#include <QThread>
#include <QMutex>
#include <QMap>
#include <QTimer>
#include <vector>

// Background thread that polls product prices at their configured intervals.
//
// Thread model:
//   - PricePoller runs in its own QThread.
//   - One QTimer per product fires the fetch on that product's checkInterval.
//   - Fetch results are emitted via Qt signals (thread-safe cross-thread delivery).
//   - The product list can be updated at runtime via setProducts() (mutex-protected).
class PricePoller : public QObject {
    Q_OBJECT
public:
    explicit PricePoller(PluginManager* pluginManager, QObject* parent = nullptr);

    // Updates the list of products to poll. Thread-safe.
    void setProducts(std::vector<Product> products);

    // Starts the poller (call from the background QThread).
    void start();

    // Stops all timers and prepares for thread shutdown.
    void stop();

public slots:
    void onProductAdded(Product product);
    void onProductRemoved(int productId);
    void checkNow(int productId);

signals:
    void priceUpdated(Product product, FetchResult result);
    void productPriceChanged(int productId, float newPrice, float newDiscount);
    void checkNowFinished(int productId, bool success, float newPrice, float newDiscount);

private slots:
    void pollProduct(int productId);

private:
    void scheduleProduct(const Product& product);
    void unscheduleProduct(int productId);

    PluginManager*         m_pluginManager;
    QRecursiveMutex        m_mutex;
    QMap<int, Product>     m_products;
    QMap<int, QTimer*>     m_timers;
    bool                   m_running = false;
};
