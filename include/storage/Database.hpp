#pragma once

#include <QString>
#include <QSqlDatabase>

// Opens (or creates) the SQLite database and applies schema migrations.
class Database {
public:
    // Opens the database at the given file path. Creates the file if it does not exist.
    // Returns true on success.
    static bool open(const QString& path = defaultPath());

    // Returns the active QSqlDatabase connection.
    static QSqlDatabase connection();

    // Closes the database connection.
    static void close();

private:
    static bool applySchema();
    static int  getSchemaVersion();
    static bool applyMigrations();
    static QString defaultPath();

    static constexpr const char* kConnectionName = "pricebell";
};
