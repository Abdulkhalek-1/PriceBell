#include "gui/MainWindow.hpp"
#include "gui/ProductDialog.hpp"
#include "gui/AlertHistoryDialog.hpp"
#include "gui/SettingsDialog.hpp"
#include "gui/TrayIcon.hpp"
#include "core/AppController.hpp"
#include "core/PluginManager.hpp"
#include "utils/Logger.hpp"
#include "utils/UpdateChecker.hpp"
#include "utils/CurrencyUtils.hpp"
#include "utils/SettingsProvider.hpp"

#include <QApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QToolBar>
#include <QMenuBar>
#include <QPushButton>
#include <QAction>
#include <QMessageBox>
#include <QHeaderView>
#include <QTableWidgetItem>
#include <QCloseEvent>
#include <QFile>
#include <QSettings>
#include <QDesktopServices>
#include <QUrl>
#include <QSet>
#include <chrono>

MainWindow::MainWindow(AppController* controller, QWidget* parent)
    : QMainWindow(parent)
    , m_controller(controller)
{
    setWindowTitle(tr("PriceBell"));
    setMinimumSize(800, 500);

    applyDarkTheme();
    setupUi();
    setupMenu();
    setupTray();
    loadProducts();
    setupUpdateChecker();

    // Connect controller signals
    connect(m_controller, &AppController::priceChanged,
            this, &MainWindow::onProductPriceChanged);
    connect(m_controller, &AppController::checkNowFinished,
            this, &MainWindow::onCheckNowFinished);
    connect(m_controller, &AppController::alertTriggered,
            this, &MainWindow::onAlertTriggered);
}

MainWindow::~MainWindow() {
}

void MainWindow::applyDarkTheme() {
    QFile qss(":/assets/themes/dark.qss");
    if (qss.open(QFile::ReadOnly)) {
        qApp->setStyleSheet(QString::fromUtf8(qss.readAll()));
        qss.close();
    } else {
        Logger::error("Failed to load dark.qss from resources — UI will use default style.");
    }
}

void MainWindow::setupUi() {
    QWidget* central = new QWidget(this);
    setCentralWidget(central);
    QVBoxLayout* layout = new QVBoxLayout(central);
    layout->setContentsMargins(12, 8, 12, 8);
    layout->setSpacing(8);

    // -- Toolbar ---------------------------------------------------------------
    QHBoxLayout* toolbar = new QHBoxLayout();
    QPushButton* addBtn      = new QPushButton(tr("+ Add Product"),   this);
    QPushButton* editBtn     = new QPushButton(tr("✎ Edit"),           this);
    QPushButton* removeBtn   = new QPushButton(tr("✕ Remove"),         this);
    QPushButton* checkNowBtn = new QPushButton(tr("🔄 Check Now"),     this);
    QPushButton* alertsBtn   = new QPushButton(tr("🔔 Alert History"), this);
    QPushButton* settingsBtn = new QPushButton(tr("⚙ Settings"),       this);

    // Tooltips for buttons
    addBtn->setToolTip(tr("Add a new product to track"));
    editBtn->setToolTip(tr("Edit selected product"));
    removeBtn->setToolTip(tr("Remove selected product"));
    checkNowBtn->setToolTip(tr("Check price now"));
    alertsBtn->setToolTip(tr("View alert history"));
    settingsBtn->setToolTip(tr("Open settings"));

    toolbar->addWidget(addBtn);
    toolbar->addWidget(editBtn);
    toolbar->addWidget(removeBtn);
    toolbar->addWidget(checkNowBtn);
    toolbar->addStretch();
    toolbar->addWidget(alertsBtn);
    toolbar->addWidget(settingsBtn);
    layout->addLayout(toolbar);

    connect(addBtn,      &QPushButton::clicked, this, &MainWindow::addProduct);
    connect(editBtn,     &QPushButton::clicked, this, &MainWindow::editProduct);
    connect(removeBtn,   &QPushButton::clicked, this, &MainWindow::removeProduct);
    connect(checkNowBtn, &QPushButton::clicked, this, &MainWindow::checkNow);
    connect(alertsBtn,   &QPushButton::clicked, this, &MainWindow::showAlertHistory);
    connect(settingsBtn, &QPushButton::clicked, this, &MainWindow::showSettings);

    // -- Product table ---------------------------------------------------------
    m_table = new QTableWidget(this);
    m_table->setColumnCount(8);
    m_table->setHorizontalHeaderLabels({
        tr("Name"), tr("Source"), tr("Current Price"), tr("Discount %"),
        tr("Status"), tr("Last Checked"), tr("Conditions"), tr("Interval (s)")
    });
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_table->setAlternatingRowColors(true);
    layout->addWidget(m_table);

    // Tooltips for table headers
    m_table->horizontalHeaderItem(0)->setToolTip(tr("Product name"));
    m_table->horizontalHeaderItem(1)->setToolTip(tr("Price source"));
    m_table->horizontalHeaderItem(2)->setToolTip(tr("Current tracked price"));
    m_table->horizontalHeaderItem(3)->setToolTip(tr("Current discount percentage"));
    m_table->horizontalHeaderItem(4)->setToolTip(tr("Monitoring status"));
    m_table->horizontalHeaderItem(5)->setToolTip(tr("Last time price was checked"));
    m_table->horizontalHeaderItem(6)->setToolTip(tr("Alert conditions for this product"));
    m_table->horizontalHeaderItem(7)->setToolTip(tr("How often price is checked (seconds)"));
}

