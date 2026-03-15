#include <QApplication>
#include <QIcon>
#include <QTranslator>
#include <QSettings>
#include <QLocale>
#include <QLibraryInfo>
#include <QStandardPaths>
#include "core/AppController.hpp"
#include "gui/MainWindow.hpp"
#include "storage/Database.hpp"
#include "utils/Logger.hpp"
#include "utils/Constants.hpp"
#include "core/DataStructs.hpp"

int main(int argc, char* argv[]) {
    int exitCode = 0;

    do {
        QApplication app(argc, argv);

        // Register custom types for cross-thread queued signal delivery.
        qRegisterMetaType<Product>("Product");
        qRegisterMetaType<FetchResult>("FetchResult");
        qRegisterMetaType<AlertEvent>("AlertEvent");
        app.setApplicationName("PriceBell");
        app.setOrganizationName("PriceBell");
        app.setWindowIcon(QIcon(":/assets/logo.svg"));
        // Keep running in the background when main window is hidden (tray)
        app.setQuitOnLastWindowClosed(false);

        // Initialise file-based logger
        Logger::init(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation).toStdString());

        // Load user's language preference
        QSettings settings("PriceBell", "PriceBell");
        QString locale = settings.value("language", "en").toString();

        // Load app translations
        QTranslator translator;
        if (translator.load("pricebell_" + locale, QApplication::applicationDirPath() + "/i18n")) {
            app.installTranslator(&translator);
        }

        // Load Qt's own translations (OK, Cancel, etc.)
        QTranslator qtTranslator;
        if (qtTranslator.load("qt_" + locale, QLibraryInfo::location(QLibraryInfo::TranslationsPath))) {
            app.installTranslator(&qtTranslator);
        }

        // Set RTL layout for Arabic and similar locales
        if (QLocale(locale).textDirection() == Qt::RightToLeft) {
            QApplication::setLayoutDirection(Qt::RightToLeft);
        } else {
            QApplication::setLayoutDirection(Qt::LeftToRight);
        }

        if (!Database::open()) {
            Logger::error("Failed to open database. Exiting.");
            return 1;
        }

        AppController controller;
        controller.initialize();

        MainWindow window(&controller);
        window.show();

        exitCode = app.exec();
        Database::close();
    } while (exitCode == RESTART_EXIT_CODE);

    return exitCode;
}
