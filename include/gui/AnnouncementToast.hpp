#pragma once

#include "gui/FeatureAnnouncement.hpp"

#include <QFrame>

class QPropertyAnimation;
class QWidget;

// Frameless, non-modal "look what's new" toast. Slides in from the bottom-right
// of an anchor window. Two buttons: primary (e.g. "Try it") and secondary
// (typically "Got it"). Both dismiss the toast; primary also emits actionId.
class AnnouncementToast : public QFrame {
    Q_OBJECT
public:
    AnnouncementToast(const FeatureAnnouncement& announcement,
                      QWidget* anchor,
                      QWidget* parent = nullptr);

    void showAnimated();

signals:
    void primaryClicked(QString actionId);
    void dismissed();

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    void positionRelativeToAnchor();

    FeatureAnnouncement m_announcement;
    QWidget*            m_anchor;
    QPropertyAnimation* m_slideIn = nullptr;
};
