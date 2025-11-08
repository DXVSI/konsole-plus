// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2025

#ifndef CURSORTRAIL_H
#define CURSORTRAIL_H

#include <QColor>
#include <QRectF>
#include <QPolygonF>
#include <QElapsedTimer>

namespace Konsole
{

class CursorTrail
{
public:
    CursorTrail();

    void update(const QRectF &cursorRect, qint64 elapsedMs);

    void reset();

    bool needsRender() const;

    // Returns polygon that stretches from old to new cursor position
    QPolygonF trailPolygon() const;

    QRectF cursorRect() const;

    qreal opacity() const;

    // Setters for configuration
    void setAnimationSpeed(qreal speed) { m_animationSpeed = speed; }
    void setFadeSpeed(qreal speed) { m_fadeSpeed = speed; }
    void setTrailWidth(qreal width) { m_trailWidth = width; }

private:
    // Cursor size (constant during animation)
    qreal m_cursorWidth;
    qreal m_cursorHeight;

    // Target cursor position (center)
    qreal m_targetCenterX;
    qreal m_targetCenterY;

    // Current trail position (center, following cursor)
    qreal m_trailCenterX;
    qreal m_trailCenterY;

    qint64 m_lastUpdateTime;
    qreal m_opacity;
    bool m_needsRender;

    // Configurable parameters
    qreal m_animationSpeed;
    qreal m_fadeSpeed;
    qreal m_trailWidth;

    static constexpr qreal DECAY_FAST = 0.1;  // From Kitty defaults
    static constexpr qreal DECAY_SLOW = 0.4;  // From Kitty defaults
    static constexpr qreal DISTANCE_THRESHOLD = 5.0;  // Start trail after 5 pixels movement
};

}

#endif