void MainWindow::setupMenu() {
    QMenuBar* mb = menuBar();

    QMenu* fileMenu = mb->addMenu(tr("File"));
    fileMenu->addAction(tr("Add Product"),    this, &MainWindow::addProduct);
    fileMenu->addAction(tr("Edit Product"),   this, &MainWindow::editProduct);
    fileMenu->addAction(tr("Remove Product"), this, &MainWindow::removeProduct);
    fileMenu->addSeparator();
    fileMenu->addAction(tr("Restart"), this, &MainWindow::restartApp);
    fileMenu->addAction(tr("Quit"), qApp, &QApplication::quit);

    QMenu* viewMenu = mb->addMenu(tr("View"));
    viewMenu->addAction(tr("Alert History"), this, &MainWindow::showAlertHistory);

    QMenu* toolsMenu = mb->addMenu(tr("Tools"));
    toolsMenu->addAction(tr("Check Now"), this, &MainWindow::checkNow);
    toolsMenu->addAction(tr("Settings"), this, &MainWindow::showSettings);

    QMenu* helpMenu = mb->addMenu(tr("Help"));
    helpMenu->addAction(tr("Check for Updates"), this, &MainWindow::checkForUpdates);
    helpMenu->addSeparator();
    helpMenu->addAction(tr("About"), this, [this]() {
        QMessageBox::about(this, tr("About PriceBell"),
            tr("PriceBell %1\nA price tracking application.").arg(APP_VERSION));
    });
}

void MainWindow::setupTray() {
    m_trayIcon = new TrayIcon(this, this);
    connect(m_trayIcon, &TrayIcon::showWindowRequested, this, &QWidget::show);
    connect(m_trayIcon, &TrayIcon::quitRequested, qApp, &QApplication::quit);
    m_trayIcon->show();
}

void MainWindow::setupUpdateChecker() {
    m_updateChecker = new UpdateChecker(this);

    connect(m_updateChecker, &UpdateChecker::updateAvailable,
            this, &MainWindow::onUpdateAvailable);
    connect(m_updateChecker, &UpdateChecker::noUpdateAvailable,
            this, &MainWindow::onNoUpdateAvailable);
    connect(m_updateChecker, &UpdateChecker::checkFailed,
            this, &MainWindow::onUpdateCheckFailed);

    QSettings s("PriceBell", "PriceBell");
    if (s.value("updates/check_on_startup", true).toBool()) {
        m_updateChecker->checkForUpdates();
    }
}

void MainWindow::loadProducts() {
    m_products = m_controller->products();
    refreshTable();
}

