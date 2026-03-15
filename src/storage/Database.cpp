#include "storage/Database.hpp"
#include "utils/Logger.hpp"

#include <QSqlQuery>
#include <QSqlError>
#include <QStandardPaths>
#include <QDir>

bool Database::open(const QString& path) {
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", kConnectionName);
    db.setDatabaseName(path);

    if (!db.open()) {
        Logger::error("Failed to open database: " + db.lastError().text().toStdString());
        return false;
    }

    Logger::info("Database opened: " + path.toStdString());
    return applySchema();
}

QSqlDatabase Database::connection() {
    return QSqlDatabase::database(kConnectionName);
}

void Database::close() {
    QSqlDatabase::database(kConnectionName).close();
    QSqlDatabase::removeDatabase(kConnectionName);
}

QString Database::defaultPath() {
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);
    return dir + "/pricebell.db";
}

bool Database::applySchema() {
    QSqlQuery q(connection());

    // Enable foreign key enforcement
    if (!q.exec("PRAGMA foreign_keys = ON;")) {
        Logger::error("PRAGMA failed: " + q.lastError().text().toStdString());
        return false;
    }

    const QStringList statements = {
        R"(
        CREATE TABLE IF NOT EXISTS sources (
            id           TEXT PRIMARY KEY,
            name         TEXT NOT NULL,
            type         INTEGER NOT NULL,
            url_template TEXT,
            price_path   TEXT,
            discount_path TEXT,
            is_plugin    INTEGER NOT NULL DEFAULT 0
        );
        )",
        R"(
        CREATE TABLE IF NOT EXISTS products (
            id             INTEGER PRIMARY KEY AUTOINCREMENT,
            name           TEXT    NOT NULL,
            url            TEXT    NOT NULL,
            source_id      TEXT    NOT NULL,
            current_price  REAL    NOT NULL DEFAULT 0,
            discount       REAL    NOT NULL DEFAULT 0,
            is_active      INTEGER NOT NULL DEFAULT 1,
            check_interval INTEGER NOT NULL DEFAULT 3600,
            last_checked   INTEGER NOT NULL DEFAULT 0
        );
        )",
        R"(
        CREATE TABLE IF NOT EXISTS price_conditions (
            id             INTEGER PRIMARY KEY AUTOINCREMENT,
            product_id     INTEGER NOT NULL REFERENCES products(id) ON DELETE CASCADE,
            condition_type INTEGER NOT NULL,
            value          REAL    NOT NULL
        );
        )",
        R"(
        CREATE TABLE IF NOT EXISTS alert_history (
            id                  INTEGER PRIMARY KEY AUTOINCREMENT,
            product_id          INTEGER NOT NULL REFERENCES products(id) ON DELETE CASCADE,
            product_name        TEXT    NOT NULL,
            price_at_trigger    REAL    NOT NULL,
            discount_at_trigger REAL    NOT NULL,
            triggered_at        INTEGER NOT NULL,
            status              INTEGER NOT NULL DEFAULT 0
        );
        )"
    };

    for (const QString& stmt : statements) {
        if (!q.exec(stmt)) {
            Logger::error("Schema error: " + q.lastError().text().toStdString());
            return false;
        }
    }

    // Seed built-in sources if not already present
    const QStringList seeds = {
        "INSERT OR IGNORE INTO sources(id,name,type,is_plugin) VALUES('steam','Steam',0,0);",
        "INSERT OR IGNORE INTO sources(id,name,type,is_plugin) VALUES('udemy','Udemy',1,0);",
        "INSERT OR IGNORE INTO sources(id,name,type,is_plugin) VALUES('amazon','Amazon',2,0);"
    };
    for (const QString& s : seeds) {
        q.exec(s); // best-effort
    }

    Logger::info("Database schema applied.");
    return true;
}
