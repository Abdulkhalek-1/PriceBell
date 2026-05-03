#include "gui/ProductDialog.hpp"
#include "utils/CurrencyUtils.hpp"
#include "utils/HttpClient.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QDialogButtonBox>
#include <QLabel>
#include <QMessageBox>
#include <QPointer>
#include <QtConcurrent/QtConcurrent>

ProductDialog::ProductDialog(PluginManager* pluginManager, QWidget* parent)
    : QDialog(parent), m_pluginManager(pluginManager)
{
    setWindowTitle(tr("Add Product"));
    resize(440, 580);
    setupUi(pluginManager);
}

ProductDialog::ProductDialog(PluginManager* pluginManager, const Product& existing, QWidget* parent)
    : QDialog(parent), m_pluginManager(pluginManager)
{
    setWindowTitle(tr("Edit Product"));
    resize(440, 580);
    setupUi(pluginManager);
    populateFrom(existing);
    // Editing an existing product: name was already user-chosen, never overwrite.
    m_userTouchedName = true;
}

void ProductDialog::setupUi(PluginManager* pluginManager) {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // -- Basic info ---------------------------------------------------------------
    QGroupBox* infoGroup = new QGroupBox(tr("Product Info"), this);
    QFormLayout* infoForm = new QFormLayout(infoGroup);

    m_nameEdit = new QLineEdit(this);
    m_nameEdit->setToolTip(tr("Enter a display name for this product"));

    m_urlEdit  = new QLineEdit(this);
    m_urlEdit->setPlaceholderText("https://store.steampowered.com/app/730/...");
    m_urlEdit->setToolTip(tr("Paste the product URL — source and name will be auto-detected"));

    // Detect-status label always reserves a single line so the form below
    // doesn't shift up/down when the message appears or disappears.
    m_detectStatus = new QLabel(QStringLiteral(" "), this);
    m_detectStatus->setStyleSheet("color: #6c7086; font-size: 11px;");
    QFontMetrics dsFm(m_detectStatus->font());
    m_detectStatus->setFixedHeight(dsFm.height() + 2);
    m_detectStatus->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    m_sourceCombo = new QComboBox(this);
    m_sourceCombo->setToolTip(tr("Select the price source type"));
    for (const auto& src : pluginManager->availableSources()) {
        m_sourceCombo->addItem(QString::fromStdString(src.name));
        m_sourceIds.append(QString::fromStdString(src.id));
    }

    infoForm->addRow(tr("Name:"),   m_nameEdit);
    infoForm->addRow(tr("URL:"),    m_urlEdit);
    // Use a span row so the status text aligns with the URL field, not the
    // label column. spanningRow keeps the form's column widths stable.
    infoForm->addRow(m_detectStatus);
    infoForm->addRow(tr("Source:"), m_sourceCombo);
    mainLayout->addWidget(infoGroup);

    // Track whether the user has typed a name themselves so auto-detect doesn't
    // overwrite it. textEdited fires only on user edits, not setText().
    connect(m_nameEdit, &QLineEdit::textEdited, this, [this](const QString& t) {
        m_userTouchedName = !t.trimmed().isEmpty();
    });

    // Debounced URL auto-detect. 400 ms is short enough to feel instant after
    // a paste but long enough to avoid firing on every keystroke during typing.
    m_urlDebounce = new QTimer(this);
    m_urlDebounce->setSingleShot(true);
    m_urlDebounce->setInterval(400);
    connect(m_urlDebounce, &QTimer::timeout, this, &ProductDialog::runUrlDetect);
    connect(m_urlEdit, &QLineEdit::textChanged, this, &ProductDialog::onUrlChanged);

    connect(this, &ProductDialog::detectFinished,
            this, [this](QString sourceId, QString name, QString /*currency*/) {
        if (sourceId.isEmpty()) {
            m_detectStatus->setText(tr("Couldn't auto-detect source from this URL."));
            return;
        }
        selectSourceById(sourceId);
        QString status = tr("Detected: %1").arg(sourceId);
        if (!name.isEmpty()) {
            status += QString(" — %1").arg(name);
            if (!m_userTouchedName) {
                m_nameEdit->setText(name);
            }
        }
        // Elide so a long product name doesn't widen the dialog.
        QFontMetrics fm(m_detectStatus->font());
        m_detectStatus->setText(fm.elidedText(status, Qt::ElideRight,
            std::max(120, m_detectStatus->width() - 8)));
        m_detectStatus->setToolTip(status);
    }, Qt::QueuedConnection);

    // -- Filters ------------------------------------------------------------------
    QGroupBox* filterGroup = new QGroupBox(tr("Alert Conditions"), this);
    filterGroup->setToolTip(tr("Alert conditions — ALL must be met to trigger (AND logic)"));
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

    // -- Interval -----------------------------------------------------------------
    QGroupBox* intervalGroup = new QGroupBox(tr("Check Interval"), this);
    QFormLayout* intervalForm = new QFormLayout(intervalGroup);
    m_intervalSpin = new QSpinBox(this);
    m_intervalSpin->setRange(30, 86400);
    m_intervalSpin->setValue(3600);
    m_intervalSpin->setSuffix(tr(" sec"));
    m_intervalSpin->setToolTip(tr("Set how often to check the price (30s – 24h)"));
    intervalForm->addRow(tr("Interval:"), m_intervalSpin);
    mainLayout->addWidget(intervalGroup);

    // -- Buttons ------------------------------------------------------------------
    QDialogButtonBox* buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    mainLayout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, this, [this]() {
        // Name validation
        QString name = m_nameEdit->text().trimmed();
        if (name.isEmpty()) {
            QMessageBox::warning(this, tr("Validation"), tr("Product name cannot be empty."));
            return;
        }
        if (name.length() > 200) {
            QMessageBox::warning(this, tr("Validation"), tr("Product name is too long (max 200 characters)."));
            return;
        }

        // URL validation
        QString url = m_urlEdit->text().trimmed();
        if (url.isEmpty()) {
            QMessageBox::warning(this, tr("Validation"), tr("URL cannot be empty."));
            return;
        }
        if (!url.startsWith("https://") && !url.startsWith("http://")) {
            QMessageBox::warning(this, tr("Validation"), tr("URL must start with http:// or https://"));
            return;
        }
        if (url.length() > 2048) {
            QMessageBox::warning(this, tr("Validation"), tr("URL is too long (max 2048 characters)."));
            return;
        }

        // Condition validation
        for (const auto& filter : m_conditions) {
            if (filter.value <= 0) {
                QMessageBox::warning(this, tr("Validation"), tr("Condition value must be greater than 0."));
                return;
            }
        }

        accept();
    });
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

