#include "utils/SingleInstanceGuard.hpp"
#include "utils/Logger.hpp"

#include <QLocalServer>
#include <QLocalSocket>
#include <QString>

#ifdef Q_OS_WIN
#include <windows.h>
#include <lmcons.h>
#else
#include <unistd.h>
#include <pwd.h>
#endif

static QString currentUserName() {
#ifdef Q_OS_WIN
    char buf[UNLEN + 1] = {0};
    DWORD size = UNLEN + 1;
    if (GetUserNameA(buf, &size)) return QString::fromLocal8Bit(buf);
    return QStringLiteral("user");
#else
    if (auto* pw = getpwuid(geteuid())) {
        return QString::fromLocal8Bit(pw->pw_name);
    }
    if (const char* u = getenv("USER")) return QString::fromLocal8Bit(u);
    return QStringLiteral("user");
#endif
}

SingleInstanceGuard::SingleInstanceGuard(const QString& appKey, QObject* parent)
    : QObject(parent)
{
    m_socketName = QString("%1-singleton-%2").arg(appKey, currentUserName());

    // Probe: try to connect first. If the connect succeeds, a primary is alive.
    {
        QLocalSocket probe;
        probe.connectToServer(m_socketName);
        if (probe.waitForConnected(150)) {
            probe.disconnectFromServer();
            m_isSecondary = true;
            return;
        }
    }

    m_server = new QLocalServer(this);
    m_server->setSocketOptions(QLocalServer::UserAccessOption);

    if (!m_server->listen(m_socketName)) {
        // Stale socket from a crashed process — remove and retry once.
        QLocalServer::removeServer(m_socketName);
        if (!m_server->listen(m_socketName)) {
            Logger::error("SingleInstanceGuard: failed to listen on '" +
                          m_socketName.toStdString() + "': " +
                          m_server->errorString().toStdString());
            // Fall through: behave as primary so the app still launches.
        }
    }

    connect(m_server, &QLocalServer::newConnection, this, [this]() {
        QLocalSocket* sock = m_server->nextPendingConnection();
        if (!sock) return;
        sock->waitForReadyRead(50);
        sock->readAll();           // drain RAISE byte (we don't act on payload)
        sock->disconnectFromServer();
        sock->deleteLater();
        emit secondInstanceLaunched();
    });
}

SingleInstanceGuard::~SingleInstanceGuard() {
    if (m_server) {
        m_server->close();
    }
}

bool SingleInstanceGuard::notifyPrimary() {
    QLocalSocket sock;
    sock.connectToServer(m_socketName);
    if (!sock.waitForConnected(300)) return false;
    sock.write("RAISE");
    sock.flush();
    sock.waitForBytesWritten(300);
    sock.disconnectFromServer();
    return true;
}
