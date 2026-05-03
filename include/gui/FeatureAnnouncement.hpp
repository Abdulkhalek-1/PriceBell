#pragma once

#include <QString>

// One feature-introduction popup, shown once per install via AnnouncementCenter.
// New features in future versions just append to the registry in
// AnnouncementCenter.cpp — no other code changes required.
struct FeatureAnnouncement {
    QString id;              // stable id, e.g. "v1.4.0_layout_switcher"
    QString minVersion;      // app version this was introduced in
    QString title;
    QString body;            // 1–3 sentences, plain text
    QString iconResource;    // optional, e.g. ":/assets/icons/layout.svg"
    QString primaryAction;   // optional, e.g. "Try it"
    QString primaryActionId; // identifier emitted on click
    QString secondaryAction; // typically "Got it"
};
