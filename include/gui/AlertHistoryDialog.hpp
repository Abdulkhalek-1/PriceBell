#pragma once

#include "core/DataStructs.hpp"
#include <QDialog>
#include <QTableWidget>
#include <vector>

class AlertHistoryDialog : public QDialog {
    Q_OBJECT
public:
    explicit AlertHistoryDialog(QWidget* parent = nullptr);

    // Reloads alert history from the database and refreshes the table.
    void refresh();

signals:
    // Emitted after the user dismisses an alert. MainWindow forwards to
    // AlertManager so the next matching tick re-notifies for that product.
    void alertDismissed(int productId);

private slots:
    void dismissSelected();
    void filterByProduct(int productId);

private:
    void setupUi();
    void populateTable(const std::vector<AlertEvent>& events);

    QTableWidget* m_table;
    int           m_filterProductId = -1; // -1 = show all
};
