#pragma once

#include "core/DataStructs.hpp"

#include <QListWidget>
#include <vector>

// Card-based view of tracked products. Rendering is done by ProductCardDelegate
// using the data stored in each item via custom Qt::UserRole+N roles.
//
// Selection model mirrors the QTableWidget API used elsewhere — selectedIds()
// returns the product ids of all selected cards (multi-select supported).
class ProductCardView : public QListWidget {
    Q_OBJECT
public:
    enum Roles {
        ProductIdRole = Qt::UserRole + 1,
        NameRole,
        UrlHostRole,
        SourceIdRole,         // "steam", "udemy", "amazon", or plugin id
        SourceIconPathRole,
        PriceRole,            // formatted string with currency
        OriginalPriceRole,    // formatted strikethrough; empty if no discount
        DiscountRole,         // int 0-100, -1 if none
        StatusTextRole,
        StatusColorRole,      // QColor
        LastCheckedRelRole,   // "2m ago"
        CurrencyRole,
    };

    explicit ProductCardView(QWidget* parent = nullptr);

    void setProducts(const std::vector<Product>& products);
    void updatePriceFor(int productId, float newPrice, float newDiscount,
                        const std::string& currency);
    void setStatusFor(int productId, const QString& text, const QColor& color);

    // Row index → product id, mirroring the existing m_table convention.
    int productIdAt(int row) const;
    QList<int> selectedProductIds() const;

signals:
    void productActivated(int productId);   // double-clicked
    void productContextRequested(int productId, const QPoint& globalPos);

protected:
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;

private:
    int findRow(int productId) const;
    static QString relativeTime(const std::chrono::system_clock::time_point& tp);
};