void ProductDialog::populateFrom(const Product& product) {
    m_nameEdit->setText(QString::fromStdString(product.name));
    // Mark this URL as already-detected so the textChanged signal fired by
    // setText() doesn't kick off a redundant network fetch in edit mode.
    m_lastDetectedUrl = QString::fromStdString(product.url);
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
            ? tr("Price ≤ %1").arg(CurrencyUtils::formatPrice(c.value, product.currency))
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

void ProductDialog::onUrlChanged() {
    QString url = m_urlEdit->text().trimmed();
    if (url == m_lastDetectedUrl) return;       // already detected this URL
    if (!url.startsWith("http://") && !url.startsWith("https://")) {
        // Use a single space so the line keeps its reserved height; this
        // avoids the form below jumping when the user clears the URL field.
        m_detectStatus->setText(QStringLiteral(" "));
        m_detectStatus->setToolTip(QString());
        m_urlDebounce->stop();
        return;
    }
    m_urlDebounce->start();
}

void ProductDialog::selectSourceById(const QString& sourceId) {
    int idx = m_sourceIds.indexOf(sourceId);
    if (idx >= 0) m_sourceCombo->setCurrentIndex(idx);
}

void ProductDialog::runUrlDetect() {
    if (!m_pluginManager) return;
    QString url = m_urlEdit->text().trimmed();
    if (url.isEmpty()) return;
    if (url == m_lastDetectedUrl) return;

    IPriceHandler* h = m_pluginManager->findHandlerForUrl(url.toStdString());
    if (!h) {
        m_lastDetectedUrl = url;
        emit detectFinished(QString(), QString(), QString());
        return;
    }

    QString sourceId = QString::fromStdString(h->handlerId());
    // Show detection status immediately while name fetch runs in background.
    selectSourceById(sourceId);
    m_detectStatus->setText(tr("Detected: %1 — fetching name…").arg(sourceId));
    m_lastDetectedUrl = url;

    // Clone the handler so the worker thread owns an isolated instance with
    // its own HttpClient — no shared mutation with the polling thread.
    std::shared_ptr<IPriceHandler> worker(h->clone().release());
    if (!worker) {
        // Handler doesn't support cloning (e.g. JSON plugin) — keep source
        // detection but skip the name fetch.
        emit detectFinished(sourceId, QString(), QString());
        return;
    }

    std::string urlStd = url.toStdString();
    QPointer<ProductDialog> self(this);
    QtConcurrent::run([self, urlStd, sourceId, worker]() {
        HttpClient localClient;
        worker->setHttpClient(&localClient);
        FetchResult res = worker->fetchProduct(urlStd);
        if (!self) return;   // dialog closed before fetch finished
        QString name     = res.success ? QString::fromStdString(res.name) : QString();
        QString currency = QString::fromStdString(res.currency);
        QMetaObject::invokeMethod(self.data(), "detectFinished",
                                  Qt::QueuedConnection,
                                  Q_ARG(QString, sourceId),
                                  Q_ARG(QString, name),
                                  Q_ARG(QString, currency));
    });
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
