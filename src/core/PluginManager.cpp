#include "core/PluginManager.hpp"
#include "core/IPlugin.hpp"
#include "handlers/SteamHandler.hpp"
#include "handlers/UdemyHandler.hpp"
#include "handlers/AmazonHandler.hpp"
#include "handlers/GenericWebHandler.hpp"
#include "storage/Database.hpp"
#include "utils/Logger.hpp"

#include <QDir>
#include <QPluginLoader>
#include <QJsonArray>
#include <QSqlQuery>

PluginManager::PluginManager() {}

void PluginManager::registerBuiltins() {
    auto steam  = std::make_shared<SteamHandler>();
    auto udemy  = std::make_shared<UdemyHandler>();
    auto amazon = std::make_shared<AmazonHandler>();

    m_handlers[QString::fromStdString(steam->handlerId())]  = steam;
    m_handlers[QString::fromStdString(udemy->handlerId())]  = udemy;
    m_handlers[QString::fromStdString(amazon->handlerId())] = amazon;

    Logger::info("Registered built-in handlers: steam, udemy, amazon");
}

bool PluginManager::validatePluginMetadata(const QJsonObject& meta) const {
    // Required fields
    if (!meta.contains("id") || !meta.contains("name") ||
        !meta.contains("version") || !meta.contains("urlPatterns")) {
        Logger::warn("Plugin rejected: missing required metadata fields (id, name, version, urlPatterns)");
        return false;
    }

    // id must not conflict with built-ins
    QString id = meta.value("id").toString();
    if (id == "steam" || id == "udemy" || id == "amazon" || id == "generic") {
        Logger::warn("Plugin rejected: id conflicts with built-in handler: " + id.toStdString());
        return false;
    }

    // urlPatterns must be a non-empty array of strings
    QJsonArray patterns = meta.value("urlPatterns").toArray();
    if (patterns.isEmpty()) {
        Logger::warn("Plugin rejected: urlPatterns must be a non-empty array");
        return false;
    }
    for (const auto& p : patterns) {
        if (!p.isString()) {
            Logger::warn("Plugin rejected: urlPatterns must contain only strings");
            return false;
        }
        // Disallow wildcard-only or overly broad patterns
        QString pat = p.toString();
        if (pat == "*" || pat == "http://*" || pat == "https://*") {
            Logger::warn("Plugin rejected: urlPattern too broad: " + pat.toStdString());
            return false;
        }
    }

    return true;
}

void PluginManager::loadPlugins(const QString& pluginDir) {
    QDir dir(pluginDir);
    if (!dir.exists()) {
        Logger::info("Plugin directory does not exist: " + pluginDir.toStdString());
        return;
    }

    const QStringList filters = {"*.so", "*.dll", "*.dylib"};
    for (const QString& file : dir.entryList(filters, QDir::Files)) {
        QString path = dir.absoluteFilePath(file);
        QPluginLoader loader(path);

        QJsonObject meta = loader.metaData().value("MetaData").toObject();
        if (!validatePluginMetadata(meta)) {
            Logger::warn("Skipping plugin (invalid metadata): " + path.toStdString());
            continue;
        }

        QObject* obj = loader.instance();
        if (!obj) {
            Logger::error("Failed to load plugin: " + loader.errorString().toStdString());
            continue;
        }

        IPlugin* plugin = qobject_cast<IPlugin*>(obj);
        if (!plugin) {
            Logger::error("Plugin does not implement IPlugin: " + path.toStdString());
            loader.unload();
            continue;
        }

        QString id = meta.value("id").toString();
        m_handlers[id] = std::shared_ptr<IPriceHandler>(plugin, [&loader](IPriceHandler*) {
            loader.unload();
        });

        Logger::info("Loaded plugin: " + id.toStdString() + " from " + path.toStdString());
    }
}

void PluginManager::loadJsonSources() {
    QSqlQuery q("SELECT id, name, url_template, price_path, discount_path FROM sources "
                "WHERE is_plugin = 0 AND id NOT IN ('steam','udemy','amazon')",
                Database::connection());

    if (!q.exec()) {
        Logger::error("loadJsonSources query failed");
        return;
    }

    while (q.next()) {
        SourceConfig cfg;
        cfg.id            = q.value(0).toString().toStdString();
        cfg.name          = q.value(1).toString().toStdString();
        cfg.urlTemplate   = q.value(2).toString().toStdString();
        cfg.pricePath     = q.value(3).toString().toStdString();
        cfg.discountPath  = q.value(4).toString().toStdString();
        cfg.isDeveloperPlugin = false;

        auto handler = std::make_shared<GenericWebHandler>(cfg);
        m_handlers[QString::fromStdString(cfg.id)] = handler;
        Logger::info("Loaded JSON source: " + cfg.id);
    }
}

IPriceHandler* PluginManager::handlerFor(const std::string& sourceId) const {
    auto it = m_handlers.find(QString::fromStdString(sourceId));
    if (it == m_handlers.end()) return nullptr;
    return it->get();
}

std::vector<SourceConfig> PluginManager::availableSources() const {
    std::vector<SourceConfig> sources;
    for (auto it = m_handlers.begin(); it != m_handlers.end(); ++it) {
        SourceConfig cfg;
        cfg.id   = it.key().toStdString();
        cfg.name = it.value()->displayName();
        cfg.isDeveloperPlugin = (dynamic_cast<IPlugin*>(it.value().get()) != nullptr);
        sources.push_back(cfg);
    }
    return sources;
}
