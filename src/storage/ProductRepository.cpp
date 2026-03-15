#include "storage/ProductRepository.hpp"
#include "storage/Database.hpp"
#include "utils/Logger.hpp"

#include <QSqlQuery>
#include <QSqlError>
#include <QSqlDatabase>
#include <QVariant>
#include <QMap>
#include <chrono>

// ── Helpers ───────────────────────────────────────────────────────────────────

std::vector<PriceCondition> ProductRepository::loadConditions(int productId) {
    std::vector<PriceCondition> conditions;
    QSqlQuery q(Database::connection());
    q.prepare("SELECT id, condition_type, value FROM price_conditions WHERE product_id = :pid");
    q.bindValue(":pid", productId);
    if (!q.exec()) {
        Logger::error("loadConditions failed: " + q.lastError().text().toStdString());
        return conditions;
    }
    while (q.next()) {
        PriceCondition c;
        c.id    = q.value(0).toInt();
        c.type  = static_cast<ConditionType>(q.value(1).toInt());
        c.value = q.value(2).toFloat();
        conditions.push_back(c);
    }
    return conditions;
}

bool ProductRepository::saveConditions(int productId, const std::vector<PriceCondition>& conditions) {
    QSqlQuery del(Database::connection());
    del.prepare("DELETE FROM price_conditions WHERE product_id = :pid");
    del.bindValue(":pid", productId);
    if (!del.exec()) {
        Logger::error("deleteConditions failed: " + del.lastError().text().toStdString());
        return false;
    }
    for (const auto& c : conditions) {
        QSqlQuery ins(Database::connection());
        ins.prepare("INSERT INTO price_conditions(product_id, condition_type, value) VALUES(:pid, :type, :val)");
        ins.bindValue(":pid",  productId);
        ins.bindValue(":type", static_cast<int>(c.type));
        ins.bindValue(":val",  c.value);
        if (!ins.exec()) {
            Logger::error("insertCondition failed: " + ins.lastError().text().toStdString());
            return false;
        }
    }
    return true;
}

// ── Public API ────────────────────────────────────────────────────────────────

bool ProductRepository::save(Product& product) {
    QSqlDatabase db = Database::connection();
    db.transaction();

    QSqlQuery q(db);
    q.prepare(
        "INSERT INTO products(name, url, source_id, current_price, discount, is_active, check_interval, last_checked, currency) "
        "VALUES(:name, :url, :src, :price, :disc, :active, :interval, :checked, :currency)"
    );
    q.bindValue(":name",     QString::fromStdString(product.name));
    q.bindValue(":url",      QString::fromStdString(product.url));
    q.bindValue(":src",      product.sourcePluginId.empty()
                               ? QString::fromStdString(sourceTypeToId(product.source))
                               : QString::fromStdString(product.sourcePluginId));
    q.bindValue(":price",    product.currentPrice);
    q.bindValue(":disc",     product.discount);
    q.bindValue(":active",   product.isActive ? 1 : 0);
    q.bindValue(":interval", static_cast<int>(product.checkInterval.count()));
    q.bindValue(":checked",  static_cast<long long>(
        std::chrono::system_clock::to_time_t(product.lastChecked)));
    q.bindValue(":currency", QString::fromStdString(product.currency));

    if (!q.exec()) {
        Logger::error("save product failed: " + q.lastError().text().toStdString());
        db.rollback();
        return false;
    }
    product.id = q.lastInsertId().toInt();

    if (!saveConditions(product.id, product.filters)) {
        db.rollback();
        return false;
    }

    db.commit();
    return true;
}

