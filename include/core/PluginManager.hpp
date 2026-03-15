#pragma once

#include "core/IPriceHandler.hpp"
#include "core/DataStructs.hpp"

#include <QString>
#include <QMap>
#include <QJsonObject>
#include <memory>
#include <vector>

// Manages all available price handlers — both built-in and dynamically loaded.
//
// Dual-tier extensibility:
//   Tier 1 (developer): Native .so/.dll plugins implementing IPlugin.
//                        Loaded at startup from the plugins/ directory.
//                        URL sandboxing enforced via declared urlPatterns.
//   Tier 2 (user):      JSON config sources stored in the database sources table.
//                        Instantiated as GenericWebHandler instances.
class PluginManager {
public:
    PluginManager();

    // Registers the three built-in handlers (Steam, Udemy, Amazon).
    void registerBuiltins();

    // Scans pluginDir for .so/.dll files, validates metadata, loads safe plugins.
    void loadPlugins(const QString& pluginDir);

    // Loads user-defined JSON config sources from the database.
    void loadJsonSources();

    // Returns the handler for the given source id, or nullptr if not found.
    IPriceHandler* handlerFor(const std::string& sourceId) const;

    // Returns metadata for all registered handlers (for UI source selector).
    std::vector<SourceConfig> availableSources() const;

private:
    // Validates that a plugin's declared metadata is safe to load.
    bool validatePluginMetadata(const QJsonObject& meta) const;

    QMap<QString, std::shared_ptr<IPriceHandler>> m_handlers;
};
