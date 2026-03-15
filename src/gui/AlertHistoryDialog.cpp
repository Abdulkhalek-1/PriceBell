#include "gui/AlertHistoryDialog.hpp"
#include "storage/AlertRepository.hpp"
#include "utils/CurrencyUtils.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QHeaderView>
#include <QTableWidgetItem>
#include <QDateTime>
#include <QLabel>

AlertHistoryDialog::AlertHistoryDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Alert History"));
    resize(700, 400);
    setupUi();
    refresh();
}

void AlertHistoryDialog::setupUi() {
    QVBoxLayout* layout = new QVBoxLayout(this);

    QLabel* title = new QLabel(tr("Past Alerts"), this);
    title->setObjectName("dialogTitle");
    layout->addWidget(title);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(5);
    m_table->setHorizontalHeaderLabels({tr("Product"), tr("Price"), tr("Discount %"), tr("Triggered At"), tr("Status")});
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setAlternatingRowColors(true);
    layout->addWidget(m_table);

    // Tooltips for table headers
    m_table->horizontalHeaderItem(0)->setToolTip(tr("Product that triggered the alert"));
    m_table->horizontalHeaderItem(1)->setToolTip(tr("Price at the time the alert was triggered"));
    m_table->horizontalHeaderItem(2)->setToolTip(tr("Discount percentage at trigger time"));
    m_table->horizontalHeaderItem(3)->setToolTip(tr("Date and time the alert was triggered"));
    m_table->horizontalHeaderItem(4)->setToolTip(tr("Whether the alert is still active or dismissed"));

    QHBoxLayout* btnLayout = new QHBoxLayout();
    QPushButton* dismissBtn = new QPushButton(tr("Dismiss Selected"), this);
    dismissBtn->setToolTip(tr("Mark the selected alert as dismissed"));
    QPushButton* refreshBtn = new QPushButton(tr("Refresh"), this);
    refreshBtn->setToolTip(tr("Reload alert history from database"));
    QPushButton* closeBtn   = new QPushButton(tr("Close"), this);
    closeBtn->setToolTip(tr("Close this dialog"));

    btnLayout->addWidget(dismissBtn);
    btnLayout->addStretch();
    btnLayout->addWidget(refreshBtn);
    btnLayout->addWidget(closeBtn);
    layout->addLayout(btnLayout);

    connect(dismissBtn, &QPushButton::clicked, this, &AlertHistoryDialog::dismissSelected);
    connect(refreshBtn, &QPushButton::clicked, this, &AlertHistoryDialog::refresh);
    connect(closeBtn,   &QPushButton::clicked, this, &QDialog::accept);
}

void AlertHistoryDialog::refresh() {
    auto events = (m_filterProductId == -1)
        ? AlertRepository::findAll()
        : AlertRepository::findByProduct(m_filterProductId);
    populateTable(events);
}

void AlertHistoryDialog::populateTable(const std::vector<AlertEvent>& events) {
    m_table->setRowCount(0);
    for (const auto& e : events) {
        int row = m_table->rowCount();
        m_table->insertRow(row);

        auto item = [](const QString& text) {
            auto* i = new QTableWidgetItem(text);
            i->setTextAlignment(Qt::AlignCenter);
            return i;
        };

        auto time_t = std::chrono::system_clock::to_time_t(e.triggeredAt);
        QString timeStr = QDateTime::fromSecsSinceEpoch(static_cast<qint64>(time_t))
            .toString("yyyy-MM-dd HH:mm:ss");

        m_table->setItem(row, 0, item(QString::fromStdString(e.productName)));
        m_table->setItem(row, 1, item(CurrencyUtils::formatPrice(e.priceAtTrigger, "USD")));
        m_table->setItem(row, 2, item(QString("%1%").arg(static_cast<int>(e.discountAtTrigger))));
        m_table->setItem(row, 3, item(timeStr));
        m_table->setItem(row, 4, item(e.status == AlertStatus::DISMISSED ? tr("Dismissed") : tr("Active")));

        // Store alert id in hidden role for dismiss action
        m_table->item(row, 0)->setData(Qt::UserRole, e.id);

        if (e.status == AlertStatus::TRIGGERED)
            m_table->item(row, 4)->setForeground(QColor("#a6e3a1")); // green
        else
            m_table->item(row, 4)->setForeground(QColor("#6c7086")); // grey
    }
}

void AlertHistoryDialog::dismissSelected() {
    int row = m_table->currentRow();
    if (row < 0) return;

    int alertId = m_table->item(row, 0)->data(Qt::UserRole).toInt();
    AlertRepository::dismiss(alertId);
    refresh();
}

void AlertHistoryDialog::filterByProduct(int productId) {
    m_filterProductId = productId;
    refresh();
}
