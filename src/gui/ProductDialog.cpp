#include "gui/ProductDialog.hpp"
#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QListWidget>
#include <QComboBox>
#include <QLineEdit>
#include <QSpinBox>

ProductDialog::ProductDialog(QWidget* parent)
    : QDialog(parent)
{
    QVBoxLayout* layout = new QVBoxLayout(this);

    nameEdit = new QLineEdit(this);
    urlEdit = new QLineEdit(this);
    sourceCombo = new QComboBox(this);
    filtersList = new QListWidget(this);
    filterTypeCombo = new QComboBox(this);
    filterValueSpin = new QSpinBox(this);
    intervalSpin = new QSpinBox(this);

    sourceCombo->addItems({"Steam", "Udemy"});
    filterTypeCombo->addItems({"Price >= ", "Discount >= "});
    intervalSpin->setSuffix(" sec");
    intervalSpin->setRange(30, 86400); // from 30 sec to 24 hrs

    layout->addWidget(new QLabel("Product Name:"));
    layout->addWidget(nameEdit);
    layout->addWidget(new QLabel("URL:"));
    layout->addWidget(urlEdit);
    layout->addWidget(new QLabel("Source:"));
    layout->addWidget(sourceCombo);

    layout->addWidget(new QLabel("Filters:"));
    layout->addWidget(filtersList);
    layout->addWidget(filterTypeCombo);
    layout->addWidget(filterValueSpin);

    QPushButton* addFilterBtn = new QPushButton("Add Filter");
    QPushButton* removeFilterBtn = new QPushButton("Remove Selected");
    layout->addWidget(addFilterBtn);
    layout->addWidget(removeFilterBtn);

    layout->addWidget(new QLabel("Check Interval:"));
    layout->addWidget(intervalSpin);

    QPushButton* okBtn = new QPushButton("OK");
    QPushButton* cancelBtn = new QPushButton("Cancel");
    layout->addWidget(okBtn);
    layout->addWidget(cancelBtn);

    connect(addFilterBtn, &QPushButton::clicked, this, &ProductDialog::addFilter);
    connect(removeFilterBtn, &QPushButton::clicked, this, &ProductDialog::removeFilter);
    connect(okBtn, &QPushButton::clicked, this, &QDialog::accept);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
}

void ProductDialog::addFilter() {
    auto type = filterTypeCombo->currentIndex() == 0
                ? PriceCondition::Type::PRICE_GREATER_EQUAL
                : PriceCondition::Type::DISCOUNT_GREATER_EQUAL;

    float value = filterValueSpin->value();
    conditions.push_back({type, value});

    QString text = (type == PriceCondition::Type::PRICE_GREATER_EQUAL)
                   ? QString("Price >= %1").arg(value)
                   : QString("Discount >= %1").arg(value);
    filtersList->addItem(text);
}

void ProductDialog::removeFilter() {
    int row = filtersList->currentRow();
    if (row >= 0) {
        delete filtersList->takeItem(row);
        conditions.erase(conditions.begin() + row);
    }
}

Product ProductDialog::getProduct() const {
    Product p;
    p.name = nameEdit->text().toStdString();
    p.source = sourceCombo->currentText().toLower().toStdString();
    p.filters = conditions;
    p.checkInterval = std::chrono::seconds(intervalSpin->value());
    p.lastChecked = std::chrono::system_clock::now();
    return p;
}
