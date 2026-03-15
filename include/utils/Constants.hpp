#pragma once

namespace PriceBell {

// Network
static constexpr int kDefaultNetworkTimeoutMs = 15000;

// Polling intervals (seconds)
static constexpr int kMinCheckInterval    = 30;
static constexpr int kMaxCheckInterval    = 86400;
static constexpr int kDefaultCheckInterval = 3600;

// UI
static constexpr int kTrayAlertTimeoutMs = 5000;

// Application
static constexpr int kRestartExitCode = 1000;

} // namespace PriceBell
