#pragma once

#include "core/DataStructs.hpp"
#include "core/PluginManager.hpp"

#include <QDialog>
#include <QLineEdit>
#include <QLabel>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QListWidget>
#include <QTimer>
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
    void onUrlChanged();
    void runUrlDetect();

signals:
    // Internal signal for thread-safe delivery from background fetch.
    void detectFinished(QString sourceId, QString name, QString currency);

private:
    void setupUi(PluginManager* pluginManager);
    void populateFrom(const Product& product);
    void selectSourceById(const QString& sourceId);

    PluginManager*   m_pluginManager = nullptr;
    QLineEdit*       m_nameEdit;
    QLineEdit*       m_urlEdit;
    QLabel*          m_detectStatus = nullptr;
    QComboBox*       m_sourceCombo;
    QListWidget*     m_filtersList;
    QComboBox*       m_filterTypeCombo;
    QDoubleSpinBox*  m_filterValueSpin;
    QSpinBox*        m_intervalSpin;

    QTimer*          m_urlDebounce = nullptr;
    QString          m_lastDetectedUrl;
    bool             m_userTouchedName = false;

    std::vector<PriceCondition> m_conditions;
    // Maps combo index → source id string
    QStringList m_sourceIds;
};
