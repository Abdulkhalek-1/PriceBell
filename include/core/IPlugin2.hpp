#pragma once

#include "core/IPlugin.hpp"
#include <QWidget>
#include <QIcon>
#include <QJsonObject>

// Extended plugin interface with optional UI integration.
//
// IPlugin2 builds on IPlugin by adding:
//   - settingsWidget(): embed a custom QWidget in the SettingsDialog "Plugins" tab
//   - icon():           provide an icon for the source selector
//   - styleSheet():     inject QSS for plugin UI elements
//   - dataModel():      describe custom data fields for generic UI rendering
//
// Plugins that only need basic price fetching can stick with IPlugin.
// Plugins that want richer UI integration should implement IPlugin2.
class IPlugin2 : public IPlugin {
public:
    virtual ~IPlugin2() = default;

    // Optional: settings widget hosted in SettingsDialog "Plugins" tab
    virtual QWidget* settingsWidget() { return nullptr; }

    // Optional: icon for source selector
    virtual QIcon icon() const { return QIcon(); }

    // Optional: QSS for plugin UI elements
    virtual QString styleSheet() const { return QString(); }

    // Optional: describes plugin's custom data fields for generic UI rendering
    virtual QJsonObject dataModel() const { return QJsonObject(); }
};

#define IPlugin2_iid "com.pricebell.IPlugin2/1.0"
Q_DECLARE_INTERFACE(IPlugin2, IPlugin2_iid)
