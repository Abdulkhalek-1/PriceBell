#include "utils/UpdateChecker.hpp"
#include "utils/HttpClient.hpp"

#include <QJsonDocument>
#include <QJsonObject>
#include <QStringList>

#ifndef APP_VERSION
#define APP_VERSION "0.0.0"
#endif

UpdateChecker::UpdateChecker(QObject* parent)
    : QObject(parent)
    , m_http(new HttpClient(this))
{
    m_http->setHeader("Accept", "application/vnd.github.v3+json");
}

void UpdateChecker::checkForUpdates() {
    m_http->get("https://api.github.com/repos/Abdulkhalek-1/PriceBell/releases/latest",
        [this](bool success, const QString& body, const QString& error) {
            if (!success) {
                emit checkFailed(error);
                return;
            }

            QJsonDocument doc = QJsonDocument::fromJson(body.toUtf8());
            if (!doc.isObject()) {
                emit checkFailed(tr("Invalid response from server."));
                return;
            }

            QJsonObject obj = doc.object();
            QString tagName = obj.value("tag_name").toString();
            QString htmlUrl = obj.value("html_url").toString();

            if (tagName.isEmpty()) {
                emit checkFailed(tr("No release information found."));
                return;
            }

            // Strip leading "v" from tag
            QString remoteVersion = tagName.startsWith('v') ? tagName.mid(1) : tagName;

            if (isNewerVersion(APP_VERSION, remoteVersion)) {
                emit updateAvailable(remoteVersion, htmlUrl);
            } else {
                emit noUpdateAvailable();
            }
        });
}

bool UpdateChecker::isNewerVersion(const QString& current, const QString& remote) {
    QStringList curParts = current.split('.');
    QStringList remParts = remote.split('.');

    int maxLen = qMax(curParts.size(), remParts.size());
    for (int i = 0; i < maxLen; ++i) {
        int c = (i < curParts.size()) ? curParts[i].toInt() : 0;
        int r = (i < remParts.size()) ? remParts[i].toInt() : 0;
        if (r > c) return true;
        if (r < c) return false;
    }
    return false;
}
