#pragma once

#include "core/DataStructs.hpp"
#include <QString>
#include <vector>
#include <optional>

class ProductRepository {
public:
    // Inserts a new product and its conditions. Sets product.id on success.
    static bool save(Product& product);

    // Returns all products with their conditions loaded.
    static std::vector<Product> findAll();

    // Returns the product with the given id, or nullopt if not found.
    static std::optional<Product> findById(int id);

    // Updates an existing product's fields and replaces its conditions.
    static bool update(const Product& product);

    // Removes the product (cascade deletes conditions and alert history).
    static bool remove(int id);

private:
    static std::vector<PriceCondition> loadConditions(int productId);
    static bool saveConditions(int productId, const std::vector<PriceCondition>& conditions);
};
