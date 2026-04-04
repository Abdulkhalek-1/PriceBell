#include "gui/TrayIcon.hpp"
#include "utils/CurrencyUtils.hpp"
#include "utils/SettingsProvider.hpp"
#include "utils/Constants.hpp"

#include <QApplication>
#include <QDesktopServices>
#include <QFile>
#include <QIcon>
#include <QSoundEffect>
#include <QUrl>

TrayIcon::TrayIcon(QWidget* mainWindow, QObject* parent)
    : QSystemTrayIcon(QIcon(":/assets/icons/tray_normal.svg"), parent)
{
    setToolTip(tr("PriceBell"));
    buildContextMenu(mainWindow);
    connect(this, &QSystemTrayIcon::activated, this, &TrayIcon::onActivated);
}

void TrayIcon::buildContextMenu(QWidget* mainWindow) {
    QMenu* menu = new QMenu(mainWindow);

    QAction* showAction = menu->addAction(tr("Show PriceBell"));
    connect(showAction, &QAction::triggered, this, &TrayIcon::showWindowRequested);

    menu->addSeparator();

    m_muteAction = menu->addAction(tr("Mute Notifications"));
    m_muteAction->setCheckable(true);
    connect(m_muteAction, &QAction::toggled, this, [this](bool checked) {
        m_muted = checked;
        emit muteToggled(m_muted);
    });

    menu->addSeparator();

    QAction* quitAction = menu->addAction(tr("Quit"));
    connect(quitAction, &QAction::triggered, this, &TrayIcon::quitRequested);

    setContextMenu(menu);
}

void TrayIcon::onActivated(QSystemTrayIcon::ActivationReason reason) {
    if (reason == QSystemTrayIcon::DoubleClick || reason == QSystemTrayIcon::Trigger)
        emit showWindowRequested();
}

void TrayIcon::showAlert(const AlertEvent& event) {
    if (m_muted || !isSystemTrayAvailable()) return;

    QString title = tr("PriceBell Alert");
    QString msg   = tr("%1 — %2 (%3% off)")
        .arg(QString::fromStdString(event.productName))
        .arg(CurrencyUtils::formatPrice(event.priceAtTrigger, "USD"))
        .arg(static_cast<int>(event.discountAtTrigger));

    // Disconnect any previous click handler before setting up the new one
    if (m_alertClickConnection) {
        disconnect(m_alertClickConnection);
        m_alertClickConnection = {};
    }

    QString alertUrl = QString::fromStdString(event.productUrl);

    if (!alertUrl.isEmpty()) {
        // Capture alertUrl and a pointer to m_alertClickConnection by value/address
        // so the lambda disconnects its own specific handle, not whatever the member
        // holds at firing time (which may have been overwritten by a subsequent alert).
        m_alertClickConnection = connect(this, &QSystemTrayIcon::messageClicked,
                                         this, [this, alertUrl]() {
            QDesktopServices::openUrl(QUrl(alertUrl));
            // Disconnect our own handle — safe because we're captured via [this]
            // and Qt guarantees this lambda won't fire after TrayIcon is destroyed.
            if (m_alertClickConnection) {
                disconnect(m_alertClickConnection);
                m_alertClickConnection = {};
            }
        });
    }

    showMessage(title, msg, QSystemTrayIcon::Information,
                PriceBell::kTrayAlertTimeoutMs);

    // Play notification sound if enabled
    if (SettingsProvider::instance().notificationSoundEnabled()) {
        QUrl soundUrl;
        QString customPath = SettingsProvider::instance().notificationSoundPath();
        if (!customPath.isEmpty() && QFile::exists(customPath)) {
            soundUrl = QUrl::fromLocalFile(customPath);
        } else {
            soundUrl = QUrl("qrc:/assets/sounds/alert.wav");
        }
        QSoundEffect* sound = new QSoundEffect(this);
        sound->setSource(soundUrl);
        sound->play();
        connect(sound, &QSoundEffect::playingChanged, this, [sound]() {
            if (!sound->isPlaying()) {
                sound->deleteLater();
            }
        });
    }
}
