#include "gui/ProductDetailPane.hpp"
#include "core/AppController.hpp"
#include "storage/AlertRepository.hpp"
#include "utils/CurrencyUtils.hpp"

#include <QDateTime>
#include <QDesktopServices>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QSizePolicy>
#include <QStackedLayout>
#include <QUrl>
#include <QVBoxLayout>

ProductDetailPane::ProductDetailPane(AppController* controller, QWidget* parent)
    : QFrame(parent), m_controller(controller)
{
    setObjectName("productDetailPane");
    setFrameShape(QFrame::StyledPanel);
    setMinimumWidth(240);

    // Outer layout = scroll area + persistent action bar at the bottom.
    auto* outer = new QVBoxLayout(this);
    outer->setContentsMargins(0, 0, 0, 0);
    outer->setSpacing(0);

    auto* scroll = new QScrollArea(this);
    scroll->setObjectName("detailScroll");
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    auto* content = new QWidget(scroll);
    auto* root = new QVBoxLayout(content);
    root->setContentsMargins(14, 14, 14, 14);
    root->setSpacing(8);

    m_titleLabel = new QLabel(content);
    m_titleLabel->setObjectName("detailTitle");
    m_titleLabel->setWordWrap(true);
    m_titleLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    root->addWidget(m_titleLabel);

    auto* urlRow = new QHBoxLayout();
    urlRow->setSpacing(6);
    m_urlLabel = new QLabel(content);
    m_urlLabel->setObjectName("detailUrl");
    m_urlLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_urlLabel->setWordWrap(true);
    m_urlLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    urlRow->addWidget(m_urlLabel, 1);
    m_openUrlBtn = new QPushButton(tr("Open"), content);
    m_openUrlBtn->setMaximumHeight(24);
    m_openUrlBtn->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    urlRow->addWidget(m_openUrlBtn, 0, Qt::AlignTop);
    root->addLayout(urlRow);

    auto* priceRow = new QHBoxLayout();
    priceRow->setSpacing(8);
    m_priceLabel = new QLabel(content);
    QFont priceF = m_priceLabel->font();
    priceF.setPointSize(priceF.pointSize() + 4);
    priceF.setBold(true);
    m_priceLabel->setFont(priceF);
    m_priceLabel->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
    priceRow->addWidget(m_priceLabel, 0, Qt::AlignVCenter);

    m_originalPriceLabel = new QLabel(content);
    m_originalPriceLabel->setObjectName("priceOriginal");
    m_originalPriceLabel->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Preferred);
    priceRow->addWidget(m_originalPriceLabel, 0, Qt::AlignBottom);

    m_discountLabel = new QLabel(content);
    m_discountLabel->setObjectName("chipDiscount");
    m_discountLabel->setMaximumHeight(20);
    m_discountLabel->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
    priceRow->addWidget(m_discountLabel, 0, Qt::AlignVCenter);
    priceRow->addStretch(1);
    root->addLayout(priceRow);

    auto* statusSection = new QLabel(tr("Status"), content);
    statusSection->setObjectName("detailSection");
    root->addWidget(statusSection);
    m_statusLabel = new QLabel(content);
    m_statusLabel->setWordWrap(true);
    root->addWidget(m_statusLabel);

    auto* condSection = new QLabel(tr("Alert conditions"), content);
    condSection->setObjectName("detailSection");
    root->addWidget(condSection);
    m_conditionsLabel = new QLabel(content);
    m_conditionsLabel->setWordWrap(true);
    m_conditionsLabel->setStyleSheet("color: #cdd6f4;");
    root->addWidget(m_conditionsLabel);

    auto* intervalSection = new QLabel(tr("Check interval"), content);
    intervalSection->setObjectName("detailSection");
    root->addWidget(intervalSection);
    m_intervalLabel = new QLabel(content);
    m_intervalLabel->setStyleSheet("color: #cdd6f4;");
    m_intervalLabel->setWordWrap(true);
    root->addWidget(m_intervalLabel);

    auto* alertsSection = new QLabel(tr("Recent alerts"), content);
    alertsSection->setObjectName("detailSection");
    root->addWidget(alertsSection);
    m_alertsLabel = new QLabel(content);
    m_alertsLabel->setWordWrap(true);
    m_alertsLabel->setStyleSheet("color: #cdd6f4; font-size: 11px;");
    root->addWidget(m_alertsLabel);

    root->addStretch();
    scroll->setWidget(content);
    outer->addWidget(scroll, 1);

    // Action bar — pinned to the bottom of the pane, never scrolls.
    auto* actionBar = new QFrame(this);
    actionBar->setObjectName("detailActionBar");
    auto* btnRow = new QHBoxLayout(actionBar);
    btnRow->setContentsMargins(14, 8, 14, 10);
    btnRow->setSpacing(6);
    m_editBtn  = new QPushButton(tr("Edit"), actionBar);
    m_pauseBtn = new QPushButton(tr("Pause"), actionBar);
    m_checkBtn = new QPushButton(tr("Check Now"), actionBar);
    for (auto* b : {m_editBtn, m_pauseBtn, m_checkBtn}) {
        b->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        b->setMinimumWidth(0);
    }
    btnRow->addWidget(m_editBtn);
    btnRow->addWidget(m_pauseBtn);
    btnRow->addWidget(m_checkBtn);
    outer->addWidget(actionBar);

    m_emptyState = new QLabel(tr("Select a product to see its details."), this);
    m_emptyState->setAlignment(Qt::AlignCenter);
    m_emptyState->setStyleSheet("color: #6c7086; padding: 40px;");
    m_emptyState->setVisible(false);

    connect(m_openUrlBtn, &QPushButton::clicked, this, [this]() {
        QString u = m_urlLabel->text();
        if (!u.isEmpty()) QDesktopServices::openUrl(QUrl(u));
    });
    connect(m_editBtn,  &QPushButton::clicked, this, [this]() {
        if (m_currentId > 0) emit editRequested(m_currentId);
    });
    connect(m_checkBtn, &QPushButton::clicked, this, [this]() {
        if (m_currentId > 0) emit checkNowRequested(m_currentId);
    });
    connect(m_pauseBtn, &QPushButton::clicked, this, [this]() {
        if (m_currentId > 0) emit pauseToggleRequested(m_currentId);
    });

    clear();
}

