#pragma once

#include "core/DataStructs.hpp"
#include "core/PluginManager.hpp"

#include <QDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QListWidget>
#include <vector>

class ProductDialog : public QDialog {
    Q_OBJECT
public:
    // For adding a new product.
    explicit ProductDialog(PluginManager* pluginManager, QWidget* parent = nullptr);

    // For editing an existing product (pre-fills fields).
    ProductDialog(PluginManager* pluginManager, const Product& existing, QWidget* parent = nullptr);

    // Returns the configured Product on accept.
    Product getProduct() const;

private slots:
    void addFilter();
    void removeFilter();

private:
    void setupUi(PluginManager* pluginManager);
    void populateFrom(const Product& product);

    QLineEdit*       m_nameEdit;
    QLineEdit*       m_urlEdit;
    QComboBox*       m_sourceCombo;
    QListWidget*     m_filtersList;
    QComboBox*       m_filterTypeCombo;
    QDoubleSpinBox*  m_filterValueSpin;
    QSpinBox*        m_intervalSpin;

    std::vector<PriceCondition> m_conditions;
    // Maps combo index → source id string
    QStringList m_sourceIds;
};
