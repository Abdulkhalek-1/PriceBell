#include "utils/AutoStartManager.hpp"
#include "utils/Logger.hpp"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QTextStream>

#if defined(Q_OS_LINUX)
#include <QStandardPaths>

QString AutoStartManager::autostartDir() {
    return QStandardPaths::writableLocation(QStandardPaths::ConfigLocation)
           + "/autostart";
}

QString AutoStartManager::desktopFilePath() {
    return autostartDir() + "/PriceBell.desktop";
}

bool AutoStartManager::isEnabled() {
    return QFile::exists(desktopFilePath());
}

bool AutoStartManager::setEnabled(bool enabled) {
    if (enabled) {
        QDir dir(autostartDir());
        if (!dir.exists() && !dir.mkpath(".")) {
            Logger::error("AutoStartManager: failed to create autostart directory");
            return false;
        }

        QFile file(desktopFilePath());
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            Logger::error("AutoStartManager: failed to write desktop file");
            return false;
        }

        QTextStream out(&file);
        out << "[Desktop Entry]\n"
            << "Type=Application\n"
            << "Name=PriceBell\n"
            << "Comment=Track and compare product prices across online stores\n"
            << "Exec=" << QCoreApplication::applicationFilePath() << "\n"
            << "Icon=pricebell\n"
            << "Terminal=false\n"
            << "X-GNOME-Autostart-enabled=true\n";
        file.close();

        Logger::info("AutoStartManager: auto-start enabled (Linux)");
        return true;
    } else {
        if (QFile::exists(desktopFilePath())) {
            if (!QFile::remove(desktopFilePath())) {
                Logger::error("AutoStartManager: failed to remove desktop file");
                return false;
            }
        }
        Logger::info("AutoStartManager: auto-start disabled (Linux)");
        return true;
    }
}

#elif defined(Q_OS_MAC)

QString AutoStartManager::launchAgentPath() {
    return QDir::homePath()
           + "/Library/LaunchAgents/com.pricebell.PriceBell.plist";
}

bool AutoStartManager::isEnabled() {
    return QFile::exists(launchAgentPath());
}

bool AutoStartManager::setEnabled(bool enabled) {
    if (enabled) {
        QDir dir(QDir::homePath() + "/Library/LaunchAgents");
        if (!dir.exists() && !dir.mkpath(".")) {
            Logger::error("AutoStartManager: failed to create LaunchAgents directory");
            return false;
        }

        QFile file(launchAgentPath());
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            Logger::error("AutoStartManager: failed to write plist file");
            return false;
        }

        QTextStream out(&file);
        out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
            << "<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\""
            << " \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n"
            << "<plist version=\"1.0\">\n"
            << "<dict>\n"
            << "    <key>Label</key>\n"
            << "    <string>com.pricebell.PriceBell</string>\n"
            << "    <key>ProgramArguments</key>\n"
            << "    <array>\n"
            << "        <string>" << QCoreApplication::applicationFilePath()
            << "</string>\n"
            << "    </array>\n"
            << "    <key>RunAtLoad</key>\n"
            << "    <true/>\n"
            << "</dict>\n"
            << "</plist>\n";
        file.close();

        Logger::info("AutoStartManager: auto-start enabled (macOS)");
        return true;
    } else {
        if (QFile::exists(launchAgentPath())) {
            if (!QFile::remove(launchAgentPath())) {
                Logger::error("AutoStartManager: failed to remove plist file");
                return false;
            }
        }
        Logger::info("AutoStartManager: auto-start disabled (macOS)");
        return true;
    }
}

#elif defined(Q_OS_WIN)
#include <QSettings>

static const char* kRunKey =
    "HKEY_CURRENT_USER\\Software\\Microsoft\\Windows\\CurrentVersion\\Run";

bool AutoStartManager::isEnabled() {
    QSettings reg(kRunKey, QSettings::NativeFormat);
    return reg.contains("PriceBell");
}

bool AutoStartManager::setEnabled(bool enabled) {
    QSettings reg(kRunKey, QSettings::NativeFormat);
    if (enabled) {
        QString path = QDir::toNativeSeparators(
            QCoreApplication::applicationFilePath());
        reg.setValue("PriceBell", path);
        Logger::info("AutoStartManager: auto-start enabled (Windows)");
    } else {
        reg.remove("PriceBell");
        Logger::info("AutoStartManager: auto-start disabled (Windows)");
    }
    return reg.status() == QSettings::NoError;
}

#else

bool AutoStartManager::isEnabled() {
    return false;
}

bool AutoStartManager::setEnabled(bool /*enabled*/) {
    Logger::warn("AutoStartManager: auto-start not supported on this platform");
    return false;
}

#endif
