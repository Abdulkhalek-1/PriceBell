#include "gui/ProductCardView.hpp"
#include "gui/ProductCardDelegate.hpp"
#include "utils/CurrencyUtils.hpp"

#include <QContextMenuEvent>
#include <QDateTime>
#include <QListWidgetItem>
#include <QMouseEvent>

ProductCardView::ProductCardView(QWidget* parent)
    : QListWidget(parent)
{
    setObjectName("productCardView");
    setViewMode(QListView::IconMode);
    setResizeMode(QListView::Adjust);
    setMovement(QListView::Static);
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setSpacing(8);
    setUniformItemSizes(true);
    setItemDelegate(new ProductCardDelegate(this));
    setMouseTracking(true);
}

static QString sourceIconFor(SourceType source, const std::string& pluginId) {
    switch (source) {
        case SourceType::STEAM:   return ":/assets/icons/source_steam.svg";
        case SourceType::UDEMY:   return ":/assets/icons/source_udemy.svg";
        case SourceType::AMAZON:  return ":/assets/icons/source_amazon.svg";
        case SourceType::PLUGIN:  return ":/assets/icons/source_plugin.svg";
        case SourceType::GENERIC: return ":/assets/icons/source_generic.svg";
    }
    (void)pluginId;
    return ":/assets/icons/source_generic.svg";
}

static QString sourceLabelFor(const Product& p) {
    switch (p.source) {
        case SourceType::STEAM:   return "Steam";
        case SourceType::UDEMY:   return "Udemy";
        case SourceType::AMAZON:  return "Amazon";
        case SourceType::PLUGIN:  return QString::fromStdString(p.sourcePluginId);
        case SourceType::GENERIC: return "Generic";
    }
    return "";
}

QString ProductCardView::relativeTime(const std::chrono::system_clock::time_point& tp) {
    using namespace std::chrono;
    auto epoch = tp.time_since_epoch().count();
    if (epoch == 0) return tr("Never");
    auto secs = duration_cast<seconds>(system_clock::now() - tp).count();
    if (secs < 0)         return tr("just now");
    if (secs < 60)        return tr("just now");
    if (secs < 3600)      return tr("%1m ago").arg(secs / 60);
    if (secs < 86400)     return tr("%1h ago").arg(secs / 3600);
    return tr("%1d ago").arg(secs / 86400);
}

static QString hostFromUrl(const QString& url) {
    int schemeEnd = url.indexOf("://");
    int start = schemeEnd >= 0 ? schemeEnd + 3 : 0;
    int end = url.indexOf('/', start);
    QString host = end > start ? url.mid(start, end - start) : url.mid(start);
    if (host.startsWith("www.")) host.remove(0, 4);
    return host;
}

void ProductCardView::setProducts(const std::vector<Product>& products) {
    clear();
    for (const auto& p : products) {
        auto* item = new QListWidgetItem(this);
        item->setData(ProductIdRole, p.id);
        item->setData(NameRole, QString::fromStdString(p.name));
        item->setData(UrlHostRole, hostFromUrl(QString::fromStdString(p.url)));
        item->setData(SourceIdRole, sourceLabelFor(p));
        item->setData(SourceIconPathRole, sourceIconFor(p.source, p.sourcePluginId));
        item->setData(CurrencyRole, QString::fromStdString(p.currency));

        QString priceText = p.currentPrice > 0
            ? CurrencyUtils::formatPrice(p.currentPrice, p.currency)
            : tr("—");
        item->setData(PriceRole, priceText);

        QString origText;
        if (p.originalPrice > 0 && p.originalPrice > p.currentPrice) {
            origText = CurrencyUtils::formatPrice(p.originalPrice, p.currency);
        }
        item->setData(OriginalPriceRole, origText);

        int discInt = static_cast<int>(p.discount);
        item->setData(DiscountRole, p.discount > 0 ? discInt : -1);

        QString status;
        QColor   statusColor("#a6e3a1");
        if (!p.isActive) {
            status = tr("Paused");
            statusColor = QColor("#6c7086");
        } else if (p.lastChecked.time_since_epoch().count() == 0) {
            status = tr("Pending");
            statusColor = QColor("#cba6f7");
        } else {
            status = tr("Watching");
        }
        item->setData(StatusTextRole, status);
        item->setData(StatusColorRole, statusColor);

        item->setData(LastCheckedRelRole, relativeTime(p.lastChecked));

        addItem(item);
    }
}

int ProductCardView::findRow(int productId) const {
    for (int i = 0; i < count(); ++i) {
        if (item(i)->data(ProductIdRole).toInt() == productId) return i;
    }
    return -1;
}

void ProductCardView::updatePriceFor(int productId, float newPrice, float newDiscount,
                                     const std::string& currency) {
    int row = findRow(productId);
    if (row < 0) return;
    QListWidgetItem* it = item(row);
    QString cur = QString::fromStdString(currency);
    if (cur.isEmpty()) cur = it->data(CurrencyRole).toString();

    it->setData(PriceRole, newPrice > 0
                ? CurrencyUtils::formatPrice(newPrice, cur.toStdString())
                : tr("—"));
    int discInt = static_cast<int>(newDiscount);
    it->setData(DiscountRole, newDiscount > 0 ? discInt : -1);
    it->setData(LastCheckedRelRole, tr("just now"));
}

void ProductCardView::setStatusFor(int productId, const QString& text, const QColor& color) {
    int row = findRow(productId);
    if (row < 0) return;
    QListWidgetItem* it = item(row);
    it->setData(StatusTextRole, text);
    it->setData(StatusColorRole, color);
}

int ProductCardView::productIdAt(int row) const {
    if (row < 0 || row >= count()) return -1;
    return item(row)->data(ProductIdRole).toInt();
}

QList<int> ProductCardView::selectedProductIds() const {
    QList<int> ids;
    for (auto* it : selectedItems()) {
        ids.append(it->data(ProductIdRole).toInt());
    }
    return ids;
}

void ProductCardView::mouseDoubleClickEvent(QMouseEvent* event) {
    QListWidgetItem* it = itemAt(event->pos());
    if (it) {
        emit productActivated(it->data(ProductIdRole).toInt());
        return;
    }
    QListWidget::mouseDoubleClickEvent(event);
}

void ProductCardView::contextMenuEvent(QContextMenuEvent* event) {
    QListWidgetItem* it = itemAt(event->pos());
    if (it) {
        emit productContextRequested(it->data(ProductIdRole).toInt(), event->globalPos());
        return;
    }
    QListWidget::contextMenuEvent(event);
}
