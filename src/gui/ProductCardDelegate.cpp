#include "gui/ProductCardDelegate.hpp"
#include "gui/ProductCardView.hpp"

#include <QFont>
#include <QFontMetrics>
#include <QIcon>
#include <QPainter>
#include <QPainterPath>

namespace {
constexpr int kCardW       = 320;
constexpr int kCardH       = 152;
constexpr int kCardMargin  = 4;
constexpr int kPad         = 14;
constexpr int kIconSize    = 22;
constexpr int kRowGap      = 8;
constexpr int kTopRowH     = kIconSize;
constexpr int kSubRowH     = 14;
constexpr int kPriceRowH   = 32;
constexpr int kBottomRowH  = 16;
} // namespace

ProductCardDelegate::ProductCardDelegate(QObject* parent)
    : QStyledItemDelegate(parent)
{}

QSize ProductCardDelegate::sizeHint(const QStyleOptionViewItem& /*option*/,
                                    const QModelIndex& /*index*/) const {
    return QSize(kCardW, kCardH);
}

static void drawChip(QPainter* p, const QRect& r, const QString& text,
                     const QColor& bg, const QColor& fg) {
    QPainterPath path;
    path.addRoundedRect(r, r.height() / 2.0, r.height() / 2.0);
    p->fillPath(path, bg);
    p->setPen(fg);
    p->drawText(r, Qt::AlignCenter, text);
}

