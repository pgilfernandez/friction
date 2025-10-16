/*
#
# Friction - https://friction.graphics
#
# Copyright (c) Ole-André Rodlie and contributors
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
# See 'README.md' for more information.
#
*/

#include "gridcontroller.h"

#include "skia/skqtconversions.h"

#include "include/core/SkCanvas.h"
#include "include/core/SkPaint.h"
#include "include/core/SkPoint.h"

#include <QPainter>
#include <QPen>
#include <QLineF>

#include <cmath>

#include <algorithm>

using namespace Friction::Core;

namespace {

double clampToRange(double value, double minValue, double maxValue)
{
    if (value < minValue) { return minValue; }
    if (value > maxValue) { return maxValue; }
    return value;
}

bool nearlyEqual(double lhs, double rhs)
{
    constexpr double eps = 1e-6;
    return std::abs(lhs - rhs) <= eps;
}

GridSettings sanitizeSettings(const GridSettings& in)
{
    GridSettings copy = in;
    if (copy.sizeX <= 0.0) { copy.sizeX = 1.0; }
    if (copy.sizeY <= 0.0) { copy.sizeY = 1.0; }
    if (copy.majorEvery < 1) { copy.majorEvery = 1; }
    if (copy.snapThresholdPx < 0) { copy.snapThresholdPx = 0; }
    auto ensureAnimatorColor = [](qsptr<ColorAnimator>& animator,
                                  const QColor& fallback)
    {
        if (!animator) {
            animator = enve::make_shared<ColorAnimator>();
        }
        QColor color = animator->getColor();
        if (!color.isValid()) {
            color = fallback;
        }
        const double alpha = clampToRange(static_cast<double>(color.alpha()), 0.0, 255.0);
        color.setAlpha(static_cast<int>(alpha));
        animator->setColor(color);
        return color;
    };
    const QColor minorFallback(255, 255, 255, 96);
    const QColor majorFallback(255, 255, 255, 160);
    ensureAnimatorColor(copy.colorAnimator, minorFallback);
    ensureAnimatorColor(copy.majorColorAnimator, majorFallback);
    return copy;
}

QColor scaledAlpha(const QColor& base, double factor)
{
    QColor c = base;
    factor = clampToRange(factor, 0.0, 1.0);
    c.setAlphaF(c.alphaF() * factor);
    return c;
}

double lineSpacingPx(const QTransform& worldToScreen,
                     qreal devicePixelRatio,
                     const QPointF& delta)
{
    const QPointF origin = worldToScreen.map(QPointF(0.0, 0.0));
    const QPointF mapped = worldToScreen.map(delta);
    return QLineF(origin, mapped).length() * devicePixelRatio;
}

double effectiveScale(const QTransform& worldToScreen)
{
    const double sx = std::hypot(worldToScreen.m11(), worldToScreen.m12());
    const double sy = std::hypot(worldToScreen.m21(), worldToScreen.m22());
    const double avg = (sx + sy) * 0.5;
    return avg > 0.0 ? avg : 1.0;
}

double fadeFactor(double spacingPx)
{
    constexpr double minVisible = 4.0;
    constexpr double fullVisible = 16.0;
    if (spacingPx <= minVisible) { return 0.0; }
    if (spacingPx >= fullVisible) { return 1.0; }
    return (spacingPx - minVisible) / (fullVisible - minVisible);
}

} // namespace

bool GridSettings::operator==(const GridSettings& other) const
{
    const QColor thisColor = colorAnimator ? colorAnimator->getColor() : QColor();
    const QColor otherColor = other.colorAnimator ? other.colorAnimator->getColor() : QColor();
    const QColor thisMajorColor = majorColorAnimator ? majorColorAnimator->getColor() : QColor();
    const QColor otherMajorColor = other.majorColorAnimator ? other.majorColorAnimator->getColor() : QColor();
    return nearlyEqual(sizeX, other.sizeX) &&
           nearlyEqual(sizeY, other.sizeY) &&
           nearlyEqual(originX, other.originX) &&
           nearlyEqual(originY, other.originY) &&
           snapThresholdPx == other.snapThresholdPx &&
           enabled == other.enabled &&
           show == other.show &&
           drawOnTop == other.drawOnTop &&
           majorEvery == other.majorEvery &&
           thisColor == otherColor &&
           thisMajorColor == otherMajorColor;
}




