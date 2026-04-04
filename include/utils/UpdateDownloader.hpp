#pragma once

#include <QObject>
#include <QString>
#include <QJsonArray>

class QNetworkAccessManager;
class QNetworkReply;
class QFile;

// Downloads the platform-appropriate installer asset from a GitHub release.
// Selects asset by file extension: .deb on Linux, .exe on Windows, .dmg on macOS.
class UpdateDownloader : public QObject {
    Q_OBJECT
public:
    explicit UpdateDownloader(QObject* parent = nullptr);

    void download(const QJsonArray& assets);
    void cancel();

signals:
    void progress(int percent);
    void finished(const QString& filePath);
    void failed(const QString& errorMessage);

private:
    QString selectAssetUrl(const QJsonArray& assets) const;
    QString platformExtension() const;

    QNetworkAccessManager* m_manager;
    QNetworkReply*         m_reply     = nullptr;
    QFile*                 m_file      = nullptr;
    QString                m_destPath;
    bool                   m_canceling = false;
};
