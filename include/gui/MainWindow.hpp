#pragma once

#include "core/DataStructs.hpp"

#include <QMainWindow>
#include <QPointer>
#include <QTableWidget>
#include <QJsonArray>
#include <vector>

class AppController;
class TrayIcon;
class UpdateChecker;
class UpdateDialog;
class ProductCardView;
class ProductDetailPane;
class AnnouncementCenter;
class QStackedWidget;
class QSplitter;
class QPushButton;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(AppController* controller, QWidget* parent = nullptr);
    ~MainWindow() override;

protected:
    void closeEvent(QCloseEvent* event) override;
    void changeEvent(QEvent* event) override;

public slots:
    void onAlertTriggered(AlertEvent event, bool firstThisSession);
    void onProductPriceChanged(int productId, float newPrice, float newDiscount);
    // Restore the window from the tray and bring it to the foreground.
    // Also reused by SingleInstanceGuard when a second instance pings.
    void showFromTray();

private slots:
    void addProduct();
    void editProduct();
    void editProductById(int productId);
    void removeProduct();
    void showAlertHistory();
    void showSettings();
    void refreshTable();
    void refreshAll();
    void setLayoutCards();
    void setLayoutTable();
    void restartApp();
    void checkNow();
    void checkNowFor(int productId);
    void onCheckNowFinished(int productId, bool success, float newPrice, float newDiscount);
    void checkForUpdates();
    void onUpdateAvailable(const QString& version,
                           const QString& url,
                           const QString& body,
                           const QJsonArray& assets);
    void onNoUpdateAvailable();
    void onUpdateCheckFailed(const QString& errorMsg);

private:
    void setupUi();
    void setupMenu();
    void setupTray();
    void setupUpdateChecker();
    void setupAnnouncements();
    void loadProducts();
    void applyDarkTheme();
    void onProductSelectionChanged();
    void showCardContextMenu(int productId, const QPoint& globalPos);
    const Product* findProduct(int productId) const;
    QList<int> currentSelectedIds() const;

    AppController*   m_controller;
    QStackedWidget*  m_viewStack = nullptr;
    QSplitter*       m_tableSplitter = nullptr;
    QTableWidget*    m_table;
    ProductCardView* m_cardView = nullptr;
    ProductDetailPane* m_detailPane = nullptr;
    // Segmented "Cards | Table" toggle (shadcn-style ToggleGroup).
    QPushButton*     m_layoutCardsBtn = nullptr;
    QPushButton*     m_layoutTableBtn = nullptr;
    AnnouncementCenter* m_announcements = nullptr;
    TrayIcon*        m_trayIcon;
    UpdateChecker*   m_updateChecker;
    QPointer<UpdateDialog> m_updateDialog;
    bool             m_manualUpdateCheck = false;
    std::vector<Product> m_products;
};