void GridController::drawGrid(QPainter* painter,
                              const QRectF& worldViewport,
                              const QTransform& worldToScreen,
                              const qreal devicePixelRatio) const
{
    const GridSettings sanitizedSettings = sanitizeSettings(settings);
    if (!painter || !sanitizedSettings.show) { return; }

    const QColor minorBase = sanitizedSettings.colorAnimator
        ? sanitizedSettings.colorAnimator->getColor()
        : QColor(255, 255, 255, 96);
    const QColor majorBase = sanitizedSettings.majorColorAnimator
        ? sanitizedSettings.majorColorAnimator->getColor()
        : minorBase;

    auto drawLine = [&](const QPointF& a,
                        const QPointF& b,
                        const bool major,
                        const Orientation orientation,
                        const double alphaFactor)
    {
        QPen pen(major ? majorBase : minorBase);
        pen.setCosmetic(true);
        pen.setColor(scaledAlpha(pen.color(), alphaFactor));
        painter->setPen(pen);
        painter->drawLine(a, b);
        Q_UNUSED(orientation)
    };

    forEachGridLine(worldViewport, worldToScreen, devicePixelRatio, drawLine);
}

void GridController::drawGrid(SkCanvas* canvas,
                              const QRectF& worldViewport,
                              const QTransform& worldToScreen,
                              const qreal devicePixelRatio) const
{
    const GridSettings sanitizedSettings = sanitizeSettings(settings);
    if (!canvas || !sanitizedSettings.show) { return; }

    const QColor minorBase = sanitizedSettings.colorAnimator
        ? sanitizedSettings.colorAnimator->getColor()
        : QColor(255, 255, 255, 96);
    const QColor majorBase = sanitizedSettings.majorColorAnimator
        ? sanitizedSettings.majorColorAnimator->getColor()
        : minorBase;
    const float strokeWidth = static_cast<float>(
        devicePixelRatio / effectiveScale(worldToScreen));

    auto drawLine = [&](const QPointF& a,
                        const QPointF& b,
                        const bool major,
                        const Orientation orientation,
                        const double alphaFactor)
    {
        SkPaint paint;
        paint.setStyle(SkPaint::kStroke_Style);
        paint.setStrokeWidth(strokeWidth);
        paint.setAntiAlias(false);
        const QColor base = major ? majorBase : minorBase;
        paint.setColor(toSkColor(scaledAlpha(base, alphaFactor)));
        canvas->drawLine(SkPoint::Make(static_cast<SkScalar>(a.x()),
                                       static_cast<SkScalar>(a.y())),
                         SkPoint::Make(static_cast<SkScalar>(b.x()),
                                       static_cast<SkScalar>(b.y())),
                         paint);
        Q_UNUSED(orientation)
    };

    forEachGridLine(worldViewport, worldToScreen, devicePixelRatio, drawLine);
}

