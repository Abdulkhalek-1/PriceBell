#include "core/PricePoller.hpp"
#include "utils/Logger.hpp"

#include <QMutexLocker>
#include <chrono>

PricePoller::PricePoller(PluginManager* pluginManager, QObject* parent)
    : QObject(parent)
    , m_pluginManager(pluginManager)
{}

void PricePoller::setProducts(const std::vector<Product>& products) {
    QMutexLocker lock(&m_mutex);
    m_products.clear();
    for (const auto& p : products)
        m_products[p.id] = p;
}

void PricePoller::start() {
    QMutexLocker lock(&m_mutex);
    for (auto it = m_products.begin(); it != m_products.end(); ++it) {
        scheduleProduct(it.value());
    }
    Logger::info("PricePoller started with " +
                 std::to_string(m_products.size()) + " product(s)");
}

void PricePoller::stop() {
    QMutexLocker lock(&m_mutex);
    for (auto* timer : m_timers)
        timer->stop();
    qDeleteAll(m_timers);
    m_timers.clear();
    Logger::info("PricePoller stopped");
}

void PricePoller::scheduleProduct(const Product& product) {
    if (!product.isActive) return;

    // Remove any existing timer for this product
    if (m_timers.contains(product.id)) {
        m_timers[product.id]->stop();
        delete m_timers[product.id];
        m_timers.remove(product.id);
    }

    int intervalMs = static_cast<int>(product.checkInterval.count()) * 1000;
    QTimer* timer  = new QTimer(this);
    timer->setInterval(intervalMs);
    timer->setSingleShot(false);

    int productId = product.id;
    connect(timer, &QTimer::timeout, this, [this, productId]() {
        pollProduct(productId);
    });

    m_timers[product.id] = timer;
    timer->start();

    // Fire immediately on first schedule
    QTimer::singleShot(0, this, [this, productId]() {
        pollProduct(productId);
    });
}

void PricePoller::unscheduleProduct(int productId) {
    if (m_timers.contains(productId)) {
        m_timers[productId]->stop();
        delete m_timers[productId];
        m_timers.remove(productId);
    }
    m_products.remove(productId);
}

void PricePoller::onProductAdded(Product product) {
    QMutexLocker lock(&m_mutex);
    m_products[product.id] = product;
    scheduleProduct(product);
}

void PricePoller::onProductRemoved(int productId) {
    QMutexLocker lock(&m_mutex);
    unscheduleProduct(productId);
}

void PricePoller::checkNow(int productId) {
    Product product;
    {
        QMutexLocker lock(&m_mutex);
        if (!m_products.contains(productId)) {
            emit checkNowFinished(productId, false, 0.0f, 0.0f);
            return;
        }
        product = m_products[productId];
    }

    std::string sourceId = product.sourcePluginId.empty()
        ? [&]() -> std::string {
            switch (product.source) {
                case SourceType::STEAM:   return "steam";
                case SourceType::UDEMY:   return "udemy";
                case SourceType::AMAZON:  return "amazon";
                default:                  return "generic";
            }
        }()
        : product.sourcePluginId;

    IPriceHandler* handler = m_pluginManager->handlerFor(sourceId);
    if (!handler) {
        emit checkNowFinished(productId, false, 0.0f, 0.0f);
        return;
    }

    FetchResult result = handler->fetchProduct(product.url);

    if (result.success) {
        {
            QMutexLocker lock(&m_mutex);
            if (m_products.contains(productId)) {
                m_products[productId].currentPrice = result.price;
                m_products[productId].discount     = result.discount;
            }
        }
        emit productPriceChanged(productId, result.price, result.discount);
        emit priceUpdated(product, result);
    }
    emit checkNowFinished(productId, result.success, result.price, result.discount);
}

void PricePoller::pollProduct(int productId) {
    Product product;
    {
        QMutexLocker lock(&m_mutex);
        if (!m_products.contains(productId)) return;
        product = m_products[productId];
    }

    if (!product.isActive) return;

    // Determine source id: plugin id takes priority over enum
    std::string sourceId = product.sourcePluginId.empty()
        ? [&]() -> std::string {
            switch (product.source) {
                case SourceType::STEAM:   return "steam";
                case SourceType::UDEMY:   return "udemy";
                case SourceType::AMAZON:  return "amazon";
                default:                  return "generic";
            }
        }()
        : product.sourcePluginId;

    IPriceHandler* handler = m_pluginManager->handlerFor(sourceId);
    if (!handler) {
        Logger::warn("No handler for source: " + sourceId + " (product: " + product.name + ")");
        return;
    }

    FetchResult result = handler->fetchProduct(product.url);

    if (result.success) {
        {
            QMutexLocker lock(&m_mutex);
            if (m_products.contains(productId)) {
                m_products[productId].currentPrice = result.price;
                m_products[productId].discount     = result.discount;
            }
        }
        emit productPriceChanged(productId, result.price, result.discount);
        emit priceUpdated(product, result);
    }
}
