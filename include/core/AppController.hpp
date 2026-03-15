#pragma once
#include <QObject>
#include <memory>
#include <vector>
#include "core/DataStructs.hpp"

class PluginManager;
class AlertManager;
class PricePoller;
class HttpClient;
class QThread;

class AppController : public QObject {
    Q_OBJECT
public:
    explicit AppController(QObject* parent = nullptr);
    ~AppController();

    void initialize();

    // Product operations
    bool addProduct(Product& product);
    bool editProduct(const Product& product);
    bool removeProduct(int productId);
    std::vector<Product> products() const;

    // Polling
    void startPolling();
    void stopPolling();
    void checkNow(int productId);

    // Component access (for signal connections)
    PluginManager* pluginManager() const;
    AlertManager* alertManager() const;

signals:
    void productAdded(const Product& product);
    void productUpdated(const Product& product);
    void productRemoved(int productId);
    void priceChanged(int productId, float newPrice, float newDiscount);
    void alertTriggered(const AlertEvent& event);
    void checkNowFinished(int productId, bool success, float price, float discount);

private:
    std::unique_ptr<HttpClient> m_httpClient;
    std::unique_ptr<PluginManager> m_pluginManager;
    AlertManager* m_alertManager;  // QObject parent = this
    PricePoller* m_poller;         // moved to m_pollerThread
    QThread* m_pollerThread;       // QObject parent = this
};
