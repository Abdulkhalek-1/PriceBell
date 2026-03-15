#pragma once

#include <string>
#include <vector>
#include <chrono>
#include <QMetaType>

// ── Enumerations ──────────────────────────────────────────────────────────────

enum class ConditionType {
    PRICE_LESS_EQUAL,        // alert when price drops to or below threshold
    DISCOUNT_GREATER_EQUAL   // alert when discount reaches or exceeds threshold
};

enum class SourceType {
    STEAM,
    UDEMY,
    AMAZON,
    GENERIC,  // user-defined URL + JSON/CSS path
    PLUGIN    // developer native plugin (.so / .dll)
};

enum class AlertStatus {
    TRIGGERED,
    DISMISSED
};

// ── Core structs ──────────────────────────────────────────────────────────────

struct PriceCondition {
    int           id    = 0;
    ConditionType type  = ConditionType::PRICE_LESS_EQUAL;
    float         value = 0.0f;
};

struct Product {
    int                                   id             = 0;
    std::string                           name;
    std::string                           url;
    SourceType                            source         = SourceType::STEAM;
    std::string                           sourcePluginId; // non-empty when source == PLUGIN
    float                                 currentPrice   = 0.0f;
    float                                 discount       = 0.0f;  // percentage 0-100
    bool                                  isActive       = true;
    std::vector<PriceCondition>           filters;
    std::chrono::seconds                  checkInterval  = std::chrono::seconds(3600);
    std::chrono::system_clock::time_point lastChecked;
};

struct AlertEvent {
    int                                   id                = 0;
    int                                   productId         = 0;
    std::string                           productName;
    float                                 priceAtTrigger    = 0.0f;
    float                                 discountAtTrigger = 0.0f;
    std::chrono::system_clock::time_point triggeredAt;
    AlertStatus                           status            = AlertStatus::TRIGGERED;
};

// ── Source / plugin metadata ──────────────────────────────────────────────────

struct SourceConfig {
    std::string id;                       // unique identifier
    std::string name;                     // display name
    std::string urlTemplate;              // used by GenericWebHandler
    std::string pricePath;                // JSON-path or CSS selector for price
    std::string discountPath;             // optional: path for discount value
    bool        isDeveloperPlugin = false; // true → .so/.dll, false → JSON config
};

// ── Fetch result returned by all handlers ─────────────────────────────────────

struct FetchResult {
    bool        success  = false;
    float       price    = 0.0f;
    float       discount = 0.0f;
    std::string errorMsg;
};

// Required so Qt can queue these types across thread boundaries via signals.
Q_DECLARE_METATYPE(Product)
Q_DECLARE_METATYPE(FetchResult)
Q_DECLARE_METATYPE(AlertEvent)
