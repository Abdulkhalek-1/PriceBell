#pragma once
#include <QDialog>
#include "core/DataStructs.hpp"

class QLineEdit;
class QComboBox;
class QSpinBox;
class QListWidget;

class ProductDialog : public QDialog {
    Q_OBJECT

public:
    ProductDialog(QWidget* parent = nullptr);
    Product getProduct() const;

private slots:
    void addFilter();
    void removeFilter();

private:
    QLineEdit* nameEdit;
    QLineEdit* urlEdit;
    QComboBox* sourceCombo;
    QListWidget* filtersList;
    QComboBox* filterTypeCombo;
    QSpinBox* filterValueSpin;
    QSpinBox* intervalSpin;
    std::vector<PriceCondition> conditions;
};
