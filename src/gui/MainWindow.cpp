#include "gui/MainWindow.hpp"
#include "gui/ProductDialog.hpp"
#include "gui/AlertHistoryDialog.hpp"
#include "gui/SettingsDialog.hpp"
#include "gui/TrayIcon.hpp"
#include "gui/UpdateDialog.hpp"
#include "gui/ProductCardView.hpp"
#include "gui/ProductDetailPane.hpp"
#include "gui/AnnouncementCenter.hpp"
#include "core/AppController.hpp"
#include "core/AlertManager.hpp"
#include "core/PluginManager.hpp"
#include "utils/Logger.hpp"
#include "utils/UpdateChecker.hpp"
#include "utils/CurrencyUtils.hpp"
#include "utils/SettingsProvider.hpp"

#include <QApplication>
#include <QIcon>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QToolBar>
#include <QButtonGroup>
#include <QSizePolicy>
#include <QMenuBar>
#include <QMenu>
#include <QActionGroup>
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
#include <QJsonArray>
#include <QSplitter>
#include <QStackedWidget>
#include <QTimer>
#include <chrono>

MainWindow::MainWindow(AppController* controller, QWidget* parent)
    : QMainWindow(parent)
    , m_controller(controller)
{
    setWindowTitle(tr("PriceBell"));
    setMinimumSize(900, 560);

    applyDarkTheme();
    setupUi();
    setupMenu();
    setupTray();
    loadProducts();
    setupUpdateChecker();
    setupAnnouncements();

    // Connect controller signals
    connect(m_controller, &AppController::priceChanged,
            this, &MainWindow::onProductPriceChanged);
    connect(m_controller, &AppController::checkNowFinished,
            this, &MainWindow::onCheckNowFinished);
    connect(m_controller, &AppController::alertTriggered,
            this, &MainWindow::onAlertTriggered);
    connect(m_controller, &AppController::productAdded,
            this, [this](const Product&){ refreshAll(); });
    connect(m_controller, &AppController::productUpdated,
            this, [this](const Product&){ refreshAll(); });
    connect(m_controller, &AppController::productRemoved,
            this, [this](int){ refreshAll(); });
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
    toolbar->setSpacing(6);
    QPushButton* addBtn      = new QPushButton(QIcon(":/assets/icons/add.svg"),     tr("Add Product"),   this);
    QPushButton* editBtn     = new QPushButton(QIcon(":/assets/icons/edit.svg"),    tr("Edit"),           this);
    QPushButton* removeBtn   = new QPushButton(QIcon(":/assets/icons/remove.svg"),  tr("Remove"),         this);
    QPushButton* checkNowBtn = new QPushButton(QIcon(":/assets/icons/refresh.svg"), tr("Check Now"),     this);
    QPushButton* alertsBtn   = new QPushButton(QIcon(":/assets/icons/history.svg"), tr("Alert History"), this);
    QPushButton* settingsBtn = new QPushButton(QIcon(":/assets/icons/settings.svg"), tr("Settings"),      this);

    // Tooltips for buttons (also used for icon-only mode at narrow widths)
    addBtn->setToolTip(tr("Add a new product to track"));
    editBtn->setToolTip(tr("Edit selected product"));
    removeBtn->setToolTip(tr("Remove selected product"));
    checkNowBtn->setToolTip(tr("Check price now"));
    alertsBtn->setToolTip(tr("View alert history"));
    settingsBtn->setToolTip(tr("Open settings"));

    // Constrain toolbar buttons so they don't grow to fill the row and
    // collide with the layout toggle on resize.
    for (auto* b : {addBtn, editBtn, removeBtn, checkNowBtn, alertsBtn, settingsBtn}) {
        b->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    }

    toolbar->addWidget(addBtn);
    toolbar->addWidget(editBtn);
    toolbar->addWidget(removeBtn);
    toolbar->addWidget(checkNowBtn);
    toolbar->addStretch();

    // Segmented layout toggle (shadcn-style ToggleGroup): two adjacent buttons,
    // exclusive checkable, the selected one gets the accent fill. Inline — no
    // extra click to open a menu.
    QFrame* layoutGroup = new QFrame(this);
    layoutGroup->setObjectName("layoutToggleGroup");
    QHBoxLayout* groupLayout = new QHBoxLayout(layoutGroup);
    groupLayout->setContentsMargins(2, 2, 2, 2);
    groupLayout->setSpacing(0);

    m_layoutCardsBtn = new QPushButton(tr("Cards"), layoutGroup);
    m_layoutTableBtn = new QPushButton(tr("Table"), layoutGroup);
    for (auto* b : {m_layoutCardsBtn, m_layoutTableBtn}) {
        b->setCheckable(true);
        b->setFlat(true);
        b->setCursor(Qt::PointingHandCursor);
        b->setFocusPolicy(Qt::NoFocus);
    }
    // objectName drives the QSS pill-shape rounding for first/last segments.
    m_layoutCardsBtn->setObjectName("layoutSegLeft");
    m_layoutTableBtn->setObjectName("layoutSegRight");
    m_layoutCardsBtn->setToolTip(tr("Card layout"));
    m_layoutTableBtn->setToolTip(tr("Table layout"));

    QButtonGroup* layoutBtnGroup = new QButtonGroup(this);
    layoutBtnGroup->setExclusive(true);
    layoutBtnGroup->addButton(m_layoutCardsBtn);
    layoutBtnGroup->addButton(m_layoutTableBtn);

    groupLayout->addWidget(m_layoutCardsBtn);
    groupLayout->addWidget(m_layoutTableBtn);
    toolbar->addWidget(layoutGroup);

    connect(m_layoutCardsBtn, &QPushButton::clicked, this, &MainWindow::setLayoutCards);
    connect(m_layoutTableBtn, &QPushButton::clicked, this, &MainWindow::setLayoutTable);

    toolbar->addWidget(alertsBtn);
    toolbar->addWidget(settingsBtn);
    layout->addLayout(toolbar);

    connect(addBtn,      &QPushButton::clicked, this, &MainWindow::addProduct);
    connect(editBtn,     &QPushButton::clicked, this, &MainWindow::editProduct);
    connect(removeBtn,   &QPushButton::clicked, this, &MainWindow::removeProduct);
    connect(checkNowBtn, &QPushButton::clicked, this, &MainWindow::checkNow);
    connect(alertsBtn,   &QPushButton::clicked, this, &MainWindow::showAlertHistory);
    connect(settingsBtn, &QPushButton::clicked, this, &MainWindow::showSettings);

    // -- Product table (one half of the layout stack) --------------------------
    // 5 columns — Conditions and Interval live in the detail pane on the right
    // so the table itself stays scannable.
    m_table = new QTableWidget(this);
    m_table->setColumnCount(5);
    m_table->setHorizontalHeaderLabels({
        tr("Product"), tr("Price"), tr("Discount"),
        tr("Status"), tr("Last Checked")
    });
    auto* hh = m_table->horizontalHeader();
    hh->setSectionResizeMode(0, QHeaderView::Stretch);          // Product takes leftover
    hh->setSectionResizeMode(1, QHeaderView::ResizeToContents); // Price
    hh->setSectionResizeMode(2, QHeaderView::ResizeToContents); // Discount
    hh->setSectionResizeMode(3, QHeaderView::ResizeToContents); // Status
    hh->setSectionResizeMode(4, QHeaderView::ResizeToContents); // Last Checked
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_table->setAlternatingRowColors(true);
    m_table->verticalHeader()->setVisible(false);
    m_table->verticalHeader()->setDefaultSectionSize(34);

    // Tooltips for table headers
    m_table->horizontalHeaderItem(0)->setToolTip(tr("Product name and source"));
    m_table->horizontalHeaderItem(1)->setToolTip(tr("Current tracked price"));
    m_table->horizontalHeaderItem(2)->setToolTip(tr("Current discount percentage"));
    m_table->horizontalHeaderItem(3)->setToolTip(tr("Monitoring status"));
    m_table->horizontalHeaderItem(4)->setToolTip(tr("Last time price was checked"));

    // Detail pane shared by both views, but in the layout stack we put it next
    // to the table inside a splitter so power users see everything at once.
    m_detailPane = new ProductDetailPane(m_controller, this);
    connect(m_detailPane, &ProductDetailPane::editRequested,
            this, &MainWindow::editProductById);
    connect(m_detailPane, &ProductDetailPane::checkNowRequested,
            this, &MainWindow::checkNowFor);
    connect(m_detailPane, &ProductDetailPane::pauseToggleRequested,
            this, [this](int productId) {
        for (auto& p : m_products) {
            if (p.id == productId) {
                p.isActive = !p.isActive;
                m_controller->editProduct(p);
                break;
            }
        }
    });

    m_tableSplitter = new QSplitter(Qt::Horizontal, this);
    m_tableSplitter->addWidget(m_table);
    m_tableSplitter->addWidget(m_detailPane);
    m_tableSplitter->setStretchFactor(0, 3);
    m_tableSplitter->setStretchFactor(1, 1);
    m_tableSplitter->setCollapsible(1, true);
    m_tableSplitter->setSizes({700, 300});

    // Card view: simpler — no detail pane next to it (cards are themselves rich).
    m_cardView = new ProductCardView(this);
    connect(m_cardView, &ProductCardView::productActivated,
            this, &MainWindow::editProductById);
    connect(m_cardView, &ProductCardView::productContextRequested,
            this, &MainWindow::showCardContextMenu);
    connect(m_cardView, &QListWidget::itemSelectionChanged,
            this, &MainWindow::onProductSelectionChanged);

    connect(m_table, &QTableWidget::itemSelectionChanged,
            this, &MainWindow::onProductSelectionChanged);

    m_viewStack = new QStackedWidget(this);
    m_viewStack->addWidget(m_cardView);     // index 0 = cards
    m_viewStack->addWidget(m_tableSplitter);// index 1 = table+pane
    layout->addWidget(m_viewStack);

    // Apply persisted layout preference. Settings dropdown writes here too.
    auto mode = SettingsProvider::instance().layoutMode();
    if (mode == SettingsProvider::LayoutMode::Table) {
        m_viewStack->setCurrentIndex(1);
        m_layoutTableBtn->setChecked(true);
    } else {
        m_viewStack->setCurrentIndex(0);
        m_layoutCardsBtn->setChecked(true);
    }
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
    connect(m_trayIcon, &TrayIcon::showWindowRequested, this, &MainWindow::showFromTray);
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
    refreshAll();
}

