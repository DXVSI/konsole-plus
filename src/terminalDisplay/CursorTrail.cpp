// SPDX-License-Identifier: GPL-2.0-or-later
// SPDX-FileCopyrightText: 2025

#include "CursorTrail.h"
#include <QtMath>
#include <QDebug>
#include <QLineF>

using namespace Konsole;

CursorTrail::CursorTrail()
    : m_cursorWidth(0.0)
    , m_cursorHeight(0.0)
    , m_targetCenterX(0.0)
    , m_targetCenterY(0.0)
    , m_trailCenterX(0.0)
    , m_trailCenterY(0.0)
    , m_lastUpdateTime(0)
    , m_opacity(0.0)
    , m_needsRender(false)
    , m_animationSpeed(10.0)  // Default values
    , m_fadeSpeed(1.5)
    , m_trailWidth(0.4)
{
}

void CursorTrail::update(const QRectF &cursorRect, qint64 elapsedMs)
{
    // Update cursor size and target center position
    m_cursorWidth = cursorRect.width();
    m_cursorHeight = cursorRect.height();

    qreal newCenterX = cursorRect.center().x();
    qreal newCenterY = cursorRect.center().y();

    if (m_lastUpdateTime == 0) {
        // Initialize trail to cursor position
        m_targetCenterX = newCenterX;
        m_targetCenterY = newCenterY;
        m_trailCenterX = newCenterX;
        m_trailCenterY = newCenterY;
        m_lastUpdateTime = elapsedMs;
        m_opacity = 0.0;
        m_needsRender = false;
        return;
    }

    // Check if cursor actually moved
    const qreal moveDx = newCenterX - m_targetCenterX;
    const qreal moveDy = newCenterY - m_targetCenterY;
    const qreal moveDistance = qSqrt(moveDx * moveDx + moveDy * moveDy);

    // Update target position
    m_targetCenterX = newCenterX;
    m_targetCenterY = newCenterY;

    const qreal dt = (elapsedMs - m_lastUpdateTime) / 1000.0;
    m_lastUpdateTime = elapsedMs;

    if (dt <= 0.0 || dt > 1.0) {
        return;
    }

    // If cursor moved significantly, reset trail to old position
    if (moveDistance > DISTANCE_THRESHOLD) {
        // Trail starts from old cursor position
        // Don't update m_trailCenter yet - let it animate from old to new
        if (moveDistance > m_cursorWidth * 3) {
            // For very large jumps (like deleting a word), instantly position trail
            m_trailCenterX = m_targetCenterX - moveDx;
            m_trailCenterY = m_targetCenterY - moveDy;
        }
        m_opacity = 1.0;
        m_needsRender = true;
    }

    // Calculate distance from trail to cursor
    const qreal dx = m_targetCenterX - m_trailCenterX;
    const qreal dy = m_targetCenterY - m_trailCenterY;
    const qreal distance = qSqrt(dx * dx + dy * dy);

    if (distance > 1.0) {
        // Exponential ease-out animation (using configurable speed)
        const qreal step = 1.0 - qPow(2.0, -m_animationSpeed * dt / DECAY_SLOW);
        m_trailCenterX += dx * step;
        m_trailCenterY += dy * step;

        // Keep opacity high while animating
        m_opacity = 1.0;
        m_needsRender = true;
    } else {
        // Trail reached cursor, fade out (using configurable fade speed)
        m_opacity = qMax(0.0, m_opacity - dt * m_fadeSpeed);
        m_needsRender = m_opacity > 0.01;
    }
}

void CursorTrail::reset()
{
    m_cursorWidth = 0.0;
    m_cursorHeight = 0.0;
    m_targetCenterX = 0.0;
    m_targetCenterY = 0.0;
    m_trailCenterX = 0.0;
    m_trailCenterY = 0.0;
    m_lastUpdateTime = 0;
    m_opacity = 0.0;
    m_needsRender = false;
}

bool CursorTrail::needsRender() const
{
    return m_needsRender;
}

QPolygonF CursorTrail::trailPolygon() const
{
    // Create a polygon that stretches from trail position to cursor position
    // This creates a smooth transition effect

    QPolygonF polygon;

    // Calculate the direction vector
    qreal dx = m_targetCenterX - m_trailCenterX;
    qreal dy = m_targetCenterY - m_trailCenterY;
    qreal distance = qSqrt(dx * dx + dy * dy);

    if (distance < 0.1) {
        // If positions are too close, just return a small rect at trail position
        qreal halfWidth = m_cursorWidth * 0.10;
        qreal halfHeight = m_cursorHeight * 0.3;

        polygon << QPointF(m_trailCenterX - halfWidth, m_trailCenterY - halfHeight)
                << QPointF(m_trailCenterX + halfWidth, m_trailCenterY - halfHeight)
                << QPointF(m_trailCenterX + halfWidth, m_trailCenterY + halfHeight)
                << QPointF(m_trailCenterX - halfWidth, m_trailCenterY + halfHeight);
    } else {
        // Create a stretched polygon from trail to cursor
        // Create perpendicular vector for width
        qreal perpX = -dy / distance;
        qreal perpY = dx / distance;

        // Trail dimensions (using configurable width)
        qreal trailHeight = m_cursorHeight * 0.35;  // Height for vertical movement
        qreal trailWidth = m_cursorWidth * m_trailWidth;  // Width from config

        // Determine which dimension to use based on movement direction
        qreal trailThickness;
        if (qAbs(dx) > qAbs(dy)) {
            // Mostly horizontal movement - use height
            trailThickness = trailHeight * 0.5;  // Adjust this multiplier to change trail height
        } else {
            // Mostly vertical movement - use width
            trailThickness = trailWidth;
        }

        // Optional offset for trail position (relative to cursor size for proper scaling)
        qreal offsetX = -m_cursorWidth * 0.15;  // Horizontal offset relative to cursor width (-0.15 = 15% left)
        qreal offsetY = 0.0;  // Vertical offset: could use m_cursorHeight * factor for scaling

        // Four corners of the stretched trail with offset
        polygon << QPointF(m_trailCenterX - perpX * trailThickness + offsetX, m_trailCenterY - perpY * trailThickness + offsetY)
                << QPointF(m_targetCenterX - perpX * trailThickness + offsetX, m_targetCenterY - perpY * trailThickness + offsetY)
                << QPointF(m_targetCenterX + perpX * trailThickness + offsetX, m_targetCenterY + perpY * trailThickness + offsetY)
                << QPointF(m_trailCenterX + perpX * trailThickness + offsetX, m_trailCenterY + perpY * trailThickness + offsetY);
    }

    return polygon;
}

QRectF CursorTrail::cursorRect() const
{
    return QRectF(m_targetCenterX - m_cursorWidth * 0.5,
                  m_targetCenterY - m_cursorHeight * 0.5,
                  m_cursorWidth,
                  m_cursorHeight);
}

qreal CursorTrail::opacity() const
{
    return m_opacity;
}
