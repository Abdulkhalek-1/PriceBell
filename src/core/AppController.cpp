#include "core/AppController.hpp"
#include "core/PluginManager.hpp"
#include "core/AlertManager.hpp"
#include "core/PricePoller.hpp"
#include "utils/HttpClient.hpp"
#include "utils/SettingsProvider.hpp"
#include "utils/Logger.hpp"
#include "storage/ProductRepository.hpp"

#include <QApplication>
#include <QThread>

AppController::AppController(QObject* parent)
    : QObject(parent)
    , m_httpClient(std::make_unique<HttpClient>())
    , m_pluginManager(std::make_unique<PluginManager>())
    , m_alertManager(new AlertManager(this))
    , m_poller(nullptr)
    , m_pollerThread(new QThread(this))
{
}

AppController::~AppController() {
    if (m_pollerThread->isRunning()) {
        if (m_poller) m_poller->stop();
        m_pollerThread->quit();
        m_pollerThread->wait();
    }
}

void AppController::initialize() {
    // Register built-in handlers and load plugins
    m_pluginManager->registerBuiltins();

    QString pluginDir = SettingsProvider::instance().pluginDirectory();
    if (pluginDir.isEmpty()) {
        pluginDir = QApplication::applicationDirPath() + "/plugins";
    }
    m_pluginManager->loadPlugins(pluginDir);
    m_pluginManager->loadJsonSources();

    // Create poller and move to background thread
    m_poller = new PricePoller(m_pluginManager.get());
    m_poller->moveToThread(m_pollerThread);

    // Wire poller signals
    connect(m_pollerThread, &QThread::started, m_poller, &PricePoller::start);

    connect(m_poller, &PricePoller::priceUpdated,
            m_alertManager, &AlertManager::onPriceUpdated);

    connect(m_poller, &PricePoller::productPriceChanged,
            this, &AppController::priceChanged);

    connect(m_poller, &PricePoller::checkNowFinished,
            this, &AppController::checkNowFinished);

    // Wire alert manager
    connect(m_alertManager, &AlertManager::alertTriggered,
            this, &AppController::alertTriggered);

    // Load products and start polling
    auto products = ProductRepository::findAll();
    m_poller->setProducts(products);
    m_pollerThread->start();
}

bool AppController::addProduct(Product& product) {
    if (!ProductRepository::save(product)) {
        return false;
    }
    m_poller->onProductAdded(product);
    emit productAdded(product);
    return true;
}

bool AppController::editProduct(const Product& product) {
    if (!ProductRepository::update(product)) {
        return false;
    }
    m_poller->onProductAdded(product); // re-schedules the timer
    emit productUpdated(product);
    return true;
}

bool AppController::removeProduct(int productId) {
    ProductRepository::remove(productId);
    m_poller->onProductRemoved(productId);
    emit productRemoved(productId);
    return true;
}

std::vector<Product> AppController::products() const {
    return ProductRepository::findAll();
}

void AppController::startPolling() {
    if (!m_pollerThread->isRunning()) {
        m_pollerThread->start();
    }
}

void AppController::stopPolling() {
    if (m_poller) m_poller->stop();
}

void AppController::checkNow(int productId) {
    QMetaObject::invokeMethod(m_poller, "checkNow",
                              Qt::QueuedConnection,
                              Q_ARG(int, productId));
}

PluginManager* AppController::pluginManager() const {
    return m_pluginManager.get();
}

AlertManager* AppController::alertManager() const {
    return m_alertManager;
}
