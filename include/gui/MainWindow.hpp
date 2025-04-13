#pragma once

#include <QMainWindow>
#include <QTableWidget>
#include <QAction>
#include <QMenuBar>
#include <QMessageBox>
#include "core/DataStructs.hpp"
#include "gui/ProductDialog.hpp"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

private slots:
    void addProduct();
    void removeProduct();
    void updateProductList();

private:
    void setupUi();
    void setupMenu();

    QTableWidget* productTable;
    std::vector<Product> products;
};
