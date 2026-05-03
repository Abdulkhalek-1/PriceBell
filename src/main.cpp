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
#include "utils/SingleInstanceGuard.hpp"
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

        // Prevent duplicate instances. Must run before Database::open() so a
        // secondary doesn't briefly grab the SQLite handle.
        SingleInstanceGuard instanceGuard(QStringLiteral("pricebell"));
        if (instanceGuard.isSecondary()) {
            instanceGuard.notifyPrimary();
            return 0;
        }
        // Bundle Noto Sans to avoid Arial/Segoe-UI fallback on Windows.
        // The QSS font-family list has Noto Sans first, but it only resolves if
        // QFontDatabase actually registered a family literally named "Noto Sans".
        auto loadFont = [](const char* path) {
            int id = QFontDatabase::addApplicationFont(QString::fromLatin1(path));
            if (id == -1) {
                qWarning("Failed to load bundled font: %s", path);
                return;
            }
            const QStringList families = QFontDatabase::applicationFontFamilies(id);
            if (families.isEmpty()) {
                qWarning("Loaded font has no families: %s", path);
            } else {
                qInfo("Loaded font %s as family '%s'", path,
                      qPrintable(families.first()));
            }
        };
        loadFont(":/fonts/NotoSans-Regular.ttf");
        loadFont(":/fonts/NotoSans-Bold.ttf");
        loadFont(":/fonts/NotoNaskhArabic-Regular.ttf");
        if (!QFontDatabase().families().contains("Noto Sans")) {
            qWarning("'Noto Sans' family not present after addApplicationFont — "
                     "stylesheet font-family will fall back to system fonts.");
        }
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
        QObject::connect(&instanceGuard, &SingleInstanceGuard::secondInstanceLaunched,
                         &window, &MainWindow::showFromTray);
        if (!startMinimized) {
            window.show();
        }

        exitCode = app.exec();
        Database::close();
    } while (exitCode == RESTART_EXIT_CODE);

    return exitCode;
}
