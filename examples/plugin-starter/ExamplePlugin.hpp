#pragma once

#include "core/IPlugin2.hpp"
#include "core/DataStructs.hpp"
#include <QObject>
#include <QJsonObject>
#include <QIcon>
#include <QWidget>

// ExamplePlugin — minimal IPlugin2 implementation.
// Returns a hardcoded price of 9.99 for any URL containing "example.com".
// Use this as a starting point for your own native plugin.
class ExamplePlugin : public QObject, public IPlugin2 {
    Q_OBJECT
    Q_INTERFACES(IPlugin IPlugin2)
    Q_PLUGIN_METADATA(IID IPlugin2_iid FILE "example-plugin.json")

public:
    // IPriceHandler
    FetchResult fetchProduct(const std::string& url) override;
    std::string handlerId()   const override;
    std::string displayName() const override;

    // IPlugin
    QJsonObject metadata() const override;

    // IPlugin2 (all optional — override only what you need)
    QWidget*    settingsWidget() override { return nullptr; }
    QIcon       icon()     const override { return QIcon(); }
};
