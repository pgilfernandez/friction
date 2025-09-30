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
#include "../skia/skiaincludes.h"
#include "../skia/skqtconversions.h"
#include "../themesupport.h"

#include <QtMath>
#include <cmath>



void drawCanvasGizmos(Canvas& canvas,
                       SkCanvas* const surface,
                       float invZoom,
                       qreal qInvZoom)
{
    canvas.updateRotateHandleGeometry(qInvZoom);

    if (!canvas.mRotateHandleVisible) { return; }
    if (canvas.mGizmosDrawnThisFrame) { return; }
    canvas.mGizmosDrawnThisFrame = true;

    if (canvas.mShowRotateGizmo) {
        const QPointF center = canvas.mRotateHandleAnchor;
        const qreal radius = canvas.mRotateHandleRadius;
        const qreal strokeWorld = kRotateGizmoStrokePx * qInvZoom;

        const SkRect arcRect = SkRect::MakeLTRB(toSkScalar(center.x() - radius),
                                                toSkScalar(center.y() - radius),
                                                toSkScalar(center.x() + radius),
                                                toSkScalar(center.y() + radius));

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
                const SkPoint skPt = SkPoint::Make(toSkScalar(pt.x()), toSkScalar(pt.y()));
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
            auto mapPoint = [&](qreal localX, qreal localY) -> SkPoint {
                const qreal worldX = geom.center.x() + localX * cosG - localY * sinG;
                const qreal worldY = geom.center.y() + localX * sinG + localY * cosG;
                return SkPoint::Make(toSkScalar(worldX), toSkScalar(worldY));
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
                const SkPoint skPt = SkPoint::Make(toSkScalar(pt.x()), toSkScalar(pt.y()));
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
            const SkRect skRect = SkRect::MakeLTRB(toSkScalar(geom.center.x() - geom.halfExtent),
                                                   toSkScalar(geom.center.y() - geom.halfExtent),
                                                   toSkScalar(geom.center.x() + geom.halfExtent),
                                                   toSkScalar(geom.center.y() + geom.halfExtent));
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
                const SkPoint skPt = SkPoint::Make(toSkScalar(pt.x()), toSkScalar(pt.y()));
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
            const SkRect skRect = SkRect::MakeLTRB(toSkScalar(geom.center.x() - geom.radius),
                                                   toSkScalar(geom.center.y() - geom.radius),
                                                   toSkScalar(geom.center.x() + geom.radius),
                                                   toSkScalar(geom.center.y() + geom.radius));
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
}
