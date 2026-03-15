#pragma once

#include "core/IPriceHandler.hpp"
#include <QtPlugin>
#include <QJsonObject>

// Qt plugin interface for developer-supplied native price handlers.
//
// Security model:
//   - Plugins are loaded via QPluginLoader which validates Qt ABI compatibility.
//   - Each plugin declares its metadata (id, name, version, urlPatterns).
//   - PluginManager enforces URL pattern sandboxing: any fetch request that does
//     not match a declared urlPattern is rejected before the network call.
//
// To create a plugin:
//   1. Subclass QObject and IPlugin in a shared library project.
//   2. Implement all pure virtuals.
//   3. Add Q_INTERFACES(IPlugin) and Q_PLUGIN_METADATA(IID IPlugin_iid FILE "meta.json").
//   4. meta.json must contain:
//        { "id": "...", "name": "...", "version": "1.0", "urlPatterns": ["https://example.com/*"] }
//   5. Drop the compiled .so / .dll into the plugins/ directory.
class IPlugin : public IPriceHandler {
public:
    ~IPlugin() override = default;

    // Returns the plugin metadata JSON as declared in the plugin meta file.
    // Must include: id, name, version, urlPatterns (array of allowed URL prefixes).
    virtual QJsonObject metadata() const = 0;
};

#define IPlugin_iid "com.pricebell.IPlugin/1.0"
Q_DECLARE_INTERFACE(IPlugin, IPlugin_iid)