void ProductDetailPane::clear() {
    m_currentId = -1;
    m_titleLabel->setText(tr("No product selected"));
    m_urlLabel->setText("");
    m_priceLabel->setText("");
    m_originalPriceLabel->setText("");
    m_discountLabel->setText("");
    m_discountLabel->setVisible(false);
    m_statusLabel->setText("");
    m_intervalLabel->setText("");
    m_conditionsLabel->setText("");
    m_alertsLabel->setText("");
    m_openUrlBtn->setEnabled(false);
    m_editBtn->setEnabled(false);
    m_pauseBtn->setEnabled(false);
    m_checkBtn->setEnabled(false);
}

void ProductDetailPane::showProduct(const Product& product) {
    m_currentId = product.id;

    m_titleLabel->setText(QString::fromStdString(product.name));
    m_urlLabel->setText(QString::fromStdString(product.url));
    m_openUrlBtn->setEnabled(true);
    m_editBtn->setEnabled(true);
    m_pauseBtn->setEnabled(true);
    m_pauseBtn->setText(product.isActive ? tr("Pause") : tr("Resume"));
    m_checkBtn->setEnabled(true);

    if (product.currentPrice > 0) {
        m_priceLabel->setText(CurrencyUtils::formatPrice(product.currentPrice, product.currency));
    } else {
        m_priceLabel->setText(tr("—"));
    }
    if (product.originalPrice > 0 && product.originalPrice > product.currentPrice) {
        m_originalPriceLabel->setText(CurrencyUtils::formatPrice(product.originalPrice, product.currency));
    } else {
        m_originalPriceLabel->setText("");
    }
    if (product.discount > 0) {
        m_discountLabel->setText(QString(" -%1% ").arg(static_cast<int>(product.discount)));
        m_discountLabel->setVisible(true);
    } else {
        m_discountLabel->setVisible(false);
    }

    if (!product.isActive) {
        m_statusLabel->setText(tr("⏸  Paused"));
        m_statusLabel->setStyleSheet("color: #6c7086;");
    } else {
        m_statusLabel->setText(tr("✓  Watching"));
        m_statusLabel->setStyleSheet("color: #a6e3a1;");
    }

    QStringList conds;
    for (const auto& c : product.filters) {
        if (c.type == ConditionType::PRICE_LESS_EQUAL) {
            conds << tr("Price ≤ %1").arg(CurrencyUtils::formatPrice(c.value, product.currency));
        } else {
            conds << tr("Discount ≥ %1%").arg(static_cast<int>(c.value));
        }
    }
    m_conditionsLabel->setText(conds.isEmpty() ? tr("None") : conds.join("\n"));

    long secs = static_cast<long>(product.checkInterval.count());
    if (secs >= 3600 && secs % 3600 == 0) {
        m_intervalLabel->setText(tr("Every %1 h").arg(secs / 3600));
    } else if (secs >= 60 && secs % 60 == 0) {
        m_intervalLabel->setText(tr("Every %1 min").arg(secs / 60));
    } else {
        m_intervalLabel->setText(tr("Every %1 s").arg(secs));
    }

    auto alerts = AlertRepository::findByProduct(product.id);
    QStringList lines;
    int n = std::min<int>(5, static_cast<int>(alerts.size()));
    for (int i = 0; i < n; ++i) {
        const auto& e = alerts[i];
        auto t = std::chrono::system_clock::to_time_t(e.triggeredAt);
        QString when = QDateTime::fromSecsSinceEpoch(static_cast<qint64>(t))
                           .toString("yyyy-MM-dd HH:mm");
        QString priceStr = CurrencyUtils::formatPrice(e.priceAtTrigger, product.currency);
        QString tag = (e.status == AlertStatus::DISMISSED) ? tr("dismissed") : tr("active");
        lines << QString("• %1 — %2 (%3)").arg(when, priceStr, tag);
    }
    m_alertsLabel->setText(lines.isEmpty() ? tr("No alerts yet.") : lines.join("\n"));
}
