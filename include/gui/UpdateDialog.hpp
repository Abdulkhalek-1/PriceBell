#pragma once

#include <QDialog>
#include <QJsonArray>

class QLabel;
class QProgressBar;
class QPushButton;
class UpdateDownloader;

class UpdateDialog : public QDialog {
    Q_OBJECT
public:
    explicit UpdateDialog(const QString& currentVersion,
                          const QString& newVersion,
                          const QString& releaseBody,
                          const QJsonArray& assets,
                          QWidget* parent = nullptr);

private slots:
    void onUpdateNow();
    void onSkipVersion();
    void onDownloadProgress(int percent);
    void onDownloadFinished(const QString& filePath);
    void onDownloadFailed(const QString& error);

private:
    void setupUi(const QString& currentVersion,
                 const QString& newVersion,
                 const QString& releaseBody);

    QString             m_newVersion;
    QJsonArray          m_assets;
    QLabel*             m_statusLabel  = nullptr;
    QProgressBar*       m_progress     = nullptr;
    QPushButton*        m_updateBtn    = nullptr;
    QPushButton*        m_laterBtn     = nullptr;
    QPushButton*        m_skipBtn      = nullptr;
    QPushButton*        m_cancelBtn    = nullptr;
    UpdateDownloader*   m_downloader   = nullptr;
};
