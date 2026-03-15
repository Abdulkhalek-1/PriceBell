#pragma once

#include <QString>

class AutoStartManager {
public:
    static bool isEnabled();
    static bool setEnabled(bool enabled);

private:
#ifdef Q_OS_LINUX
    static QString autostartDir();
    static QString desktopFilePath();
#endif

#ifdef Q_OS_MAC
    static QString launchAgentPath();
#endif
};
