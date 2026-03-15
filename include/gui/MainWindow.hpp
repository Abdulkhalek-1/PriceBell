#pragma once

#include "core/DataStructs.hpp"
#include "core/PricePoller.hpp"
#include "core/AlertManager.hpp"
#include "core/PluginManager.hpp"

#include <QMainWindow>
#include <QTableWidget>
#include <QThread>
#include <vector>

class TrayIcon;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

protected:
    // Intercept close to minimize to tray instead of quitting.
    void closeEvent(QCloseEvent* event) override;

public slots:
    void onAlertTriggered(AlertEvent event);
    void onProductPriceChanged(int productId, float newPrice, float newDiscount);

private slots:
    void addProduct();
    void editProduct();
    void removeProduct();
    void showAlertHistory();
    void showSettings();
    void refreshTable();

private:
    void setupUi();
    void setupMenu();
    void setupTray();
    void setupPoller();
    void loadProducts();
    void applyDarkTheme();

    QTableWidget*   m_table;
    TrayIcon*       m_trayIcon;
    PluginManager*  m_pluginManager;
    AlertManager*   m_alertManager;
    PricePoller*    m_poller;
    QThread*        m_pollerThread;
    std::vector<Product> m_products;
};
