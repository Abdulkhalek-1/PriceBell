#include "utils/UpdateDownloader.hpp"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QFile>
#include <QJsonObject>
#include <QStandardPaths>
#include <QDir>

UpdateDownloader::UpdateDownloader(QObject* parent)
    : QObject(parent)
    , m_manager(new QNetworkAccessManager(this))
{}

QString UpdateDownloader::platformExtension() const {
#if defined(Q_OS_WIN)
    return ".exe";
#elif defined(Q_OS_MACOS)
    return ".dmg";
#else
    return ".deb";
#endif
}

QString UpdateDownloader::selectAssetUrl(const QJsonArray& assets) const {
    QString ext = platformExtension();
    for (const auto& val : assets) {
        QJsonObject asset = val.toObject();
        QString name = asset.value("name").toString();
        if (name.endsWith(ext, Qt::CaseInsensitive)) {
            return asset.value("browser_download_url").toString();
        }
    }
    return QString();
}

void UpdateDownloader::download(const QJsonArray& assets) {
    QString url = selectAssetUrl(assets);
    if (url.isEmpty()) {
        emit failed(tr("No installer asset found for this platform."));
        return;
    }

    QString tempDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    m_destPath = QDir(tempDir).filePath("PriceBell-update" + platformExtension());

    m_file = new QFile(m_destPath, this);
    if (!m_file->open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        emit failed(tr("Cannot write to temp file: %1").arg(m_destPath));
        return;
    }

    QNetworkRequest req{QUrl(url)};
    req.setHeader(QNetworkRequest::UserAgentHeader, "PriceBell");
    m_reply = m_manager->get(req);

    connect(m_reply, &QNetworkReply::downloadProgress,
            this, [this](qint64 received, qint64 total) {
        if (total > 0)
            emit progress(static_cast<int>(received * 100 / total));
    });

    connect(m_reply, &QNetworkReply::readyRead, this, [this]() {
        if (m_file) m_file->write(m_reply->readAll());
    });

    connect(m_reply, &QNetworkReply::finished, this, [this]() {
        m_file->close();
        if (m_reply->error() != QNetworkReply::NoError) {
            m_file->remove();
            emit failed(m_reply->errorString());
        } else {
            emit finished(m_destPath);
        }
        m_reply->deleteLater();
        m_reply = nullptr;
    });
}

void UpdateDownloader::cancel() {
    if (m_reply) {
        m_reply->abort();
        m_reply = nullptr;
    }
    if (m_file) {
        m_file->close();
        m_file->remove();
        m_file = nullptr;
    }
}
