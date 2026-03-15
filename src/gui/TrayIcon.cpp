#include "gui/TrayIcon.hpp"
#include <QApplication>
#include <QIcon>

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
    QString msg   = tr("%1 — $%2 (%3% off)")
        .arg(QString::fromStdString(event.productName))
        .arg(event.priceAtTrigger, 0, 'f', 2)
        .arg(static_cast<int>(event.discountAtTrigger));

    showMessage(title, msg, QSystemTrayIcon::Information, 5000);
}