void MainWindow::refreshTable() {
    m_table->setRowCount(0);
    for (const auto& p : m_products) {
        int row = m_table->rowCount();
        m_table->insertRow(row);

        auto cell = [](const QString& text) {
            auto* item = new QTableWidgetItem(text);
            item->setTextAlignment(Qt::AlignCenter);
            return item;
        };

        QString source;
        switch (p.source) {
            case SourceType::STEAM:  source = tr("Steam");  break;
            case SourceType::UDEMY:  source = tr("Udemy");  break;
            case SourceType::AMAZON: source = tr("Amazon"); break;
            case SourceType::PLUGIN: source = QString::fromStdString(p.sourcePluginId); break;
            default: source = tr("Generic"); break;
        }

        auto time_t = std::chrono::system_clock::to_time_t(p.lastChecked);
        QString lastChecked = time_t == 0 ? tr("Never")
            : QDateTime::fromSecsSinceEpoch(static_cast<qint64>(time_t)).toString("HH:mm:ss");

        // Build conditions string
        QStringList condStrings;
        for (const auto& f : p.filters) {
            if (f.type == ConditionType::PRICE_LESS_EQUAL)
                condStrings << tr("Price <= %1").arg(CurrencyUtils::formatPrice(f.value, p.currency));
            else
                condStrings << tr("Discount >= %1%").arg(static_cast<int>(f.value));
        }
        QString condText = condStrings.join(", ");

        m_table->setItem(row, 0, cell(QString::fromStdString(p.name)));
        m_table->setItem(row, 1, cell(source));
        m_table->setItem(row, 2, cell(CurrencyUtils::formatPrice(p.currentPrice, p.currency)));
        m_table->setItem(row, 3, cell(QString("%1%").arg(static_cast<int>(p.discount))));
        m_table->setItem(row, 4, cell(p.isActive ? tr("Watching") : tr("Paused")));
        m_table->setItem(row, 5, cell(lastChecked));
        m_table->setItem(row, 6, cell(condText));
        m_table->setItem(row, 7, cell(QString::number(p.checkInterval.count())));

        // Set full conditions text as tooltip if truncated
        if (!condText.isEmpty()) {
            m_table->item(row, 6)->setToolTip(condText);
        }

        // Store product id for row operations
        m_table->item(row, 0)->setData(Qt::UserRole, p.id);
    }
}

void MainWindow::addProduct() {
    ProductDialog dlg(m_controller->pluginManager(), this);
    if (dlg.exec() != QDialog::Accepted) return;

    Product product = dlg.getProduct();
    if (!m_controller->addProduct(product)) {
        QMessageBox::critical(this, tr("Error"), tr("Failed to save product to database."));
        return;
    }
    m_products.push_back(product);
    refreshTable();
}

void MainWindow::editProduct() {
    int row = m_table->currentRow();
    if (row < 0) {
        QMessageBox::information(this, tr("No Selection"), tr("Please select a product to edit."));
        return;
    }
    int productId = m_table->item(row, 0)->data(Qt::UserRole).toInt();
    auto it = std::find_if(m_products.begin(), m_products.end(),
                           [productId](const Product& p){ return p.id == productId; });
    if (it == m_products.end()) return;

    ProductDialog dlg(m_controller->pluginManager(), *it, this);
    if (dlg.exec() != QDialog::Accepted) return;

    Product updated = dlg.getProduct();
    updated.id = productId;
    if (!m_controller->editProduct(updated)) {
        QMessageBox::critical(this, tr("Error"), tr("Failed to update product."));
        return;
    }
    *it = updated;
    refreshTable();
}

void MainWindow::removeProduct() {
    int row = m_table->currentRow();
    if (row < 0) {
        QMessageBox::information(this, tr("No Selection"), tr("Please select a product to remove."));
        return;
    }
    int productId = m_table->item(row, 0)->data(Qt::UserRole).toInt();
    QString name  = m_table->item(row, 0)->text();

    auto reply = QMessageBox::question(this, tr("Confirm"),
        tr("Remove \"%1\"? This will also delete its alert history.").arg(name),
        QMessageBox::Yes | QMessageBox::No);
    if (reply != QMessageBox::Yes) return;

    m_controller->removeProduct(productId);
    m_products.erase(std::remove_if(m_products.begin(), m_products.end(),
                     [productId](const Product& p){ return p.id == productId; }),
                     m_products.end());
    refreshTable();
}

void MainWindow::showAlertHistory() {
    AlertHistoryDialog dlg(this);
    dlg.exec();
}

void MainWindow::showSettings() {
    SettingsDialog dlg(this, m_controller->pluginManager());
    if (dlg.exec() == QDialog::Accepted) {
        // Reload plugins in case directory changed
        QString dir = SettingsProvider::instance().pluginDirectory();
        m_controller->pluginManager()->loadPlugins(dir);
        m_controller->pluginManager()->loadJsonSources();

        if (dlg.isRestartNeeded()) {
            qApp->exit(RESTART_EXIT_CODE);
        }
    }
}

void MainWindow::restartApp() {
    qApp->exit(RESTART_EXIT_CODE);
}

// -- Check Now -----------------------------------------------------------------

