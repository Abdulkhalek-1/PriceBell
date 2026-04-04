#pragma once

#include "core/DataStructs.hpp"

#include <QMainWindow>
#include <QTableWidget>
#include <QJsonArray>
#include <vector>

class AppController;
class TrayIcon;
class UpdateChecker;
class UpdateDialog;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(AppController* controller, QWidget* parent = nullptr);
    ~MainWindow() override;

protected:
    void closeEvent(QCloseEvent* event) override;
    void changeEvent(QEvent* event) override;

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
    void restartApp();
    void checkNow();
    void onCheckNowFinished(int productId, bool success, float newPrice, float newDiscount);
    void checkForUpdates();
    void onUpdateAvailable(const QString& version,
                           const QString& url,
                           const QString& body,
                           const QJsonArray& assets);
    void onNoUpdateAvailable();
    void onUpdateCheckFailed(const QString& errorMsg);
    void showFromTray();

private:
    void setupUi();
    void setupMenu();
    void setupTray();
    void setupUpdateChecker();
    void loadProducts();
    void applyDarkTheme();

    AppController*   m_controller;
    QTableWidget*    m_table;
    TrayIcon*        m_trayIcon;
    UpdateChecker*   m_updateChecker;
    bool             m_manualUpdateCheck = false;
    std::vector<Product> m_products;
};
