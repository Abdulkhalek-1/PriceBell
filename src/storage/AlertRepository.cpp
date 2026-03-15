#include "storage/AlertRepository.hpp"
#include "storage/Database.hpp"
#include "utils/Logger.hpp"

#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>
#include <chrono>

bool AlertRepository::save(AlertEvent& event) {
    QSqlQuery q(Database::connection());
    q.prepare(
        "INSERT INTO alert_history(product_id, product_name, price_at_trigger, discount_at_trigger, triggered_at, status) "
        "VALUES(:pid, :pname, :price, :disc, :time, :status)"
    );
    q.bindValue(":pid",    event.productId);
    q.bindValue(":pname",  QString::fromStdString(event.productName));
    q.bindValue(":price",  event.priceAtTrigger);
    q.bindValue(":disc",   event.discountAtTrigger);
    q.bindValue(":time",   static_cast<long long>(
        std::chrono::system_clock::to_time_t(event.triggeredAt)));
    q.bindValue(":status", static_cast<int>(event.status));

    if (!q.exec()) {
        Logger::error("save alert failed: " + q.lastError().text().toStdString());
        return false;
    }
    event.id = q.lastInsertId().toInt();
    return true;
}

static std::vector<AlertEvent> queryAlerts(QSqlQuery& q) {
    std::vector<AlertEvent> events;
    while (q.next()) {
        AlertEvent e;
        e.id                = q.value(0).toInt();
        e.productId         = q.value(1).toInt();
        e.productName       = q.value(2).toString().toStdString();
        e.priceAtTrigger    = q.value(3).toFloat();
        e.discountAtTrigger = q.value(4).toFloat();
        e.triggeredAt       = std::chrono::system_clock::from_time_t(q.value(5).toLongLong());
        e.status            = static_cast<AlertStatus>(q.value(6).toInt());
        events.push_back(e);
    }
    return events;
}

std::vector<AlertEvent> AlertRepository::findAll() {
    QSqlQuery q("SELECT id, product_id, product_name, price_at_trigger, discount_at_trigger, triggered_at, status "
                "FROM alert_history ORDER BY triggered_at DESC",
                Database::connection());
    if (!q.exec()) {
        Logger::error("findAll alerts failed: " + q.lastError().text().toStdString());
        return {};
    }
    return queryAlerts(q);
}

std::vector<AlertEvent> AlertRepository::findByProduct(int productId) {
    QSqlQuery q(Database::connection());
    q.prepare("SELECT id, product_id, product_name, price_at_trigger, discount_at_trigger, triggered_at, status "
              "FROM alert_history WHERE product_id = :pid ORDER BY triggered_at DESC");
    q.bindValue(":pid", productId);
    if (!q.exec()) {
        Logger::error("findByProduct alerts failed: " + q.lastError().text().toStdString());
        return {};
    }
    return queryAlerts(q);
}

bool AlertRepository::dismiss(int alertId) {
    QSqlQuery q(Database::connection());
    q.prepare("UPDATE alert_history SET status = 1 WHERE id = :id");
    q.bindValue(":id", alertId);
    if (!q.exec()) {
        Logger::error("dismiss alert failed: " + q.lastError().text().toStdString());
        return false;
    }
    return true;
}

bool AlertRepository::removeByProduct(int productId) {
    QSqlQuery q(Database::connection());
    q.prepare("DELETE FROM alert_history WHERE product_id = :pid");
    q.bindValue(":pid", productId);
    if (!q.exec()) {
        Logger::error("removeByProduct alerts failed: " + q.lastError().text().toStdString());
        return false;
    }
    return true;
}
