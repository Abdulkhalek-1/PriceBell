#include "gui/ProductDialog.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QLabel>
#include <QMessageBox>

ProductDialog::ProductDialog(PluginManager* pluginManager, QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Add Product"));
    resize(420, 540);
    setupUi(pluginManager);
}

ProductDialog::ProductDialog(PluginManager* pluginManager, const Product& existing, QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Edit Product"));
    resize(420, 540);
    setupUi(pluginManager);
    populateFrom(existing);
}

void ProductDialog::setupUi(PluginManager* pluginManager) {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // ── Basic info ────────────────────────────────────────────────────────────
    QGroupBox* infoGroup = new QGroupBox(tr("Product Info"), this);
    QFormLayout* infoForm = new QFormLayout(infoGroup);

    m_nameEdit = new QLineEdit(this);
    m_urlEdit  = new QLineEdit(this);
    m_urlEdit->setPlaceholderText("https://store.steampowered.com/app/730/...");

    m_sourceCombo = new QComboBox(this);
    for (const auto& src : pluginManager->availableSources()) {
        m_sourceCombo->addItem(QString::fromStdString(src.name));
        m_sourceIds.append(QString::fromStdString(src.id));
    }

    infoForm->addRow(tr("Name:"),   m_nameEdit);
    infoForm->addRow(tr("URL:"),    m_urlEdit);
    infoForm->addRow(tr("Source:"), m_sourceCombo);
    mainLayout->addWidget(infoGroup);

    // ── Filters ───────────────────────────────────────────────────────────────
    QGroupBox* filterGroup = new QGroupBox(tr("Alert Conditions"), this);
    QVBoxLayout* filterLayout = new QVBoxLayout(filterGroup);

    m_filtersList = new QListWidget(this);
    filterLayout->addWidget(m_filtersList);

    QHBoxLayout* addFilterRow = new QHBoxLayout();
    m_filterTypeCombo  = new QComboBox(this);
    m_filterTypeCombo->addItems({tr("Price ≤"), tr("Discount ≥")});
    m_filterValueSpin  = new QDoubleSpinBox(this);
    m_filterValueSpin->setRange(0.0, 10000.0);
    m_filterValueSpin->setDecimals(2);
    QPushButton* addFilterBtn    = new QPushButton(tr("+ Add"), this);
    QPushButton* removeFilterBtn = new QPushButton(tr("Remove"), this);
    addFilterRow->addWidget(m_filterTypeCombo);
    addFilterRow->addWidget(m_filterValueSpin);
    addFilterRow->addWidget(addFilterBtn);
    addFilterRow->addWidget(removeFilterBtn);
    filterLayout->addLayout(addFilterRow);
    mainLayout->addWidget(filterGroup);

    connect(addFilterBtn,    &QPushButton::clicked, this, &ProductDialog::addFilter);
    connect(removeFilterBtn, &QPushButton::clicked, this, &ProductDialog::removeFilter);

    // ── Interval ──────────────────────────────────────────────────────────────
    QGroupBox* intervalGroup = new QGroupBox(tr("Check Interval"), this);
    QFormLayout* intervalForm = new QFormLayout(intervalGroup);
    m_intervalSpin = new QSpinBox(this);
    m_intervalSpin->setRange(30, 86400);
    m_intervalSpin->setValue(3600);
    m_intervalSpin->setSuffix(tr(" sec"));
    intervalForm->addRow(tr("Interval:"), m_intervalSpin);
    mainLayout->addWidget(intervalGroup);

    // ── Buttons ───────────────────────────────────────────────────────────────
    QDialogButtonBox* buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    mainLayout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, this, [this]() {
        if (m_nameEdit->text().trimmed().isEmpty()) {
            QMessageBox::warning(this, tr("Validation"), tr("Product name cannot be empty."));
            return;
        }
        if (m_urlEdit->text().trimmed().isEmpty()) {
            QMessageBox::warning(this, tr("Validation"), tr("URL cannot be empty."));
            return;
        }
        accept();
    });
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void ProductDialog::populateFrom(const Product& product) {
    m_nameEdit->setText(QString::fromStdString(product.name));
    m_urlEdit->setText(QString::fromStdString(product.url));
    m_intervalSpin->setValue(static_cast<int>(product.checkInterval.count()));

    // Set source combo
    for (int i = 0; i < m_sourceIds.size(); ++i) {
        if (m_sourceIds[i].toStdString() ==
            (product.sourcePluginId.empty()
                ? [&]() -> std::string {
                    switch (product.source) {
                        case SourceType::STEAM:  return "steam";
                        case SourceType::UDEMY:  return "udemy";
                        case SourceType::AMAZON: return "amazon";
                        default:                 return "generic";
                    }
                }()
                : product.sourcePluginId)) {
            m_sourceCombo->setCurrentIndex(i);
            break;
        }
    }

    m_conditions = product.filters;
    for (const auto& c : m_conditions) {
        QString label = (c.type == ConditionType::PRICE_LESS_EQUAL)
            ? tr("Price ≤ $%1").arg(c.value, 0, 'f', 2)
            : tr("Discount ≥ %1%").arg(static_cast<int>(c.value));
        m_filtersList->addItem(label);
    }
}

void ProductDialog::addFilter() {
    ConditionType type = (m_filterTypeCombo->currentIndex() == 0)
        ? ConditionType::PRICE_LESS_EQUAL
        : ConditionType::DISCOUNT_GREATER_EQUAL;

    float value = static_cast<float>(m_filterValueSpin->value());
    m_conditions.push_back({0, type, value});

    QString label = (type == ConditionType::PRICE_LESS_EQUAL)
        ? tr("Price ≤ $%1").arg(value, 0, 'f', 2)
        : tr("Discount ≥ %1%").arg(static_cast<int>(value));
    m_filtersList->addItem(label);
}

void ProductDialog::removeFilter() {
    int row = m_filtersList->currentRow();
    if (row >= 0) {
        delete m_filtersList->takeItem(row);
        m_conditions.erase(m_conditions.begin() + row);
    }
}

Product ProductDialog::getProduct() const {
    Product p;
    p.name          = m_nameEdit->text().trimmed().toStdString();
    p.url           = m_urlEdit->text().trimmed().toStdString();
    p.checkInterval = std::chrono::seconds(m_intervalSpin->value());
    p.lastChecked   = std::chrono::system_clock::now();
    p.filters       = m_conditions;
    p.isActive      = true;

    int idx = m_sourceCombo->currentIndex();
    if (idx >= 0 && idx < m_sourceIds.size()) {
        QString srcId = m_sourceIds[idx];
        if (srcId == "steam")       p.source = SourceType::STEAM;
        else if (srcId == "udemy")  p.source = SourceType::UDEMY;
        else if (srcId == "amazon") p.source = SourceType::AMAZON;
        else if (srcId == "generic") p.source = SourceType::GENERIC;
        else {
            p.source        = SourceType::PLUGIN;
            p.sourcePluginId = srcId.toStdString();
        }
    }
    return p;
}