QPointF GridController::maybeSnapPivot(const QPointF& pivotWorld,
                                       const QTransform& worldToScreen,
                                       const bool forceSnap,
                                       const bool bypassSnap) const
{
    const GridSettings sanitizedSettings = sanitizeSettings(settings);

    if (!sanitizedSettings.enabled || bypassSnap) {
        return pivotWorld;
    }

    const double sizeX = sanitizedSettings.sizeX;
    const double sizeY = sanitizedSettings.sizeY;
    if (sizeX <= 0.0 || sizeY <= 0.0) { return pivotWorld; }

    const double gx = sanitizedSettings.originX +
        std::round((pivotWorld.x() - sanitizedSettings.originX) / sizeX) * sizeX;
    const double gy = sanitizedSettings.originY +
        std::round((pivotWorld.y() - sanitizedSettings.originY) / sizeY) * sizeY;
    const QPointF snapped(gx, gy);

    if (forceSnap) { return snapped; }

    const QPointF screenPivot = worldToScreen.map(pivotWorld);
    const QPointF screenSnap = worldToScreen.map(snapped);
    const double dist = QLineF(screenPivot, screenSnap).length();

    if (dist <= sanitizedSettings.snapThresholdPx) {
        return snapped;
    }
    return pivotWorld;
}

template<typename DrawLineFunc>
void GridController::forEachGridLine(const QRectF& viewport,
                                     const QTransform& worldToScreen,
                                     const qreal devicePixelRatio,
                                     DrawLineFunc&& drawLine) const
{
    const GridSettings sanitizedSettings = sanitizeSettings(settings);
    if (!sanitizedSettings.show) { return; }

    const double sizeX = sanitizedSettings.sizeX;
    const double sizeY = sanitizedSettings.sizeY;
    if (sizeX <= 0.0 || sizeY <= 0.0) { return; }

    QRectF baseView = viewport.normalized();
    if (!baseView.isValid() || baseView.isEmpty()) { return; }
    const double expandX = std::max(baseView.width(), sizeX);
    const double expandY = std::max(baseView.height(), sizeY);
    QRectF view = baseView.adjusted(-expandX, -expandY, expandX, expandY);

    const int majorEvery = std::max(1, sanitizedSettings.majorEvery);

    const double spacingX = lineSpacingPx(worldToScreen, devicePixelRatio, {sizeX, 0.0});
    const double spacingY = lineSpacingPx(worldToScreen, devicePixelRatio, {0.0, sizeY});
    const double majorSpacingX = spacingX * majorEvery;
    const double majorSpacingY = spacingY * majorEvery;

    const double majorAlphaX = fadeFactor(majorSpacingX);
    const double majorAlphaY = fadeFactor(majorSpacingY);

    if (majorAlphaX <= 0.0 && majorAlphaY <= 0.0) {
        return;
    }

    const double minorAlphaX = fadeFactor(spacingX);
    const double minorAlphaY = fadeFactor(spacingY);

    auto firstAligned = [](double start,
                           double origin,
                           double spacing)
    {
        const double steps = std::floor((start - origin) / spacing);
        return origin + steps * spacing;
    };

    const double originX = sanitizedSettings.originX;
    const double originY = sanitizedSettings.originY;

    const double xBegin = firstAligned(view.left(), originX, sizeX);
    const double xEnd = view.right() + sizeX;

    for (double x = xBegin; x <= xEnd; x += sizeX) {
        const long long index = static_cast<long long>(
            std::llround((x - originX) / sizeX));
        const bool major = (index % majorEvery) == 0;
        const double alpha = major ? majorAlphaX : minorAlphaX;
        if (!major && alpha <= 0.0) { continue; }
        const QPointF top(x, view.top());
        const QPointF bottom(x, view.bottom());
        drawLine(top, bottom, major, Orientation::Vertical, alpha);
    }

    const double yBegin = firstAligned(view.top(), originY, sizeY);
    const double yEnd = view.bottom() + sizeY;

    for (double y = yBegin; y <= yEnd; y += sizeY) {
        const long long index = static_cast<long long>(
            std::llround((y - originY) / sizeY));
        const bool major = (index % majorEvery) == 0;
        const double alpha = major ? majorAlphaY : minorAlphaY;
        if (!major && alpha <= 0.0) { continue; }
        const QPointF left(view.left(), y);
        const QPointF right(view.right(), y);
        drawLine(left, right, major, Orientation::Horizontal, alpha);
    }
}