void MainWindow::refreshTable() {
    m_table->setRowCount(0);
    for (const auto& p : m_products) {
        int row = m_table->rowCount();
        m_table->insertRow(row);

        auto cell = [](const QString& text, Qt::Alignment align = Qt::AlignVCenter | Qt::AlignLeft) {
            auto* item = new QTableWidgetItem(text);
            item->setTextAlignment(align);
            return item;
        };

        QIcon sourceIcon;
        QString sourceTip;
        switch (p.source) {
            case SourceType::STEAM:
                sourceIcon = QIcon(":/assets/icons/source_steam.svg"); sourceTip = tr("Steam"); break;
            case SourceType::UDEMY:
                sourceIcon = QIcon(":/assets/icons/source_udemy.svg"); sourceTip = tr("Udemy"); break;
            case SourceType::AMAZON:
                sourceIcon = QIcon(":/assets/icons/source_amazon.svg"); sourceTip = tr("Amazon"); break;
            case SourceType::PLUGIN:
                sourceIcon = QIcon(":/assets/icons/source_plugin.svg");
                sourceTip = QString::fromStdString(p.sourcePluginId); break;
            default:
                sourceIcon = QIcon(":/assets/icons/source_generic.svg"); sourceTip = tr("Generic"); break;
        }

        // Col 0 — Product (source icon + name; full URL on hover)
        auto* nameItem = cell(QString::fromStdString(p.name));
        nameItem->setIcon(sourceIcon);
        nameItem->setData(Qt::UserRole, p.id);
        nameItem->setToolTip(QString("%1\n%2")
                             .arg(QString::fromStdString(p.name),
                                  QString::fromStdString(p.url)));
        m_table->setItem(row, 0, nameItem);

        // Col 1 — Price (right-aligned so prices line up by digit)
        m_table->setItem(row, 1, cell(
            p.currentPrice > 0
                ? CurrencyUtils::formatPrice(p.currentPrice, p.currency)
                : QStringLiteral("—"),
            Qt::AlignVCenter | Qt::AlignRight));

        // Col 2 — Discount (em-dash when no discount, less visual noise)
        auto* discItem = cell(
            p.discount > 0 ? QString("-%1%").arg(static_cast<int>(p.discount))
                           : QStringLiteral("—"),
            Qt::AlignCenter);
        if (p.discount > 0) discItem->setForeground(QColor("#a6e3a1"));
        else                discItem->setForeground(QColor("#6c7086"));
        m_table->setItem(row, 2, discItem);

        // Col 3 — Status (icon + label, color set later by signal handlers)
        auto* statusItem = cell(p.isActive ? tr("Watching") : tr("Paused"),
                                Qt::AlignVCenter | Qt::AlignLeft);
        statusItem->setIcon(p.isActive
            ? QIcon(":/assets/icons/status_watching.svg")
            : QIcon(":/assets/icons/status_paused.svg"));
        statusItem->setForeground(p.isActive ? QColor("#a6e3a1") : QColor("#6c7086"));
        m_table->setItem(row, 3, statusItem);

        // Col 4 — Last Checked (relative is too jittery, keep absolute time)
        auto time_t = std::chrono::system_clock::to_time_t(p.lastChecked);
        QString lastChecked = time_t == 0
            ? tr("Never")
            : QDateTime::fromSecsSinceEpoch(static_cast<qint64>(time_t)).toString("HH:mm:ss");
        auto* lastItem = cell(lastChecked, Qt::AlignVCenter | Qt::AlignRight);
        lastItem->setForeground(QColor("#6c7086"));
        m_table->setItem(row, 4, lastItem);
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
    refreshAll();
}

void MainWindow::editProduct() {
    auto ids = currentSelectedIds();
    if (ids.isEmpty()) {
        QMessageBox::information(this, tr("No Selection"), tr("Please select a product to edit."));
        return;
    }
    editProductById(ids.first());
}

void MainWindow::removeProduct() {
    auto ids = currentSelectedIds();
    if (ids.isEmpty()) {
        QMessageBox::information(this, tr("No Selection"),
            tr("Please select one or more products to remove."));
        return;
    }

    // Resolve names up-front for the confirmation dialog.
    QStringList names;
    for (int id : ids) {
        if (const Product* p = findProduct(id)) names << QString::fromStdString(p->name);
    }

    QString prompt;
    if (ids.size() == 1) {
        prompt = tr("Remove \"%1\"? This will also delete its alert history.")
                    .arg(names.value(0));
    } else {
        QStringList bullets;
        for (const auto& n : names) bullets << QString("• %1").arg(n);
        prompt = tr("Remove %1 products?\n\n%2\n\nThis will also delete their alert history.")
                    .arg(ids.size())
                    .arg(bullets.join('\n'));
    }

    auto reply = QMessageBox::question(this, tr("Confirm"),
        prompt, QMessageBox::Yes | QMessageBox::No);
    if (reply != QMessageBox::Yes) return;

    for (int id : ids) {
        m_controller->removeProduct(id);
        m_products.erase(std::remove_if(m_products.begin(), m_products.end(),
                         [id](const Product& p){ return p.id == id; }),
                         m_products.end());
    }
    refreshAll();
}

void MainWindow::showAlertHistory() {
    AlertHistoryDialog dlg(this);
    if (auto* mgr = m_controller->alertManager()) {
        connect(&dlg, &AlertHistoryDialog::alertDismissed,
                mgr, &AlertManager::resetNotificationFor);
    }
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
    auto ids = currentSelectedIds();
    // Empty selection → check all (matches the v1.3 behaviour)
    if (ids.isEmpty()) {
        for (const auto& p : m_products) ids.append(p.id);
    }

    for (int productId : ids) {
        // Reflect "checking" state in whichever view is active.
        for (int row = 0; row < m_table->rowCount(); ++row) {
            if (m_table->item(row, 0)->data(Qt::UserRole).toInt() == productId) {
                m_table->item(row, 3)->setText(tr("Checking..."));
                m_table->item(row, 3)->setForeground(QColor("#f9e2af"));
                break;
            }
        }
        if (m_cardView) {
            m_cardView->setStatusFor(productId, tr("Checking…"), QColor("#f9e2af"));
        }
        m_controller->checkNow(productId);
    }
}

void MainWindow::onCheckNowFinished(int productId, bool success, float, float) {
    for (int row = 0; row < m_table->rowCount(); ++row) {
        if (m_table->item(row, 0)->data(Qt::UserRole).toInt() == productId) {
            if (!success) {
                m_table->item(row, 3)->setText(tr("Error"));
                m_table->item(row, 3)->setIcon(QIcon(":/assets/icons/status_error.svg"));
                m_table->item(row, 3)->setForeground(QColor("#f38ba8"));
            } else {
                auto it = std::find_if(m_products.begin(), m_products.end(),
                    [productId](const Product& p) { return p.id == productId; });
                if (it != m_products.end()) {
                    m_table->item(row, 3)->setText(it->isActive ? tr("Watching") : tr("Paused"));
                    m_table->item(row, 3)->setIcon(it->isActive
                        ? QIcon(":/assets/icons/status_watching.svg")
                        : QIcon(":/assets/icons/status_paused.svg"));
                    m_table->item(row, 3)->setForeground(
                        it->isActive ? QColor("#a6e3a1") : QColor("#6c7086"));
                }
            }
            break;
        }
    }
    if (m_cardView) {
        const Product* p = findProduct(productId);
        if (!success) {
            m_cardView->setStatusFor(productId, tr("Error"), QColor("#f38ba8"));
        } else if (p) {
            m_cardView->setStatusFor(productId,
                p->isActive ? tr("Watching") : tr("Paused"),
                QColor(p->isActive ? "#a6e3a1" : "#6c7086"));
        }
    }
}

// -- Update checker ------------------------------------------------------------

void MainWindow::checkForUpdates() {
    m_manualUpdateCheck = true;
    m_updateChecker->checkForUpdates();
}

void MainWindow::onUpdateAvailable(const QString& version,
                                   const QString& url,
                                   const QString& body,
                                   const QJsonArray& assets) {
    Q_UNUSED(body)

    // Don't show if user has skipped this version
    QSettings s("PriceBell", "PriceBell");
    if (s.value("updates/skippedVersion").toString() == version) {
        m_manualUpdateCheck = false;
        return;
    }

    m_manualUpdateCheck = false;
    if (m_updateDialog) {
        m_updateDialog->raise();
        m_updateDialog->activateWindow();
        return;
    }
    m_updateDialog = new UpdateDialog(APP_VERSION, version, url, assets, this);
    m_updateDialog->setAttribute(Qt::WA_DeleteOnClose);
    m_updateDialog->show();
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

void MainWindow::onAlertTriggered(AlertEvent event, bool firstThisSession) {
    // Tray notifications dedupe to once per product per app session — see
    // AlertManager. The in-app status indicator always updates so the user
    // can see the row is in alert state.
    if (firstThisSession) {
        m_trayIcon->showAlert(event);
    }

    // Highlight the matching row in the table
    for (int row = 0; row < m_table->rowCount(); ++row) {
        if (m_table->item(row, 0)->data(Qt::UserRole).toInt() == event.productId) {
            m_table->item(row, 3)->setText(tr("Alert!"));
            m_table->item(row, 3)->setIcon(QIcon(":/assets/icons/status_alert.svg"));
            m_table->item(row, 3)->setForeground(QColor("#f38ba8")); // red
            break;
        }
    }
}

void MainWindow::onProductPriceChanged(int productId, float newPrice, float newDiscount) {
    std::string currency = "USD";
    for (auto& p : m_products) {
        if (p.id == productId) {
            p.currentPrice = newPrice;
            p.discount     = newDiscount;
            currency = p.currency;
            break;
        }
    }

    for (int row = 0; row < m_table->rowCount(); ++row) {
        if (m_table->item(row, 0)->data(Qt::UserRole).toInt() == productId) {
            m_table->item(row, 1)->setText(
                newPrice > 0 ? CurrencyUtils::formatPrice(newPrice, currency)
                             : QStringLiteral("—"));
            if (auto* discItem = m_table->item(row, 2)) {
                if (newDiscount > 0) {
                    discItem->setText(QString("-%1%").arg(static_cast<int>(newDiscount)));
                    discItem->setForeground(QColor("#a6e3a1"));
                } else {
                    discItem->setText(QStringLiteral("—"));
                    discItem->setForeground(QColor("#6c7086"));
                }
            }
            m_table->item(row, 4)->setText(
                QDateTime::currentDateTime().toString("HH:mm:ss"));
            break;
        }
    }
    if (m_cardView) {
        m_cardView->updatePriceFor(productId, newPrice, newDiscount, currency);
    }
    if (m_detailPane) {
        // Refresh the detail pane if it's showing this product.
        if (const Product* p = findProduct(productId)) {
            m_detailPane->showProduct(*p);
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

void MainWindow::changeEvent(QEvent* event) {
    if (event->type() == QEvent::WindowStateChange) {
        Qt::WindowStates state = windowState();
        // Only persist when settling into a non-minimized state.
        // Minimizing fires this event too; ignoring it preserves the
        // fullscreen flag so tray-restore works correctly.
        if (!(state & Qt::WindowMinimized)) {
            SettingsProvider::instance().setWasFullscreen(
                bool(state & Qt::WindowFullScreen));
        }
    }
    QMainWindow::changeEvent(event);
}

void MainWindow::showFromTray() {
    if (SettingsProvider::instance().wasFullscreen()) {
        showFullScreen();
    } else {
        showNormal();
    }
    raise();
    activateWindow();
}

// -- Layout switching ---------------------------------------------------------

void MainWindow::setLayoutCards() {
    if (m_viewStack) m_viewStack->setCurrentIndex(0);
    // Sync the segmented toggle when called programmatically (e.g. from the
    // "Try it" announcement action) so the visual state stays consistent.
    if (m_layoutCardsBtn) m_layoutCardsBtn->setChecked(true);
    SettingsProvider::instance().setLayoutMode(SettingsProvider::LayoutMode::Cards);
    refreshAll();
}

void MainWindow::setLayoutTable() {
    if (m_viewStack) m_viewStack->setCurrentIndex(1);
    if (m_layoutTableBtn) m_layoutTableBtn->setChecked(true);
    SettingsProvider::instance().setLayoutMode(SettingsProvider::LayoutMode::Table);
    refreshAll();
}

void MainWindow::refreshAll() {
    refreshTable();
    if (m_cardView) m_cardView->setProducts(m_products);
    onProductSelectionChanged();
}

void MainWindow::onProductSelectionChanged() {
    if (!m_detailPane) return;
    int id = -1;
    if (m_viewStack && m_viewStack->currentIndex() == 0 && m_cardView) {
        auto ids = m_cardView->selectedProductIds();
        if (!ids.isEmpty()) id = ids.first();
    } else if (m_table && m_table->currentRow() >= 0) {
        auto* item = m_table->item(m_table->currentRow(), 0);
        if (item) id = item->data(Qt::UserRole).toInt();
    }
    if (id < 0) {
        m_detailPane->clear();
        return;
    }
    if (const Product* p = findProduct(id)) {
        m_detailPane->showProduct(*p);
    }
}

const Product* MainWindow::findProduct(int productId) const {
    for (const auto& p : m_products) {
        if (p.id == productId) return &p;
    }
    return nullptr;
}

QList<int> MainWindow::currentSelectedIds() const {
    // Read selection from whichever view is currently visible so the toolbar
    // buttons (Edit / Remove / Check Now) work the same in both layouts.
    if (m_viewStack && m_viewStack->currentIndex() == 0 && m_cardView) {
        return m_cardView->selectedProductIds();
    }
    QSet<int> rows;
    for (auto* item : m_table->selectedItems()) rows.insert(item->row());
    QList<int> ids;
    for (int row : rows) {
        if (auto* item = m_table->item(row, 0)) {
            ids.append(item->data(Qt::UserRole).toInt());
        }
    }
    return ids;
}

void MainWindow::editProductById(int productId) {
    const Product* p = findProduct(productId);
    if (!p) return;
    ProductDialog dlg(m_controller->pluginManager(), *p, this);
    if (dlg.exec() != QDialog::Accepted) return;
    Product updated = dlg.getProduct();
    updated.id = productId;
    if (!m_controller->editProduct(updated)) {
        QMessageBox::critical(this, tr("Error"), tr("Failed to update product."));
        return;
    }
    for (auto& q : m_products) {
        if (q.id == productId) { q = updated; break; }
    }
    refreshAll();
}

void MainWindow::checkNowFor(int productId) {
    m_controller->checkNow(productId);
}

void MainWindow::showCardContextMenu(int productId, const QPoint& globalPos) {
    QMenu menu(this);
    QAction* aEdit   = menu.addAction(QIcon(":/assets/icons/edit.svg"),    tr("Edit"));
    QAction* aRemove = menu.addAction(QIcon(":/assets/icons/remove.svg"),  tr("Remove"));
    QAction* aCheck  = menu.addAction(QIcon(":/assets/icons/refresh.svg"), tr("Check Now"));
    QAction* aOpen   = menu.addAction(tr("Open in browser"));

    QAction* chosen = menu.exec(globalPos);
    if (!chosen) return;
    if (chosen == aEdit) {
        editProductById(productId);
    } else if (chosen == aRemove) {
        // Reuse the multi-select removal flow with a single id selected.
        const Product* p = findProduct(productId);
        if (!p) return;
        auto reply = QMessageBox::question(this, tr("Confirm"),
            tr("Remove \"%1\"? This will also delete its alert history.")
                .arg(QString::fromStdString(p->name)),
            QMessageBox::Yes | QMessageBox::No);
        if (reply == QMessageBox::Yes) {
            m_controller->removeProduct(productId);
        }
    } else if (chosen == aCheck) {
        checkNowFor(productId);
    } else if (chosen == aOpen) {
        if (const Product* p = findProduct(productId)) {
            QDesktopServices::openUrl(QUrl(QString::fromStdString(p->url)));
        }
    }
}

// -- Announcements -----------------------------------------------------------

void MainWindow::setupAnnouncements() {
    m_announcements = new AnnouncementCenter(this);
    m_announcements->setAnchor(this);
    m_announcements->registerDefaults();
    connect(m_announcements, &AnnouncementCenter::primaryActionTriggered,
            this, [this](const QString& actionId) {
        if (actionId == "switch_to_cards") {
            setLayoutCards();
        } else if (actionId == "open_add_product") {
            addProduct();
        } else if (actionId == "open_settings") {
            showSettings();
        }
    });
    // Delay so the window is fully painted before the toast slides in.
    QTimer::singleShot(1500, m_announcements, &AnnouncementCenter::showPending);
}
