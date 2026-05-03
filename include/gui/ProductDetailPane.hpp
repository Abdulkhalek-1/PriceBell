#pragma once

#include "core/DataStructs.hpp"

#include <QFrame>

class QLabel;
class QPushButton;
class QVBoxLayout;
class AppController;

// Right-hand info pane shown alongside the table view. Displays full product
// details for the currently-selected row: name, URL, conditions, interval,
// recent alert history. Empty state when no product is selected.
class ProductDetailPane : public QFrame {
    Q_OBJECT
public:
    explicit ProductDetailPane(AppController* controller, QWidget* parent = nullptr);

    void showProduct(const Product& product);
    void clear();

signals:
    void editRequested(int productId);
    void checkNowRequested(int productId);
    void pauseToggleRequested(int productId);

private:
    AppController* m_controller;
    int            m_currentId = -1;

    QLabel*  m_titleLabel;
    QLabel*  m_urlLabel;
    QPushButton* m_openUrlBtn;
    QLabel*  m_priceLabel;
    QLabel*  m_originalPriceLabel;
    QLabel*  m_discountLabel;
    QLabel*  m_statusLabel;
    QLabel*  m_intervalLabel;
    QLabel*  m_conditionsLabel;
    QLabel*  m_alertsLabel;
    QPushButton* m_editBtn;
    QPushButton* m_pauseBtn;
    QPushButton* m_checkBtn;
    QLabel*  m_emptyState;
};
