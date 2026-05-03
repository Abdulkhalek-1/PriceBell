#include "gui/AnnouncementToast.hpp"

#include <QCloseEvent>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QVBoxLayout>

AnnouncementToast::AnnouncementToast(const FeatureAnnouncement& announcement,
                                     QWidget* anchor, QWidget* parent)
    : QFrame(parent), m_announcement(announcement), m_anchor(anchor)
{
    setObjectName("announcementToast");
    // SubWindow + FramelessWindowHint keeps the toast borderless and on top
    // without grabbing focus from the main window.
    setWindowFlags(Qt::SubWindow | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_DeleteOnClose);
    setAttribute(Qt::WA_ShowWithoutActivating);
    setFixedWidth(360);
    setMinimumHeight(120);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(14, 12, 14, 12);
    root->setSpacing(8);

    auto* topRow = new QHBoxLayout();
    if (!announcement.iconResource.isEmpty()) {
        auto* icon = new QLabel(this);
        icon->setPixmap(QIcon(announcement.iconResource).pixmap(20, 20));
        icon->setFixedSize(20, 20);
        topRow->addWidget(icon);
    }
    auto* title = new QLabel(announcement.title, this);
    title->setObjectName("announcementTitle");
    title->setWordWrap(true);
    topRow->addWidget(title, 1);
    root->addLayout(topRow);

    auto* body = new QLabel(announcement.body, this);
    body->setObjectName("announcementBody");
    body->setWordWrap(true);
    root->addWidget(body);

    auto* btnRow = new QHBoxLayout();
    btnRow->addStretch();

    if (!announcement.primaryAction.isEmpty()) {
        auto* primary = new QPushButton(announcement.primaryAction, this);
        primary->setDefault(true);
        connect(primary, &QPushButton::clicked, this, [this]() {
            emit primaryClicked(m_announcement.primaryActionId);
            close();
        });
        btnRow->addWidget(primary);
    }

    QString secondaryText = announcement.secondaryAction.isEmpty()
                                ? tr("Got it")
                                : announcement.secondaryAction;
    auto* secondary = new QPushButton(secondaryText, this);
    connect(secondary, &QPushButton::clicked, this, &QWidget::close);
    btnRow->addWidget(secondary);

    root->addLayout(btnRow);
}

void AnnouncementToast::positionRelativeToAnchor() {
    if (!m_anchor) return;
    adjustSize();
    QRect anchorRect = m_anchor->geometry();
    QPoint topLeft = m_anchor->mapToGlobal(QPoint(
        anchorRect.width()  - width() - 24,
        anchorRect.height() - height() - 24));
    move(topLeft);
}

void AnnouncementToast::showAnimated() {
    positionRelativeToAnchor();

    // Slide up by 16 px while fading in via opacity wouldn't work without
    // QGraphicsOpacityEffect; geometry slide alone reads as smooth enough.
    QPoint endPos = pos();
    QPoint startPos = endPos + QPoint(0, 16);
    move(startPos);

    show();

    m_slideIn = new QPropertyAnimation(this, "pos", this);
    m_slideIn->setDuration(220);
    m_slideIn->setStartValue(startPos);
    m_slideIn->setEndValue(endPos);
    m_slideIn->setEasingCurve(QEasingCurve::OutCubic);
    m_slideIn->start(QAbstractAnimation::DeleteWhenStopped);
}

void AnnouncementToast::closeEvent(QCloseEvent* event) {
    emit dismissed();
    QFrame::closeEvent(event);
}