void ProductCardDelegate::paint(QPainter* painter, const QStyleOptionViewItem& option,
                                const QModelIndex& index) const {
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setRenderHint(QPainter::TextAntialiasing);

    const QRect rect = option.rect.adjusted(kCardMargin, kCardMargin,
                                            -kCardMargin, -kCardMargin);

    // -- Card background + border ---------------------------------------------
    QPainterPath cardPath;
    cardPath.addRoundedRect(rect, 8, 8);
    QColor bg = QColor("#181825");
    if (option.state & QStyle::State_Selected)        bg = QColor("#313244");
    else if (option.state & QStyle::State_MouseOver)  bg = QColor("#1e1e2e");
    painter->fillPath(cardPath, bg);

    QColor border = (option.state & QStyle::State_Selected)
                        ? QColor("#cba6f7") : QColor("#313244");
    painter->setPen(QPen(border, (option.state & QStyle::State_Selected) ? 2 : 1));
    painter->drawPath(cardPath);

    // Clip everything below to the card's interior so a too-long string can
    // never bleed onto the next card.
    painter->setClipPath(cardPath);

    const QRect inner = rect.adjusted(kPad, kPad, -kPad, -kPad);

    // Defensive: if the card is too small to render anything sensible, bail.
    if (inner.width() < 80 || inner.height() < 60) {
        painter->restore();
        return;
    }

    // Pre-compute fonts once so chip widths are correct relative to actual font.
    const QFont baseFont   = painter->font();
    QFont chipFont = baseFont; chipFont.setPointSize(baseFont.pointSize() - 1); chipFont.setBold(true);
    QFont nameFont = baseFont; nameFont.setBold(true); nameFont.setPointSize(baseFont.pointSize() + 1);
    QFont subFont  = baseFont; subFont.setPointSize(baseFont.pointSize() - 2);
    QFont priceFont= baseFont; priceFont.setBold(true); priceFont.setPointSize(baseFont.pointSize() + 6);
    QFont origFont = baseFont; origFont.setPointSize(baseFont.pointSize() - 1); origFont.setStrikeOut(true);
    QFont botFont  = baseFont; botFont.setPointSize(baseFont.pointSize() - 1);

    const QFontMetrics chipFm(chipFont);
    const QFontMetrics nameFm(nameFont);
    const QFontMetrics subFm(subFont);
    const QFontMetrics priceFm(priceFont);
    const QFontMetrics origFm(origFont);
    const QFontMetrics botFm(botFont);

    // -- Top row: source icon + name + source-name chip (right) ---------------
    int topY = inner.top();
    QRect iconRect(inner.left(), topY, kIconSize, kIconSize);
    const QString iconPath = index.data(ProductCardView::SourceIconPathRole).toString();
    if (!iconPath.isEmpty()) QIcon(iconPath).paint(painter, iconRect);

    const QString sourceLabel = index.data(ProductCardView::SourceIdRole).toString();
    const int chipPaddingX = 10;
    int chipDesiredW = chipFm.horizontalAdvance(sourceLabel) + chipPaddingX * 2;
    // Cap chip width so the name always gets at least 80 px to render in.
    const int nameLeft = iconRect.right() + 8;
    const int maxChipW = std::max(0, inner.right() - nameLeft - 8 - 80);
    int chipW = std::min(chipDesiredW, maxChipW);
    QString chipText = sourceLabel;
    if (chipW < chipDesiredW) {
        chipText = chipFm.elidedText(sourceLabel, Qt::ElideRight, std::max(0, chipW - chipPaddingX * 2));
    }
    QRect chipRect(inner.right() - chipW, topY + (kIconSize - 18) / 2, chipW, 18);
    if (chipW > 0) {
        painter->setFont(chipFont);
        drawChip(painter, chipRect, chipText, QColor("#45475a"), QColor("#cdd6f4"));
    }

    // Name fills the gap between icon and chip.
    const int nameRight = (chipW > 0 ? chipRect.left() : inner.right()) - 8;
    QRect nameRect(nameLeft, topY, std::max(0, nameRight - nameLeft), kIconSize);
    painter->setFont(nameFont);
    painter->setPen(QColor("#cdd6f4"));
    const QString name = index.data(ProductCardView::NameRole).toString();
    painter->drawText(nameRect, Qt::AlignVCenter | Qt::AlignLeft,
                      nameFm.elidedText(name, Qt::ElideRight, nameRect.width()));

    // -- URL host subtitle (just under the name) ------------------------------
    int subY = topY + kTopRowH + 2;
    QRect hostRect(nameLeft, subY, std::max(0, inner.right() - nameLeft), kSubRowH);
    painter->setFont(subFont);
    painter->setPen(QColor("#6c7086"));
    painter->drawText(hostRect, Qt::AlignLeft | Qt::AlignVCenter,
                      subFm.elidedText(index.data(ProductCardView::UrlHostRole).toString(),
                                       Qt::ElideRight, hostRect.width()));

    // -- Middle row: discount chip on the right anchors the layout, then ------
    //    price + (optional) strikethrough fill the remaining left side.
    int priceTop = subY + kSubRowH + kRowGap;
    QRect priceBand(inner.left(), priceTop, inner.width(), kPriceRowH);

    int discInt = index.data(ProductCardView::DiscountRole).toInt();
    QRect dRect;
    if (discInt > 0) {
        QString dText = QString("-%1%").arg(discInt);
        int dW = chipFm.horizontalAdvance(dText) + chipPaddingX * 2;
        dRect = QRect(priceBand.right() - dW,
                      priceBand.top() + (priceBand.height() - 22) / 2,
                      dW, 22);
        painter->setFont(chipFont);
        drawChip(painter, dRect, dText, QColor("#a6e3a1"), QColor("#1e1e2e"));
    }

    const int priceMaxRight = (discInt > 0 ? dRect.left() - 10 : priceBand.right());
    const int priceMaxW     = std::max(0, priceMaxRight - priceBand.left());
    const QString priceStr  = index.data(ProductCardView::PriceRole).toString();
    const QString origStr   = index.data(ProductCardView::OriginalPriceRole).toString();

    painter->setFont(priceFont);
    painter->setPen(QColor("#cdd6f4"));
    int priceTextW = std::min(priceFm.horizontalAdvance(priceStr), priceMaxW);
    QRect priceRect(priceBand.left(),
                    priceBand.top() + (priceBand.height() - priceFm.height()) / 2,
                    priceTextW, priceFm.height());
    painter->drawText(priceRect, Qt::AlignLeft | Qt::AlignVCenter,
                      priceFm.elidedText(priceStr, Qt::ElideRight, priceRect.width()));

    if (!origStr.isEmpty()) {
        int origLeft  = priceRect.right() + 8;
        int origAvail = std::max(0, priceMaxRight - origLeft);
        if (origAvail > 20) {
            painter->setFont(origFont);
            painter->setPen(QColor("#6c7086"));
            QRect origRect(origLeft,
                           priceRect.bottom() - origFm.height(),
                           origAvail, origFm.height());
            painter->drawText(origRect, Qt::AlignLeft | Qt::AlignBottom,
                              origFm.elidedText(origStr, Qt::ElideRight, origAvail));
        }
    }

    // -- Bottom row: status (left) | last checked (right) ---------------------
    int botY = inner.bottom() - kBottomRowH;
    painter->setFont(botFont);
    const QString statusStr = index.data(ProductCardView::StatusTextRole).toString();
    const QString lastStr   = index.data(ProductCardView::LastCheckedRelRole).toString();
    int statusW = botFm.horizontalAdvance(statusStr);
    int lastW   = botFm.horizontalAdvance(lastStr);

    // Reserve right-side space for the timestamp first; status takes whatever's
    // left so the two don't overlap on narrow cards or when text grows.
    int rightAvail = std::min(lastW, inner.width() - 24);
    QRect timeRect(inner.right() - rightAvail, botY, rightAvail, kBottomRowH);
    int statusAvail = std::max(0, timeRect.left() - inner.left() - 12);
    QRect statusRect(inner.left(), botY, statusAvail, kBottomRowH);

    QColor statusColor = index.data(ProductCardView::StatusColorRole).value<QColor>();
    if (!statusColor.isValid()) statusColor = QColor("#a6e3a1");
    painter->setPen(statusColor);
    painter->drawText(statusRect, Qt::AlignLeft | Qt::AlignVCenter,
                      botFm.elidedText(statusStr, Qt::ElideRight, statusRect.width()));

    painter->setPen(QColor("#6c7086"));
    painter->drawText(timeRect, Qt::AlignRight | Qt::AlignVCenter,
                      botFm.elidedText(lastStr, Qt::ElideRight, timeRect.width()));

    painter->restore();
}
