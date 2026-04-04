#include "gui/UpdateDialog.hpp"
#include "utils/UpdateDownloader.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QSettings>
#include <QProcess>
#include <QApplication>
#include <QMessageBox>

UpdateDialog::UpdateDialog(const QString& currentVersion,
                           const QString& newVersion,
                           const QString& releaseBody,
                           const QJsonArray& assets,
                           QWidget* parent)
    : QDialog(parent)
    , m_newVersion(newVersion)
    , m_assets(assets)
{
    setWindowTitle(tr("PriceBell Update Available"));
    setModal(true);
    setFixedWidth(440);
    setupUi(currentVersion, newVersion, releaseBody);
}

void UpdateDialog::setupUi(const QString& currentVersion,
                            const QString& newVersion,
                            const QString& releaseBody) {
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setSpacing(12);
    layout->setContentsMargins(20, 16, 20, 16);

    QLabel* versionLabel = new QLabel(
        tr("<b>PriceBell %1</b> is available — you have %2")
            .arg(newVersion, currentVersion), this);
    versionLabel->setWordWrap(true);
    layout->addWidget(versionLabel);

    if (!releaseBody.isEmpty()) {
        QString truncated = releaseBody.left(400);
        if (releaseBody.size() > 400) truncated += "…";
        QLabel* changelogLabel = new QLabel(truncated, this);
        changelogLabel->setWordWrap(true);
        changelogLabel->setStyleSheet("color: #a6adc8; font-size: 11px;");
        layout->addWidget(changelogLabel);
    }

    m_statusLabel = new QLabel(this);
    m_statusLabel->setVisible(false);
    layout->addWidget(m_statusLabel);

    m_progress = new QProgressBar(this);
    m_progress->setRange(0, 100);
    m_progress->setValue(0);
    m_progress->setVisible(false);
    layout->addWidget(m_progress);

    QHBoxLayout* btnRow = new QHBoxLayout();
    m_updateBtn = new QPushButton(tr("Update Now"), this);
    m_laterBtn  = new QPushButton(tr("Later"),      this);
    m_skipBtn   = new QPushButton(tr("Skip This Version"), this);
    m_cancelBtn = new QPushButton(tr("Cancel Download"),   this);
    m_cancelBtn->setVisible(false);

    m_updateBtn->setDefault(true);
    btnRow->addWidget(m_skipBtn);
    btnRow->addStretch();
    btnRow->addWidget(m_laterBtn);
    btnRow->addWidget(m_cancelBtn);
    btnRow->addWidget(m_updateBtn);
    layout->addLayout(btnRow);

    connect(m_updateBtn, &QPushButton::clicked, this, &UpdateDialog::onUpdateNow);
    connect(m_laterBtn,  &QPushButton::clicked, this, &QDialog::reject);
    connect(m_skipBtn,   &QPushButton::clicked, this, &UpdateDialog::onSkipVersion);
    connect(m_cancelBtn, &QPushButton::clicked, this, [this]() {
        if (m_downloader) m_downloader->cancel();
        m_progress->setVisible(false);
        m_statusLabel->setVisible(false);
        m_cancelBtn->setVisible(false);
        m_updateBtn->setVisible(true);
        m_laterBtn->setEnabled(true);
        m_skipBtn->setEnabled(true);
    });
}

void UpdateDialog::onUpdateNow() {
    m_updateBtn->setVisible(false);
    m_laterBtn->setEnabled(false);
    m_skipBtn->setEnabled(false);
    m_cancelBtn->setVisible(true);
    m_statusLabel->setText(tr("Downloading update…"));
    m_statusLabel->setVisible(true);
    m_progress->setVisible(true);

    m_downloader = new UpdateDownloader(this);
    connect(m_downloader, &UpdateDownloader::progress,
            this, &UpdateDialog::onDownloadProgress);
    connect(m_downloader, &UpdateDownloader::finished,
            this, &UpdateDialog::onDownloadFinished);
    connect(m_downloader, &UpdateDownloader::failed,
            this, &UpdateDialog::onDownloadFailed);
    m_downloader->download(m_assets);
}

void UpdateDialog::onSkipVersion() {
    QSettings s("PriceBell", "PriceBell");
    s.setValue("updates/skippedVersion", m_newVersion);
    reject();
}

void UpdateDialog::onDownloadProgress(int percent) {
    m_progress->setValue(percent);
    m_statusLabel->setText(tr("Downloading update… %1%").arg(percent));
}

void UpdateDialog::onDownloadFinished(const QString& filePath) {
    m_statusLabel->setText(tr("Download complete. Launching installer…"));
    m_cancelBtn->setVisible(false);

#if defined(Q_OS_WIN)
    bool launched = QProcess::startDetached(filePath, {});
#elif defined(Q_OS_MACOS)
    bool launched = QProcess::startDetached("open", {filePath});
#else
    bool launched = QProcess::startDetached("xdg-open", {filePath});
#endif
    if (!launched) {
        QMessageBox::critical(this, tr("Update Error"),
            tr("Could not launch installer:\n%1").arg(filePath));
        reject();
        return;
    }
    QApplication::quit();
}

void UpdateDialog::onDownloadFailed(const QString& error) {
    m_progress->setVisible(false);
    m_cancelBtn->setVisible(false);
    m_updateBtn->setVisible(true);
    m_laterBtn->setEnabled(true);
    m_skipBtn->setEnabled(true);
    m_statusLabel->setText(tr("Download failed: %1").arg(error));
}
