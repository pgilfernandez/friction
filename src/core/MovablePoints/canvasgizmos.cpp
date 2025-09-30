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

#include "canvasgizmos.h"

#include "../canvas.h"
#include "pathpivot.h"
#include "../skia/skiaincludes.h"
#include "../skia/skqtconversions.h"
#include "../themesupport.h"

#include <QtMath>
#include <cmath>

// Define the constants
const qreal kRotateGizmoSweepDeg = 90.0;
const qreal kRotateGizmoBaseOffsetDeg = 270.0;
const qreal kRotateGizmoRadiusPx = 40.0;
const qreal kRotateGizmoStrokePx = 6.0;
const qreal kRotateGizmoHitWidthPx = kRotateGizmoStrokePx;
const qreal kAxisGizmoWidthPx = 5.0;
const qreal kAxisGizmoHeightPx = 60.0;
const qreal kAxisGizmoYOffsetPx = 40.0;
const qreal kAxisGizmoXOffsetPx = 40.0;
const qreal kScaleGizmoSizePx = 10.0;
const qreal kScaleGizmoGapPx = 4.0;
const qreal kShearGizmoRadiusPx = 6.0;
const qreal kShearGizmoGapPx = 4.0;



void drawCanvasGizmos(Canvas& canvas,
                       SkCanvas* const surface,
                       float invZoom,
                       qreal qInvZoom,
                       bool useLocalOrigin)
{
    canvas.updateRotateHandleGeometry(qInvZoom);

    if (!canvas.mRotateHandleVisible) { return; }
    if (!canvas.mRotPivot) { return; }

    const QPointF pivot = canvas.mRotPivot->getAbsolutePos();
    auto toLocal = [&](const QPointF &world) -> QPointF {
        return QPointF(world.x() - pivot.x(), world.y() - pivot.y());
    };
    auto toSkLocal = [&](const QPointF &world) -> SkPoint {
        const QPointF local = toLocal(world);
        return SkPoint::Make(toSkScalar(local.x()), toSkScalar(local.y()));
    };

    const bool needRestore = !useLocalOrigin;
    if (needRestore) {
        surface->save();
        surface->translate(toSkScalar(pivot.x()), toSkScalar(pivot.y()));
    }

    if (canvas.mShowRotateGizmo) {
        const QPointF centerLocal = toLocal(canvas.mRotateHandleAnchor);
        const qreal radius = canvas.mRotateHandleRadius;
        const qreal strokeWorld = kRotateGizmoStrokePx * qInvZoom;

        const SkRect arcRect = SkRect::MakeLTRB(toSkScalar(centerLocal.x() - radius),
                                                toSkScalar(centerLocal.y() - radius),
                                                toSkScalar(centerLocal.x() + radius),
                                                toSkScalar(centerLocal.y() + radius));

        qreal startAngle = std::fmod(canvas.mRotateHandleStartOffsetDeg + canvas.mRotateHandleAngleDeg, 360.0);
        if (startAngle < 0) { startAngle += 360.0; }
        const float startAngleF = static_cast<float>(startAngle);
        const float sweepAngleF = static_cast<float>(canvas.mRotateHandleSweepDeg);

        SkPaint arcPaint;
        arcPaint.setAntiAlias(true);
        arcPaint.setStyle(SkPaint::kStroke_Style);
        arcPaint.setStrokeCap(SkPaint::kButt_Cap);
        arcPaint.setStrokeWidth(toSkScalar(strokeWorld));
        const SkColor arcColor = ThemeSupport::getThemeHighlightSkColor(canvas.mRotateHandleHovered ? 255 : 190);
        arcPaint.setColor(arcColor);
        surface->drawArc(arcRect, startAngleF, sweepAngleF, false, arcPaint);
    }

    using AxisConstraint = Canvas::AxisConstraint;
    using AxisGizmoGeometry = Canvas::AxisGizmoGeometry;
    using ScaleHandle = Canvas::ScaleHandle;
    using ScaleGizmoGeometry = Canvas::ScaleGizmoGeometry;
    using ShearHandle = Canvas::ShearHandle;
    using ShearGizmoGeometry = Canvas::ShearGizmoGeometry;

    auto drawAxisRect = [&](AxisConstraint axis, const AxisGizmoGeometry &geom, const QColor &baseColor) {
        if (!geom.visible) { return; }
        const bool hovered = axis == AxisConstraint::X ? canvas.mAxisXHovered : canvas.mAxisYHovered;
        const bool active = (canvas.mAxisConstraint == axis);
        QColor color = baseColor;
        if (active) {
            color = color.lighter(135);
        } else if (hovered) {
            color = color.lighter(120);
        }

        SkPaint fillPaint;
        fillPaint.setAntiAlias(true);
        fillPaint.setStyle(SkPaint::kFill_Style);
        fillPaint.setColor(toSkColor(color));

        SkPaint borderPaint;
        borderPaint.setAntiAlias(true);
        borderPaint.setStyle(SkPaint::kStroke_Style);
        borderPaint.setStrokeWidth(toSkScalar(kRotateGizmoStrokePx * invZoom * 0.2f));
        borderPaint.setColor(toSkColor(color.darker(150)));

        SkPath path;
        if (geom.usePolygon && geom.polygonPoints.size() >= 3) {
            bool first = true;
            for (const QPointF &pt : geom.polygonPoints) {
                const SkPoint skPt = toSkLocal(pt);
                if (first) {
                    path.moveTo(skPt);
                    first = false;
                } else {
                    path.lineTo(skPt);
                }
            }
            path.close();
        } else {
            const qreal halfW = geom.size.width() * 0.5;
            const qreal halfH = geom.size.height() * 0.5;
            const qreal angleRadGeom = qDegreesToRadians(geom.angleDeg);
            const qreal cosG = std::cos(angleRadGeom);
            const qreal sinG = std::sin(angleRadGeom);
            const QPointF centerLocal = toLocal(geom.center);

            auto mapPoint = [&](qreal localX, qreal localY) -> SkPoint {
                const qreal rotatedX = localX * cosG - localY * sinG;
                const qreal rotatedY = localX * sinG + localY * cosG;
                return SkPoint::Make(toSkScalar(centerLocal.x() + rotatedX),
                                     toSkScalar(centerLocal.y() + rotatedY));
            };

            path.moveTo(mapPoint(-halfW, -halfH));
            path.lineTo(mapPoint(halfW, -halfH));
            path.lineTo(mapPoint(halfW, halfH));
            path.lineTo(mapPoint(-halfW, halfH));
            path.close();
        }

        surface->drawPath(path, fillPaint);
        surface->drawPath(path, borderPaint);
    };

    auto drawScaleSquare = [&](ScaleHandle handle, const ScaleGizmoGeometry &geom, const QColor &baseColor) {
        if (!geom.visible) { return; }
        bool hovered = false;
        switch (handle) {
        case ScaleHandle::X: hovered = canvas.mScaleXHovered; break;
        case ScaleHandle::Y: hovered = canvas.mScaleYHovered; break;
        case ScaleHandle::Uniform: hovered = canvas.mScaleUniformHovered; break;
        case ScaleHandle::None: default: hovered = false; break;
        }
        const bool active = (canvas.mScaleConstraint == handle);
        QColor color = baseColor;
        if (active) {
            color = color.lighter(135);
        } else if (hovered) {
            color = color.lighter(120);
        }

        SkPaint fillPaint;
        fillPaint.setAntiAlias(true);
        fillPaint.setStyle(SkPaint::kFill_Style);
        fillPaint.setColor(toSkColor(color));

        SkPaint borderPaint;
        borderPaint.setAntiAlias(true);
        borderPaint.setStyle(SkPaint::kStroke_Style);
        borderPaint.setStrokeWidth(toSkScalar(kRotateGizmoStrokePx * invZoom * 0.2f));
        borderPaint.setColor(toSkColor(color.darker(150)));

        if (geom.usePolygon && geom.polygonPoints.size() >= 3) {
            SkPath path;
            bool first = true;
            for (const QPointF &pt : geom.polygonPoints) {
                const SkPoint skPt = toSkLocal(pt);
                if (first) {
                    path.moveTo(skPt);
                    first = false;
                } else {
                    path.lineTo(skPt);
                }
            }
            path.close();
            surface->drawPath(path, fillPaint);
            surface->drawPath(path, borderPaint);
        } else {
            const QPointF centerLocal = toLocal(geom.center);
            const SkRect skRect = SkRect::MakeLTRB(toSkScalar(centerLocal.x() - geom.halfExtent),
                                                   toSkScalar(centerLocal.y() - geom.halfExtent),
                                                   toSkScalar(centerLocal.x() + geom.halfExtent),
                                                   toSkScalar(centerLocal.y() + geom.halfExtent));
            surface->drawRect(skRect, fillPaint);
            surface->drawRect(skRect, borderPaint);
        }
    };

    auto drawShearCircle = [&](ShearHandle handle, const ShearGizmoGeometry &geom, const QColor &baseColor) {
        if (!geom.visible) { return; }
        bool hovered = (handle == ShearHandle::X) ? canvas.mShearXHovered : canvas.mShearYHovered;
        const bool active = (canvas.mShearConstraint == handle);
        QColor color = baseColor;
        if (active) {
            color = color.lighter(135);
        } else if (hovered) {
            color = color.lighter(120);
        }

        SkPaint fillPaint;
        fillPaint.setAntiAlias(true);
        fillPaint.setStyle(SkPaint::kFill_Style);
        fillPaint.setColor(toSkColor(color));

        SkPaint borderPaint;
        borderPaint.setAntiAlias(true);
        borderPaint.setStyle(SkPaint::kStroke_Style);
        borderPaint.setStrokeWidth(toSkScalar(kRotateGizmoStrokePx * invZoom * 0.2f));
        borderPaint.setColor(toSkColor(color.darker(150)));

        if (geom.usePolygon && geom.polygonPoints.size() >= 3) {
            SkPath path;
            bool first = true;
            for (const QPointF &pt : geom.polygonPoints) {
                const SkPoint skPt = toSkLocal(pt);
                if (first) {
                    path.moveTo(skPt);
                    first = false;
                } else {
                    path.lineTo(skPt);
                }
            }
            path.close();
            surface->drawPath(path, fillPaint);
            surface->drawPath(path, borderPaint);
        } else {
            const QPointF centerLocal = toLocal(geom.center);
            const SkRect skRect = SkRect::MakeLTRB(toSkScalar(centerLocal.x() - geom.radius),
                                                   toSkScalar(centerLocal.y() - geom.radius),
                                                   toSkScalar(centerLocal.x() + geom.radius),
                                                   toSkScalar(centerLocal.y() + geom.radius));
            surface->drawOval(skRect, fillPaint);
            surface->drawOval(skRect, borderPaint);
        }
    };

    drawAxisRect(AxisConstraint::Y, canvas.mAxisYGeom, ThemeSupport::getThemeColorGreen(190));
    drawAxisRect(AxisConstraint::X, canvas.mAxisXGeom, ThemeSupport::getThemeColorRed(190));
    drawScaleSquare(ScaleHandle::Y, canvas.mScaleYGeom, ThemeSupport::getThemeColorGreen(190));
    drawScaleSquare(ScaleHandle::X, canvas.mScaleXGeom, ThemeSupport::getThemeColorRed(190));
    drawScaleSquare(ScaleHandle::Uniform, canvas.mScaleUniformGeom, ThemeSupport::getThemeColorYellow(190));
    drawShearCircle(ShearHandle::Y, canvas.mShearYGeom, ThemeSupport::getThemeColorGreen(190));
    drawShearCircle(ShearHandle::X, canvas.mShearXGeom, ThemeSupport::getThemeColorRed(190));

    if (needRestore) { surface->restore(); }
}

