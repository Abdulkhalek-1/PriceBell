#pragma once

#include "DataStructs.hpp"
#include <memory>
#include <string>

class HttpClient;

// Strategy interface for all price-fetching implementations.
// Every built-in handler and developer plugin must implement this.
class IPriceHandler {
public:
    virtual ~IPriceHandler() = default;

    // Fetch the current price and discount for the given product URL.
    virtual FetchResult fetchProduct(const std::string& url) = 0;

    // Unique identifier for this handler (e.g. "steam", "udemy", "my-plugin").
    virtual std::string handlerId() const = 0;

    // Human-readable display name shown in the UI source selector.
    virtual std::string displayName() const = 0;

    // Inject or replace the HttpClient used for network requests.
    virtual void setHttpClient(HttpClient* http) { (void)http; }

    // True when this handler recognises and can fetch the given URL. Used by
    // PluginManager::findHandlerForUrl to auto-route a pasted URL to the
    // matching handler. Default false so existing plugins keep compiling and
    // are only matched via their JSON-declared urlPatterns.
    virtual bool canHandle(const std::string& url) const { (void)url; return false; }

    // Returns a fresh, independent instance of this handler. Used by the
    // ProductDialog auto-detect flow so the dialog can run a name-fetch on its
    // own thread with its own HttpClient, without racing the poller against
    // the shared handler instance owned by PluginManager. Default returns
    // nullptr — handlers that don't support cloning simply skip auto-fetch.
    virtual std::unique_ptr<IPriceHandler> clone() const { return nullptr; }
};
