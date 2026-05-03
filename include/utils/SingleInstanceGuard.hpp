#pragma once

#include <QObject>
#include <QString>

class QLocalServer;

// Cross-platform single-instance enforcement using QLocalServer.
//
// Usage in main():
//   SingleInstanceGuard guard("pricebell");
//   if (guard.isSecondary()) {
//       guard.notifyPrimary();   // best-effort RAISE byte
//       return 0;
//   }
//   QObject::connect(&guard, &SingleInstanceGuard::secondInstanceLaunched,
//                    &window, &MainWindow::raiseFromAnywhere);
class SingleInstanceGuard : public QObject {
    Q_OBJECT
public:
    // appKey: short application identifier; the actual socket name appends
    // the current username so multiple users on the same host can each run
    // their own instance.
    explicit SingleInstanceGuard(const QString& appKey, QObject* parent = nullptr);
    ~SingleInstanceGuard() override;

    // True when another instance is already listening on the socket.
    bool isSecondary() const { return m_isSecondary; }

    // Connect to the existing primary and send a RAISE byte. Best-effort —
    // returns true on socket connect, false otherwise.
    bool notifyPrimary();

signals:
    // Emitted on the primary instance when a secondary connects.
    void secondInstanceLaunched();

private:
    QString       m_socketName;
    QLocalServer* m_server = nullptr;
    bool          m_isSecondary = false;
};
