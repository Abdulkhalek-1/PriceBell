#pragma once

#include <QStyledItemDelegate>

// Renders a single product card. Reads display data from custom roles defined
// in ProductCardView::Roles. Catppuccin-inspired colors match dark.qss.
class ProductCardDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    explicit ProductCardDelegate(QObject* parent = nullptr);

    void paint(QPainter* painter, const QStyleOptionViewItem& option,
               const QModelIndex& index) const override;
    QSize sizeHint(const QStyleOptionViewItem& option,
                   const QModelIndex& index) const override;
};