void MainWindow::checkNow() {
    QSet<int> selectedRows;
    for (auto* item : m_table->selectedItems()) {
        selectedRows.insert(item->row());
    }

    // If nothing selected, check all products
    if (selectedRows.isEmpty()) {
        for (int row = 0; row < m_table->rowCount(); ++row) {
            selectedRows.insert(row);
        }
    }

    for (int row : selectedRows) {
        int productId = m_table->item(row, 0)->data(Qt::UserRole).toInt();

        // Set loading state
        m_table->item(row, 4)->setText(tr("Checking..."));
        m_table->item(row, 4)->setForeground(QColor("#f9e2af"));

        m_controller->checkNow(productId);
    }
}

void MainWindow::onCheckNowFinished(int productId, bool success, float, float) {
    for (int row = 0; row < m_table->rowCount(); ++row) {
        if (m_table->item(row, 0)->data(Qt::UserRole).toInt() == productId) {
            if (!success) {
                m_table->item(row, 4)->setText(tr("Error"));
                m_table->item(row, 4)->setForeground(QColor("#f38ba8"));
            } else {
                auto it = std::find_if(m_products.begin(), m_products.end(),
                    [productId](const Product& p) { return p.id == productId; });
                if (it != m_products.end()) {
                    m_table->item(row, 4)->setText(it->isActive ? tr("Watching") : tr("Paused"));
                    m_table->item(row, 4)->setForeground(QColor("#cdd6f4"));
                }
            }
            break;
        }
    }
}

// -- Update checker ------------------------------------------------------------

void MainWindow::checkForUpdates() {
    m_manualUpdateCheck = true;
    m_updateChecker->checkForUpdates();
}

void MainWindow::onUpdateAvailable(const QString& version, const QString& url) {
    if (m_manualUpdateCheck) {
        m_manualUpdateCheck = false;
        auto reply = QMessageBox::information(this, tr("Update Available"),
            tr("A new version of PriceBell (%1) is available.\n\n"
               "Would you like to open the release page?").arg(version),
            QMessageBox::Yes | QMessageBox::No);
        if (reply == QMessageBox::Yes) {
            QDesktopServices::openUrl(QUrl(url));
        }
    } else {
        // Silent auto-check: only tray notification
        m_trayIcon->showMessage(
            tr("Update Available"),
            tr("PriceBell %1 is available.").arg(version),
            QSystemTrayIcon::Information, 8000);
    }
}

void MainWindow::onNoUpdateAvailable() {
    if (m_manualUpdateCheck) {
        m_manualUpdateCheck = false;
        QMessageBox::information(this, tr("Updates"),
            tr("You are running the latest version of PriceBell."));
    }
}

void MainWindow::onUpdateCheckFailed(const QString& errorMsg) {
    if (m_manualUpdateCheck) {
        m_manualUpdateCheck = false;
        QMessageBox::warning(this, tr("Update Check Failed"),
            tr("Could not check for updates: %1").arg(errorMsg));
    }
}

// -- Events --------------------------------------------------------------------

void MainWindow::onAlertTriggered(AlertEvent event) {
    m_trayIcon->showAlert(event);

    // Highlight the matching row in the table
    for (int row = 0; row < m_table->rowCount(); ++row) {
        if (m_table->item(row, 0)->data(Qt::UserRole).toInt() == event.productId) {
            m_table->item(row, 4)->setText(tr("🔔 Alert!"));
            m_table->item(row, 4)->setForeground(QColor("#f38ba8")); // red
            break;
        }
    }
}

void MainWindow::onProductPriceChanged(int productId, float newPrice, float newDiscount) {
    for (int row = 0; row < m_table->rowCount(); ++row) {
        if (m_table->item(row, 0)->data(Qt::UserRole).toInt() == productId) {
            // Find the product to get its currency
            std::string currency = "USD";
            for (auto& p : m_products) {
                if (p.id == productId) {
                    p.currentPrice = newPrice;
                    p.discount     = newDiscount;
                    currency = p.currency;
                    break;
                }
            }

            m_table->item(row, 2)->setText(CurrencyUtils::formatPrice(newPrice, currency));
            m_table->item(row, 3)->setText(QString("%1%").arg(static_cast<int>(newDiscount)));
            m_table->item(row, 5)->setText(
                QDateTime::currentDateTime().toString("HH:mm:ss"));
            break;
        }
    }
}

void MainWindow::closeEvent(QCloseEvent* event) {
    // Minimize to tray instead of quitting
    hide();
    m_trayIcon->showMessage(tr("PriceBell"),
        tr("PriceBell is still running in the background."),
        QSystemTrayIcon::Information, 3000);
    event->ignore();
}
