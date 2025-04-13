# **PriceBell** - Price Tracker Application

PriceBell is a price tracker application that monitors prices and discounts of various products from platforms like Steam and Udemy. Users can set filters based on price or discount, specify the check interval, and track products in real-time. The application provides a GUI to manage products, set filters, and view product information.

## Features

- **Add Products**: Add products from sources like Steam, Udemy, etc.
- **Filters**: Apply filters on price (e.g., "Price >= $100") and discount (e.g., "Discount >= 20%").
- **Check Interval**: Set the time interval for checking price updates.
- **GUI Interface**: Manage products and filters through a user-friendly graphical interface built with Qt.
- **Remove Products**: Remove selected products from the tracking list.

## Prerequisites

- **Qt 5** or later
- **CMake** 3.x or later
- **C++** compiler (GCC or Clang)
- **CMake** to build the project

## Getting Started

### 1. Clone the Repository

```bash
git clone https://github.com/yourusername/PriceBell.git
cd PriceBell
```

### 2. Install Dependencies

Make sure you have **Qt 5** and **CMake** installed. On Linux, you can install them using your package manager.

#### For Ubuntu/Debian-based systems:
```bash
sudo apt update
sudo apt install qt5-qmake qtbase5-dev qtchooser cmake build-essential
```

#### For Arch Linux:
```bash
sudo pacman -S qt5-base qt5-tools cmake base-devel
```

### 3. Build the Project

Create a build directory and compile the application:

```bash
mkdir build
cd build
cmake ..
make
```

### 4. Run the Application

Once the build is complete, you can run the application:

```bash
./PriceBell
```

## Usage

1. **Add Product**: Click "Add Product" from the menu or use the button to open a dialog and input product details (name, source, filters, and check interval).
2. **Product List**: The main window displays a table of all added products with details like name, source, price, discount, and check interval.
3. **Remove Product**: Select a product from the table and click "Remove Product" to delete it.
4. **Filters**: Apply filters like price >= $100 or discount >= 20% when adding products.
5. **Check Interval**: Set the interval (in seconds) to check product prices.

## Project Structure

```bash
PriceBell/
├── CMakeLists.txt          # CMake build file
├── README.md               # Project documentation
├── include/
│   ├── core/               # Core functionality (data structs, price checks)
│   └── gui/                # Qt GUI components
├── src/
│   ├── core/               # Core logic implementation
│   └── gui/                # GUI implementation
└── build/                  # Build directory (generated)
```

## Contributing

1. Fork this repository.
2. Create your feature branch (`git checkout -b feature/my-feature`).
3. Commit your changes (`git commit -am 'Add some feature'`).
4. Push to the branch (`git push origin feature/my-feature`).
5. Open a pull request.

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

---

Feel free to modify the **Installation** or **Usage** steps according to any specific details related to your project. You can also add any additional features you might want to highlight in the **Features** section.