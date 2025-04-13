#include "gui/MainWindow.hpp"
#include <QTableWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QMenuBar>
#include <QAction>
#include <QMessageBox>
#include <QHeaderView>
#include <QTableWidgetItem>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setupUi();
    setupMenu();
}

MainWindow::~MainWindow() {}

void MainWindow::setupUi() {
    // Create central widget
    QWidget* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    // Create table to display products
    productTable = new QTableWidget(centralWidget);
    productTable->setColumnCount(6);
    productTable->setHorizontalHeaderLabels({"ID", "Name", "Source", "Price", "Discount", "Check Interval (s)"});
    productTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    // Layout to hold the table
    QVBoxLayout* layout = new QVBoxLayout(centralWidget);
    layout->addWidget(productTable);

    // Add product button
    QPushButton* addButton = new QPushButton("Add Product", this);
    layout->addWidget(addButton);
    connect(addButton, &QPushButton::clicked, this, &MainWindow::addProduct);

    // Remove product button
    QPushButton* removeButton = new QPushButton("Remove Product", this);
    layout->addWidget(removeButton);
    connect(removeButton, &QPushButton::clicked, this, &MainWindow::removeProduct);
}

void MainWindow::setupMenu() {
    QMenuBar* menuBar = this->menuBar();
    QMenu* fileMenu = menuBar->addMenu("File");

    QAction* addAction = new QAction("Add Product", this);
    fileMenu->addAction(addAction);
    connect(addAction, &QAction::triggered, this, &MainWindow::addProduct);

    QAction* removeAction = new QAction("Remove Product", this);
    fileMenu->addAction(removeAction);
    connect(removeAction, &QAction::triggered, this, &MainWindow::removeProduct);
}

void MainWindow::addProduct() {
    ProductDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        Product product = dialog.getProduct();
        products.push_back(product);
        updateProductList();
    }
}

void MainWindow::removeProduct() {
    int row = productTable->currentRow();
    if (row >= 0) {
        products.erase(products.begin() + row);
        updateProductList();
    } else {
        QMessageBox::warning(this, "No Product Selected", "Please select a product to remove.");
    }
}

void MainWindow::updateProductList() {
    productTable->setRowCount(0); // Clear the table
    for (const auto& product : products) {
        int row = productTable->rowCount();
        productTable->insertRow(row);

        productTable->setItem(row, 0, new QTableWidgetItem(QString::number(product.id)));
        productTable->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(product.name)));
        productTable->setItem(row, 2, new QTableWidgetItem(QString::fromStdString(product.source)));
        productTable->setItem(row, 3, new QTableWidgetItem(QString::number(product.currentPrice)));
        productTable->setItem(row, 4, new QTableWidgetItem(QString::number(product.discount)));
        productTable->setItem(row, 5, new QTableWidgetItem(QString::number(product.checkInterval.count())));
    }
}