std::vector<Product> ProductRepository::findAll() {
    std::vector<Product> products;

    // Query 1: Load all products
    QSqlQuery q("SELECT id, name, url, source_id, current_price, discount, is_active, check_interval, last_checked, currency "
                "FROM products ORDER BY id ASC",
                Database::connection());
    if (!q.exec()) {
        Logger::error("findAll failed: " + q.lastError().text().toStdString());
        return products;
    }
    while (q.next()) {
        Product p;
        p.id            = q.value(0).toInt();
        p.name          = q.value(1).toString().toStdString();
        p.url           = q.value(2).toString().toStdString();
        std::string srcId = q.value(3).toString().toStdString();
        p.source        = sourceIdToType(srcId);
        if (p.source == SourceType::PLUGIN)
            p.sourcePluginId = srcId;
        p.currentPrice  = q.value(4).toFloat();
        p.discount      = q.value(5).toFloat();
        p.isActive      = q.value(6).toInt() != 0;
        p.checkInterval = std::chrono::seconds(q.value(7).toInt());
        p.lastChecked   = std::chrono::system_clock::from_time_t(q.value(8).toLongLong());
        p.currency      = q.value(9).toString().toStdString();
        if (p.currency.empty()) p.currency = "USD";
        products.push_back(std::move(p));
    }

    // Query 2: Load ALL conditions in one query (N+1 fix)
    QSqlQuery cq("SELECT id, product_id, condition_type, value FROM price_conditions ORDER BY product_id",
                 Database::connection());
    if (!cq.exec()) {
        Logger::error("findAll conditions failed: " + cq.lastError().text().toStdString());
        return products; // return products without conditions rather than failing entirely
    }

    QMap<int, std::vector<PriceCondition>> conditionsMap;
    while (cq.next()) {
        PriceCondition pc;
        pc.id    = cq.value(0).toInt();
        pc.type  = static_cast<ConditionType>(cq.value(2).toInt());
        pc.value = cq.value(3).toFloat();
        int pid  = cq.value(1).toInt();
        conditionsMap[pid].push_back(pc);
    }

    // Assign conditions to products
    for (auto& p : products) {
        if (conditionsMap.contains(p.id)) {
            p.filters = std::move(conditionsMap[p.id]);
        }
    }

    return products;
}

std::optional<Product> ProductRepository::findById(int id) {
    QSqlQuery q(Database::connection());
    q.prepare("SELECT id, name, url, source_id, current_price, discount, is_active, check_interval, last_checked, currency "
              "FROM products WHERE id = :id");
    q.bindValue(":id", id);
    if (!q.exec() || !q.next()) return std::nullopt;

    Product p;
    p.id            = q.value(0).toInt();
    p.name          = q.value(1).toString().toStdString();
    p.url           = q.value(2).toString().toStdString();
    std::string srcId = q.value(3).toString().toStdString();
    p.source        = sourceIdToType(srcId);
    if (p.source == SourceType::PLUGIN)
        p.sourcePluginId = srcId;
    p.currentPrice  = q.value(4).toFloat();
    p.discount      = q.value(5).toFloat();
    p.isActive      = q.value(6).toInt() != 0;
    p.checkInterval = std::chrono::seconds(q.value(7).toInt());
    p.lastChecked   = std::chrono::system_clock::from_time_t(q.value(8).toLongLong());
    p.currency      = q.value(9).toString().toStdString();
    if (p.currency.empty()) p.currency = "USD";
    p.filters       = loadConditions(p.id);
    return p;
}

bool ProductRepository::update(const Product& product) {
    QSqlDatabase db = Database::connection();
    db.transaction();

    QSqlQuery q(db);
    q.prepare(
        "UPDATE products SET name=:name, url=:url, source_id=:src, current_price=:price, "
        "discount=:disc, is_active=:active, check_interval=:interval, last_checked=:checked, "
        "currency=:currency WHERE id=:id"
    );
    q.bindValue(":name",     QString::fromStdString(product.name));
    q.bindValue(":url",      QString::fromStdString(product.url));
    q.bindValue(":src",      product.sourcePluginId.empty()
                               ? QString::fromStdString(sourceTypeToId(product.source))
                               : QString::fromStdString(product.sourcePluginId));
    q.bindValue(":price",    product.currentPrice);
    q.bindValue(":disc",     product.discount);
    q.bindValue(":active",   product.isActive ? 1 : 0);
    q.bindValue(":interval", static_cast<int>(product.checkInterval.count()));
    q.bindValue(":checked",  static_cast<long long>(
        std::chrono::system_clock::to_time_t(product.lastChecked)));
    q.bindValue(":currency", QString::fromStdString(product.currency));
    q.bindValue(":id",       product.id);

    if (!q.exec()) {
        Logger::error("update product failed: " + q.lastError().text().toStdString());
        db.rollback();
        return false;
    }

    if (!saveConditions(product.id, product.filters)) {
        db.rollback();
        return false;
    }

    db.commit();
    return true;
}

bool ProductRepository::remove(int id) {
    QSqlDatabase db = Database::connection();
    db.transaction();

    // Delete conditions first (even though CASCADE would handle it,
    // explicit delete ensures we control the transaction boundary)
    QSqlQuery delCond(db);
    delCond.prepare("DELETE FROM price_conditions WHERE product_id = :id");
    delCond.bindValue(":id", id);
    if (!delCond.exec()) {
        Logger::error("remove conditions failed: " + delCond.lastError().text().toStdString());
        db.rollback();
        return false;
    }

    QSqlQuery q(db);
    q.prepare("DELETE FROM products WHERE id = :id");
    q.bindValue(":id", id);
    if (!q.exec()) {
        Logger::error("remove product failed: " + q.lastError().text().toStdString());
        db.rollback();
        return false;
    }

    db.commit();
    return true;
}
