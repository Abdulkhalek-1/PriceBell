#pragma once

#include "core/DataStructs.hpp"
#include <QSystemTrayIcon>
#include <QMenu>

class TrayIcon : public QSystemTrayIcon {
    Q_OBJECT
public:
    explicit TrayIcon(QWidget* mainWindow, QObject* parent = nullptr);

    void showAlert(const AlertEvent& event);

signals:
    void showWindowRequested();
    void quitRequested();
    void muteToggled(bool muted);

private slots:
    void onActivated(QSystemTrayIcon::ActivationReason reason);

private:
    void buildContextMenu(QWidget* mainWindow);
    bool m_muted = false;
    QAction* m_muteAction = nullptr;
    QString m_pendingAlertUrl;
    QMetaObject::Connection m_alertClickConnection;
};
