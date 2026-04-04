#pragma once

#include <QObject>
#include <QString>
#include <QJsonArray>

class HttpClient;

class UpdateChecker : public QObject {
    Q_OBJECT
public:
    explicit UpdateChecker(QObject* parent = nullptr);

    void checkForUpdates();
    static bool isNewerVersion(const QString& current, const QString& remote);

signals:
    void updateAvailable(const QString& latestVersion,
                         const QString& releaseUrl,
                         const QString& releaseBody,
                         const QJsonArray& assets);
    void noUpdateAvailable();
    void checkFailed(const QString& errorMessage);

private:
    HttpClient* m_http;
};
