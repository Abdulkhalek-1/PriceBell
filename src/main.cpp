#include <QApplication>
#include <QCommandLineParser>
#include <QFontDatabase>
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
#include "utils/SettingsProvider.hpp"
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
        app.setWindowIcon(QIcon(":/assets/icons/app_icon.svg"));
        // Bundle Noto Sans to avoid Arial fallback on systems missing it
        if (QFontDatabase::addApplicationFont(":/fonts/NotoSans-Regular.ttf") == -1)
            qWarning("Failed to load bundled font: NotoSans-Regular.ttf");
        if (QFontDatabase::addApplicationFont(":/fonts/NotoSans-Bold.ttf") == -1)
            qWarning("Failed to load bundled font: NotoSans-Bold.ttf");
        if (QFontDatabase::addApplicationFont(":/fonts/NotoNaskhArabic-Regular.ttf") == -1)
            qWarning("Failed to load bundled font: NotoNaskhArabic-Regular.ttf");
        app.setFont(QFont("Noto Sans", 10));
        // Keep running in the background when main window is hidden (tray)
        app.setQuitOnLastWindowClosed(false);

        QCommandLineParser parser;
        parser.addHelpOption();
        parser.addOption(QCommandLineOption("minimized", "Start minimized to tray"));
        parser.process(app);
        bool startMinimized = parser.isSet("minimized")
                           || SettingsProvider::instance().launchMinimized();

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
        if (!startMinimized) {
            window.show();
        }

        exitCode = app.exec();
        Database::close();
    } while (exitCode == RESTART_EXIT_CODE);

    return exitCode;
}
