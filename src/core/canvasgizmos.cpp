/*
#
# Friction - https://friction.graphics
#
# Copyright (c) Ole-André Rodlie and contributors
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, version 3.
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

#include "canvas.h"

#include "MovablePoints/pathpivot.h"
#include "eevent.h"

using namespace Friction::Core;

bool Canvas::showRotateGizmo() const
{
    return mGizmos.fState.mShowRotateGizmo;
}

void Canvas::setShowRotateGizmo(bool enabled)
{
    if (mGizmos.fState.mShowRotateGizmo == enabled) { return; }
    mGizmos.fState.mShowRotateGizmo = enabled;
    if (!enabled) {
        setRotateHandleHover(false);
        mGizmos.fState.mRotatingFromHandle = false;
        setGizmosSuppressed(false);
    }
    emit requestUpdate();
}

bool Canvas::showPositionGizmo() const
{
    return mGizmos.fState.mShowPositionGizmo;
}

void Canvas::setShowPositionGizmo(bool enabled)
{
    if (mGizmos.fState.mShowPositionGizmo == enabled) { return; }
    mGizmos.fState.mShowPositionGizmo = enabled;
    if (!enabled) {
        mGizmos.fState.mAxisHandleActive = false;
        mGizmos.fState.mAxisConstraint = Gizmos::AxisConstraint::None;
        setAxisGizmoHover(Gizmos::AxisConstraint::X, false);
        setAxisGizmoHover(Gizmos::AxisConstraint::Y, false);
        setAxisGizmoHover(Gizmos::AxisConstraint::Uniform, false);
        setGizmosSuppressed(false);
    }
    emit requestUpdate();
}

bool Canvas::showScaleGizmo() const
{
    return mGizmos.fState.mShowScaleGizmo;
}

void Canvas::setShowScaleGizmo(bool enabled)
{
    if (mGizmos.fState.mShowScaleGizmo == enabled) { return; }
    mGizmos.fState.mShowScaleGizmo = enabled;
    if (!enabled) {
        mGizmos.fState.mScaleHandleActive = false;
        mGizmos.fState.mScaleConstraint = Gizmos::ScaleHandle::None;
        setScaleGizmoHover(Gizmos::ScaleHandle::X, false);
        setScaleGizmoHover(Gizmos::ScaleHandle::Y, false);
        setScaleGizmoHover(Gizmos::ScaleHandle::Uniform, false);
        setGizmosSuppressed(false);
    }
    emit requestUpdate();
}

bool Canvas::showShearGizmo() const
{
    return mGizmos.fState.mShowShearGizmo;
}

void Canvas::setShowShearGizmo(bool enabled)
{
    if (mGizmos.fState.mShowShearGizmo == enabled) { return; }
    mGizmos.fState.mShowShearGizmo = enabled;
    if (!enabled) {
        mGizmos.fState.mShearHandleActive = false;
        mGizmos.fState.mShearConstraint = Gizmos::ShearHandle::None;
        setShearGizmoHover(Gizmos::ShearHandle::X, false);
        setShearGizmoHover(Gizmos::ShearHandle::Y, false);
        setGizmosSuppressed(false);
    }
    emit requestUpdate();
}

bool Canvas::prepareRotation(const QPointF &startPos,
                             bool fromHandle)
{
    if (mCurrentMode != CanvasMode::boxTransform &&
        mCurrentMode != CanvasMode::pointTransform) { return false; }
    if (mSelectedBoxes.isEmpty()) { return false; }
    if (mCurrentMode == CanvasMode::pointTransform) {
        if (mSelectedPoints_d.isEmpty()) { return false; }
    }

    mGizmos.fState.mRotatingFromHandle = fromHandle;
    mValueInput.clearAndDisableInput();
    mValueInput.setupRotate();

    if (fromHandle) { setGizmosSuppressed(true); }

    mRotPivot->setMousePos(startPos);
    mTransMode = TransformMode::rotate;
    mRotHalfCycles = 0;
    mLastDRot = 0;

    mDoubleClick = false;
    mStartTransform = true;
    return true;
}

void Canvas::updateRotateHandleHover(const QPointF &pos,
                                     qreal invScale)
{
    updateRotateHandleGeometry(invScale);
    setRotateHandleHover(pointOnRotateGizmo(pos, invScale));
    setAxisGizmoHover(Gizmos::AxisConstraint::X,
                      pointOnAxisGizmo(Gizmos::AxisConstraint::X,
                                       pos, invScale));
    setAxisGizmoHover(Gizmos::AxisConstraint::Y,
                      pointOnAxisGizmo(Gizmos::AxisConstraint::Y,
                                       pos, invScale));
    setAxisGizmoHover(Gizmos::AxisConstraint::Uniform,
                      pointOnAxisGizmo(Gizmos::AxisConstraint::Uniform,
                                       pos, invScale));
    setScaleGizmoHover(Gizmos::ScaleHandle::X,
                       pointOnScaleGizmo(Gizmos::ScaleHandle::X,
                                         pos, invScale));
    setScaleGizmoHover(Gizmos::ScaleHandle::Y,
                       pointOnScaleGizmo(Gizmos::ScaleHandle::Y,
                                         pos, invScale));
    setScaleGizmoHover(Gizmos::ScaleHandle::Uniform,
                       pointOnScaleGizmo(Gizmos::ScaleHandle::Uniform,
                                         pos, invScale));
    setShearGizmoHover(Gizmos::ShearHandle::X,
                       pointOnShearGizmo(Gizmos::ShearHandle::X,
                                         pos, invScale));
    setShearGizmoHover(Gizmos::ShearHandle::Y,
                       pointOnShearGizmo(Gizmos::ShearHandle::Y,
                                         pos, invScale));
}

bool Canvas::pointOnRotateGizmo(const QPointF &pos,
                                qreal invScale) const
{
    if (!mGizmos.fState.mRotateHandleVisible || !mGizmos.fState.mShowRotateGizmo) { return false; }
    if (mGizmos.fState.mRotateHandleHitPolygon.size() >= 3) {
        QPolygonF hitPoly(mGizmos.fState.mRotateHandleHitPolygon);
        if (hitPoly.containsPoint(pos, Qt::OddEvenFill)) { return true; }
    }
    const qreal halfThicknessWorld = (mGizmos.fConfig.kRotateGizmoHitWidthPx * invScale) * 0.5;
    const QPointF center = mGizmos.fState.mRotateHandleAnchor;
    const qreal radius = mGizmos.fState.mRotateHandleRadius;
    if (radius <= 0) { return false; }

    const qreal dx = pos.x() - center.x();
    const qreal dy = pos.y() - center.y();
    const qreal distance = std::hypot(dx, dy);
    if (distance < radius - halfThicknessWorld || distance > radius + halfThicknessWorld) {
        return false;
    }

    const double angleCCW = qRadiansToDegrees(std::atan2(center.y() - pos.y(), pos.x() - center.x()));
    const double skAngle = std::fmod(360.0 + (360.0 - angleCCW), 360.0);
    const double arcStart = std::fmod(mGizmos.fState.mRotateHandleStartOffsetDeg + mGizmos.fState.mRotateHandleAngleDeg, 360.0);
    const double normalizedStart = arcStart < 0 ? arcStart + 360.0 : arcStart;
    const double delta = std::fmod((skAngle - normalizedStart + 360.0), 360.0);

    double extraAngleDeg = 0.0;
    if (radius > 0) {
        const qreal halfStrokeWorld = (mGizmos.fConfig.kRotateGizmoStrokePx * invScale) * 0.5;
        extraAngleDeg = qRadiansToDegrees(halfStrokeWorld / radius);
    }

    if (delta <= mGizmos.fState.mRotateHandleSweepDeg + extraAngleDeg) { return true; }
    if (extraAngleDeg > 0.0 && delta >= 360.0 - extraAngleDeg) { return true; }
    return false;
}

void Canvas::setRotateHandleHover(bool hovered)
{
    if (mGizmos.fState.mRotateHandleHovered == hovered) { return; }
    mGizmos.fState.mRotateHandleHovered = hovered;
    emit requestUpdate();
}

void Canvas::setGizmosSuppressed(bool suppressed)
{
    if (mGizmos.fState.mGizmosSuppressed == suppressed) { return; }
    mGizmos.fState.mGizmosSuppressed = suppressed;
    if (suppressed) {
        mGizmos.fState.mRotateHandleHovered = false;
        mGizmos.fState.mAxisXHovered = false;
        mGizmos.fState.mAxisYHovered = false;
        mGizmos.fState.mAxisUniformHovered = false;
        mGizmos.fState.mScaleXHovered = false;
        mGizmos.fState.mScaleYHovered = false;
        mGizmos.fState.mScaleUniformHovered = false;
        mGizmos.fState.mShearXHovered = false;
        mGizmos.fState.mShearYHovered = false;
    }
    emit requestUpdate();
}

bool Canvas::startRotatingAction(const eKeyEvent &e)
{
    if (!prepareRotation(e.fPos)) { return false; }
    e.fGrabMouse();
    return true;
}

void Canvas::updateRotateHandleGeometry(qreal invScale)
{
    mGizmos.fState.mRotateHandleVisible = false;
    mGizmos.fState.mRotateHandleRadius = 0;
    mGizmos.fState.mRotateHandlePolygon.clear();
    mGizmos.fState.mRotateHandleHitPolygon.clear();

    auto resetAllGizmos = [&]() {
        setAxisGizmoHover(Gizmos::AxisConstraint::X, false);
        setAxisGizmoHover(Gizmos::AxisConstraint::Y, false);
        setAxisGizmoHover(Gizmos::AxisConstraint::Uniform, false);
        setScaleGizmoHover(Gizmos::ScaleHandle::X, false);
        setScaleGizmoHover(Gizmos::ScaleHandle::Y, false);
        setScaleGizmoHover(Gizmos::ScaleHandle::Uniform, false);
        setShearGizmoHover(Gizmos::ShearHandle::X, false);
        setShearGizmoHover(Gizmos::ShearHandle::Y, false);

        mGizmos.fState.mAxisXGeom = Gizmos::AxisGeometry();
        mGizmos.fState.mAxisYGeom = Gizmos::AxisGeometry();
        mGizmos.fState.mAxisUniformGeom = Gizmos::AxisGeometry();
        mGizmos.fState.mScaleXGeom = Gizmos::ScaleGeometry();
        mGizmos.fState.mScaleYGeom = Gizmos::ScaleGeometry();
        mGizmos.fState.mScaleUniformGeom = Gizmos::ScaleGeometry();
        mGizmos.fState.mShearXGeom = Gizmos::ShearGeometry();
        mGizmos.fState.mShearYGeom = Gizmos::ShearGeometry();

        mGizmos.fState.mAxisConstraint = Gizmos::AxisConstraint::None;
        mGizmos.fState.mScaleConstraint = Gizmos::ScaleHandle::None;
        mGizmos.fState.mShearConstraint = Gizmos::ShearHandle::None;
        mGizmos.fState.mAxisHandleActive = false;
        mGizmos.fState.mScaleHandleActive = false;
        mGizmos.fState.mShearHandleActive = false;
        mValueInput.setForce1D(false);
        mValueInput.setXYMode();
        mGizmos.fState.mRotateHandlePolygon.clear();
        mGizmos.fState.mRotateHandleHitPolygon.clear();
    };

    if (mGizmos.fState.mGizmosSuppressed) { return; }

    if (mCurrentMode != CanvasMode::boxTransform) {
        setRotateHandleHover(false);
        resetAllGizmos();
        return;
    }

    if (mSelectedBoxes.isEmpty()) {
        setRotateHandleHover(false);
        resetAllGizmos();
        return;
    }

    if (!mRotPivot) {
        setRotateHandleHover(false);
        resetAllGizmos();
        return;
    }

    mGizmos.fState.mRotateHandleAngleDeg = 0.0; // keep gizmo orientation screen-aligned

    QPointF pivot;
    if (pivotPosForGizmosValid) { pivot = pivotPosForGizmos; }
    else { pivot = mRotPivot->getAbsolutePos(); }

    const qreal axisWidthWorld = mGizmos.fConfig.kAxisGizmoWidthPx * invScale;
    const qreal axisHeightWorld = mGizmos.fConfig.kAxisGizmoHeightPx * invScale;
    const qreal axisGapYWorld = mGizmos.fConfig.kAxisGizmoYOffsetPx * invScale;
    const qreal axisGapXWorld = mGizmos.fConfig.kAxisGizmoXOffsetPx * invScale;

    // const qreal anchorOffset = axisWidthWorld * 0.5;
    const qreal anchorOffset = 2.0 * invScale;
    mGizmos.fState.mRotateHandleAnchor = pivot + QPointF(anchorOffset, -anchorOffset);

    const qreal baseRotateRadiusWorld = mGizmos.fConfig.kRotateGizmoRadiusPx * invScale;
    mGizmos.fState.mRotateHandleRadius = baseRotateRadiusWorld;
    mGizmos.fState.mRotateHandleSweepDeg = mGizmos.fConfig.kRotateGizmoSweepDeg; // default sweep angle
    mGizmos.fState.mRotateHandleStartOffsetDeg = mGizmos.fConfig.kRotateGizmoBaseOffsetDeg; // default base angle offset

    const qreal strokeWorld = mGizmos.fConfig.kRotateGizmoStrokePx * invScale;
    const qreal sweepDegAbs = std::abs(mGizmos.fState.mRotateHandleSweepDeg);
    auto normalizeAngle = [](qreal degrees) {
        degrees = std::fmod(degrees, 360.0);
        if (degrees < 0.0) { degrees += 360.0; }
        return degrees;
    };
    const qreal startAngleSk = normalizeAngle(mGizmos.fState.mRotateHandleStartOffsetDeg + mGizmos.fState.mRotateHandleAngleDeg);
    const qreal direction = (mGizmos.fState.mRotateHandleSweepDeg >= 0.0) ? 1.0 : -1.0;
    mGizmos.fState.mRotateHandlePolygon.clear();
    mGizmos.fState.mRotateHandleHitPolygon.clear();

    if (mGizmos.fState.mShowRotateGizmo && sweepDegAbs > std::numeric_limits<qreal>::epsilon()) {
        const int segments = std::max(8, static_cast<int>(std::ceil(sweepDegAbs / 6.0)));
        auto angleToPoint = [&](qreal angleDeg, qreal radius) {
            const qreal angleRad = qDegreesToRadians(angleDeg);
            return QPointF(mGizmos.fState.mRotateHandleAnchor.x() + radius * std::cos(angleRad),
                           mGizmos.fState.mRotateHandleAnchor.y() + radius * std::sin(angleRad));
        };
        auto buildArcPolygon = [&](qreal halfThickness, QVector<QPointF> &storage) {
            storage.clear();
            const qreal outerRadius = mGizmos.fState.mRotateHandleRadius + halfThickness;
            if (outerRadius <= 0.0) { return; }
            const qreal innerRadius = std::max(mGizmos.fState.mRotateHandleRadius - halfThickness, 0.0);
            storage.reserve((segments + 1) * 2);
            for (int i = 0; i <= segments; ++i) {
                const qreal angle = normalizeAngle(startAngleSk + direction * (sweepDegAbs * i) / segments);
                storage.append(angleToPoint(angle, outerRadius));
            }
            if (innerRadius > std::numeric_limits<qreal>::epsilon()) {
                for (int i = 0; i <= segments; ++i) {
                    const qreal angle = normalizeAngle(startAngleSk + direction * sweepDegAbs - direction * (sweepDegAbs * i) / segments);
                    storage.append(angleToPoint(angle, innerRadius));
                }
            } else {
                storage.append(mGizmos.fState.mRotateHandleAnchor);
            }
        };

        buildArcPolygon(strokeWorld * 0.5, mGizmos.fState.mRotateHandlePolygon);
        const qreal hitHalfThickness = (mGizmos.fConfig.kRotateGizmoHitWidthPx * invScale) * 0.5;
        buildArcPolygon(hitHalfThickness, mGizmos.fState.mRotateHandleHitPolygon);
    }

    mGizmos.fState.mAxisYGeom.center = pivot + QPointF(0.0, -axisGapYWorld);
    mGizmos.fState.mAxisYGeom.size = QSizeF(axisWidthWorld, axisHeightWorld);
    mGizmos.fState.mAxisYGeom.angleDeg = 0.0;
    mGizmos.fState.mAxisYGeom.visible = mGizmos.fState.mShowPositionGizmo;
    mGizmos.fState.mAxisYGeom.usePolygon = true;
    mGizmos.fState.mAxisYGeom.polygonPoints = {
        pivot + QPointF(0.0, - 10.0 * invScale),
        pivot + QPointF(-2.0 * invScale, - 11.0 * invScale),
        pivot + QPointF(-2.0 * invScale, - 55.0 * invScale),
        pivot + QPointF(-6.0 * invScale, - 57.0 * invScale),
        pivot + QPointF(0.0, - 70.0 * invScale),
        pivot + QPointF(6.0 * invScale, - 57.0 * invScale),
        pivot + QPointF(2.0 * invScale, - 55.0 * invScale),
        pivot + QPointF(2.0 * invScale, - 11.0 * invScale)
    };

    mGizmos.fState.mAxisXGeom.center = pivot + QPointF(axisGapXWorld, 0.0);
    mGizmos.fState.mAxisXGeom.size = QSizeF(axisHeightWorld, axisWidthWorld);
    mGizmos.fState.mAxisXGeom.angleDeg = 0.0;
    mGizmos.fState.mAxisXGeom.visible = mGizmos.fState.mShowPositionGizmo;
    mGizmos.fState.mAxisXGeom.usePolygon = true;
    mGizmos.fState.mAxisXGeom.polygonPoints = {
        pivot + QPointF(10.0 * invScale, 0.0),
        pivot + QPointF(11.0 * invScale, -2.0 * invScale),
        pivot + QPointF(55.0 * invScale, -2.0 * invScale),
        pivot + QPointF(57.0 * invScale, -6.0 * invScale),
        pivot + QPointF(70.0 * invScale, 0.0),
        pivot + QPointF(57.0 * invScale, 6.0 * invScale),
        pivot + QPointF(55.0 * invScale, 2.0 * invScale),
        pivot + QPointF(11.0 * invScale, 2.0 * invScale)
    };

    mGizmos.fState.mAxisUniformGeom.center = pivot + QPointF(axisGapXWorld, 0.0);
    mGizmos.fState.mAxisUniformGeom.size = QSizeF(axisHeightWorld, axisWidthWorld);
    mGizmos.fState.mAxisUniformGeom.visible = mGizmos.fState.mShowPositionGizmo;
    mGizmos.fState.mAxisUniformGeom.usePolygon = true;
    mGizmos.fState.mAxisUniformGeom.polygonPoints = {
        pivot + QPointF((mGizmos.fConfig.kAxisGizmoUniformOffsetPx + mGizmos.fConfig.kAxisGizmoUniformChamferPx) * invScale , - mGizmos.fConfig.kAxisGizmoUniformOffsetPx * invScale),
        pivot + QPointF(mGizmos.fConfig.kAxisGizmoUniformOffsetPx * invScale, - (mGizmos.fConfig.kAxisGizmoUniformOffsetPx + mGizmos.fConfig.kAxisGizmoUniformChamferPx) * invScale),
        pivot + QPointF(mGizmos.fConfig.kAxisGizmoUniformOffsetPx * invScale, - (mGizmos.fConfig.kAxisGizmoUniformWidthPx - mGizmos.fConfig.kAxisGizmoUniformChamferPx) * invScale),
        pivot + QPointF((mGizmos.fConfig.kAxisGizmoUniformOffsetPx + mGizmos.fConfig.kAxisGizmoUniformChamferPx) * invScale, - mGizmos.fConfig.kAxisGizmoUniformWidthPx * invScale),
        pivot + QPointF((mGizmos.fConfig.kAxisGizmoUniformWidthPx - mGizmos.fConfig.kAxisGizmoUniformChamferPx) * invScale, - mGizmos.fConfig.kAxisGizmoUniformWidthPx * invScale),
        pivot + QPointF(mGizmos.fConfig.kAxisGizmoUniformWidthPx * invScale, - (mGizmos.fConfig.kAxisGizmoUniformWidthPx - mGizmos.fConfig.kAxisGizmoUniformChamferPx) * invScale),
        pivot + QPointF(mGizmos.fConfig.kAxisGizmoUniformWidthPx * invScale, - (mGizmos.fConfig.kAxisGizmoUniformOffsetPx + mGizmos.fConfig.kAxisGizmoUniformChamferPx) * invScale),
        pivot + QPointF((mGizmos.fConfig.kAxisGizmoUniformWidthPx - mGizmos.fConfig.kAxisGizmoUniformChamferPx) * invScale, - mGizmos.fConfig.kAxisGizmoUniformOffsetPx * invScale)
    };

    const qreal rotateOffsetWorld = mGizmos.fState.mAxisYGeom.size.width() * 0.5;
    mGizmos.fState.mRotateHandleRadius = baseRotateRadiusWorld + rotateOffsetWorld;

    const QPointF offset(0.0, -mGizmos.fState.mRotateHandleRadius);
    mGizmos.fState.mRotateHandlePos = pivot + offset;

    const qreal scaleSizeWorld = mGizmos.fConfig.kScaleGizmoSizePx * invScale;
    const qreal scaleHalf = scaleSizeWorld * 0.5;
    const qreal scaleGapWorld = mGizmos.fConfig.kScaleGizmoGapPx * invScale;
    const qreal axisYTop = mGizmos.fState.mAxisYGeom.center.y() - (mGizmos.fState.mAxisYGeom.size.height() * 0.5);
    const qreal axisXRight = mGizmos.fState.mAxisXGeom.center.x() + (mGizmos.fState.mAxisXGeom.size.width() * 0.5);
    const qreal scaleCenterY = axisYTop - scaleGapWorld - scaleHalf;
    const qreal scaleCenterX = axisXRight + scaleGapWorld + scaleHalf;

    mGizmos.fState.mScaleYGeom.center = QPointF(mGizmos.fState.mAxisYGeom.center.x(), scaleCenterY);
    mGizmos.fState.mScaleYGeom.halfExtent = scaleHalf;
    mGizmos.fState.mScaleYGeom.visible = mGizmos.fState.mShowScaleGizmo;
    mGizmos.fState.mScaleYGeom.usePolygon = false;
    mGizmos.fState.mScaleYGeom.polygonPoints.clear();

    mGizmos.fState.mScaleXGeom.center = QPointF(scaleCenterX, mGizmos.fState.mAxisXGeom.center.y());
    mGizmos.fState.mScaleXGeom.halfExtent = scaleHalf;
    mGizmos.fState.mScaleXGeom.visible = mGizmos.fState.mShowScaleGizmo;
    mGizmos.fState.mScaleXGeom.usePolygon = false;
    mGizmos.fState.mScaleXGeom.polygonPoints.clear();

    mGizmos.fState.mScaleUniformGeom.center = QPointF(scaleCenterX, scaleCenterY);
    mGizmos.fState.mScaleUniformGeom.halfExtent = scaleHalf;
    mGizmos.fState.mScaleUniformGeom.visible = mGizmos.fState.mShowScaleGizmo;
    mGizmos.fState.mScaleUniformGeom.usePolygon = true;
    mGizmos.fState.mScaleUniformGeom.polygonPoints = {
        QPointF(mGizmos.fState.mScaleUniformGeom.center.x() - scaleHalf * 2, mGizmos.fState.mScaleUniformGeom.center.y() - scaleHalf),
        QPointF(mGizmos.fState.mScaleUniformGeom.center.x() + scaleHalf, mGizmos.fState.mScaleUniformGeom.center.y() - scaleHalf),
        QPointF(mGizmos.fState.mScaleUniformGeom.center.x() + scaleHalf, mGizmos.fState.mScaleUniformGeom.center.y() + scaleHalf * 2),
        QPointF(mGizmos.fState.mScaleUniformGeom.center.x() - scaleHalf, mGizmos.fState.mScaleUniformGeom.center.y() + scaleHalf * 2),
        QPointF(mGizmos.fState.mScaleUniformGeom.center.x() - scaleHalf, mGizmos.fState.mScaleUniformGeom.center.y() + scaleHalf),
        QPointF(mGizmos.fState.mScaleUniformGeom.center.x() - scaleHalf * 2, mGizmos.fState.mScaleUniformGeom.center.y() + scaleHalf)
    };

    const qreal shearRadiusWorld = mGizmos.fConfig.kShearGizmoRadiusPx * invScale;

    const QPointF scaleYCenter = mGizmos.fState.mScaleYGeom.center;
    const QPointF scaleXCenter = mGizmos.fState.mScaleXGeom.center;
    const QPointF scaleUniformCenter = mGizmos.fState.mScaleUniformGeom.center;

    mGizmos.fState.mShearXGeom.center = QPointF((scaleYCenter.x() + scaleUniformCenter.x()) * 0.5,
                                                scaleCenterY);
    mGizmos.fState.mShearXGeom.radius = shearRadiusWorld;
    mGizmos.fState.mShearXGeom.visible = mGizmos.fState.mShowShearGizmo;
    mGizmos.fState.mShearXGeom.usePolygon = true;
    {
        const qreal halfWidth = shearRadiusWorld * 1.5;
        const qreal topHeight = shearRadiusWorld * 0.8;
        //const qreal bottomHeight = shearRadiusWorld * 0.6;
        const QPointF c = mGizmos.fState.mShearXGeom.center;
        mGizmos.fState.mShearXGeom.polygonPoints = {
            QPointF(c.x() - scaleHalf * 0.5 - halfWidth * 1.5, c.y()),
            QPointF(c.x() - scaleHalf * 0.5 - halfWidth, c.y() - topHeight),
            QPointF(c.x() - scaleHalf * 0.5 + halfWidth, c.y() - topHeight),
            QPointF(c.x() - scaleHalf * 0.5 + halfWidth * 1.5, c.y()),
            QPointF(c.x() - scaleHalf * 0.5 + halfWidth, c.y() + topHeight),
            QPointF(c.x() - scaleHalf * 0.5 - halfWidth, c.y() + topHeight)
        };
    }

    mGizmos.fState.mShearYGeom.center = QPointF(scaleCenterX,
                                 (scaleXCenter.y() + scaleUniformCenter.y()) * 0.5);
    mGizmos.fState.mShearYGeom.radius = shearRadiusWorld;
    mGizmos.fState.mShearYGeom.visible = mGizmos.fState.mShowShearGizmo;
    mGizmos.fState.mShearYGeom.usePolygon = true;
    {
        const qreal halfHeight = shearRadiusWorld * 1.5;
        const qreal topWidth = shearRadiusWorld * 0.8;
        //const qreal bottomWidth = shearRadiusWorld * 0.6;
        const QPointF c = mGizmos.fState.mShearYGeom.center;
        mGizmos.fState.mShearYGeom.polygonPoints = {
            QPointF(c.x(), c.y() + scaleHalf * 0.5 - halfHeight * 1.5),
            QPointF(c.x() + topWidth, c.y() + scaleHalf * 0.5 - halfHeight),
            QPointF(c.x() + topWidth, c.y() + scaleHalf * 0.5 + halfHeight),
            QPointF(c.x(), c.y() + scaleHalf * 0.5 + halfHeight * 1.5),
            QPointF(c.x() - topWidth, c.y() + scaleHalf * 0.5 + halfHeight),
            QPointF(c.x() - topWidth, c.y() + scaleHalf * 0.5 - halfHeight)
        };
    }

    const bool anyGizmoVisible = (mGizmos.fState.mShowRotateGizmo || mGizmos.fState.mShowPositionGizmo ||
                                  mGizmos.fState.mShowScaleGizmo || mGizmos.fState.mShowShearGizmo);
    mGizmos.fState.mRotateHandleVisible = anyGizmoVisible;
}

bool Canvas::tryStartRotateWithGizmo(const eMouseEvent &e,
                                     qreal invScale)
{
    updateRotateHandleGeometry(invScale);
    const bool hovered = pointOnRotateGizmo(e.fPos, invScale);
    setRotateHandleHover(hovered);
    if (!hovered) { return false; }

    if (!prepareRotation(e.fPos, true)) {
        setRotateHandleHover(false);
        return false;
    }
    e.fGrabMouse();
    return true;
}

bool Canvas::tryStartScaleGizmo(const eMouseEvent &e,
                                qreal invScale)
{
    updateRotateHandleGeometry(invScale);
    if (!mGizmos.fState.mRotateHandleVisible) { return false; }

    if (pointOnScaleGizmo(Gizmos::ScaleHandle::Y, e.fPos, invScale)) {
        return startScaleConstrainedMove(e, Gizmos::ScaleHandle::Y);
    }
    if (pointOnScaleGizmo(Gizmos::ScaleHandle::X, e.fPos, invScale)) {
        return startScaleConstrainedMove(e, Gizmos::ScaleHandle::X);
    }
    if (pointOnScaleGizmo(Gizmos::ScaleHandle::Uniform, e.fPos, invScale)) {
        return startScaleConstrainedMove(e, Gizmos::ScaleHandle::Uniform);
    }
    return false;
}

bool Canvas::tryStartShearGizmo(const eMouseEvent &e,
                                qreal invScale)
{
    updateRotateHandleGeometry(invScale);
    if (!mGizmos.fState.mRotateHandleVisible) { return false; }

    if (pointOnShearGizmo(Gizmos::ShearHandle::Y, e.fPos, invScale)) {
        return startShearConstrainedMove(e, Gizmos::ShearHandle::Y);
    }
    if (pointOnShearGizmo(Gizmos::ShearHandle::X, e.fPos, invScale)) {
        return startShearConstrainedMove(e, Gizmos::ShearHandle::X);
    }
    return false;
}

bool Canvas::tryStartAxisGizmo(const eMouseEvent &e,
                               qreal invScale)
{
    updateRotateHandleGeometry(invScale);
    if (!mGizmos.fState.mRotateHandleVisible) { return false; }

    if (pointOnAxisGizmo(Gizmos::AxisConstraint::Uniform, e.fPos, invScale)) {
        return startAxisConstrainedMove(e, Gizmos::AxisConstraint::Uniform);
    }
    if (pointOnAxisGizmo(Gizmos::AxisConstraint::Y, e.fPos, invScale)) {
        return startAxisConstrainedMove(e, Gizmos::AxisConstraint::Y);
    }
    if (pointOnAxisGizmo(Gizmos::AxisConstraint::X, e.fPos, invScale)) {
        return startAxisConstrainedMove(e, Gizmos::AxisConstraint::X);
    }
    return false;
}

bool Canvas::startScaleConstrainedMove(const eMouseEvent &e,
                                       Gizmos::ScaleHandle handle)
{
    if (mCurrentMode != CanvasMode::boxTransform &&
        mCurrentMode != CanvasMode::pointTransform) { return false; }
    if (mSelectedBoxes.isEmpty() && mSelectedPoints_d.isEmpty()) { return false; }

    mValueInput.clearAndDisableInput();
    mValueInput.setupScale();

    if (handle == Gizmos::ScaleHandle::Uniform) {
        mValueInput.setForce1D(false);
        mValueInput.setXYMode();
    } else {
        mValueInput.setForce1D(true);
        if (handle == Gizmos::ScaleHandle::X) { mValueInput.setXOnlyMode(); }
        else { mValueInput.setYOnlyMode(); }
    }

    mTransMode = TransformMode::scale;
    mDoubleClick = false;
    mStartTransform = true;

    mGizmos.fState.mScaleConstraint = handle;
    mGizmos.fState.mScaleHandleActive = true;
    setScaleGizmoHover(handle, true);
    mRotPivot->setMousePos(e.fPos);
    setGizmosSuppressed(true);

    e.fGrabMouse();
    return true;
}

bool Canvas::startShearConstrainedMove(const eMouseEvent &e,
                                       Gizmos::ShearHandle handle)
{
    if (mCurrentMode != CanvasMode::boxTransform &&
        mCurrentMode != CanvasMode::pointTransform) { return false; }
    if (mSelectedBoxes.isEmpty() && mSelectedPoints_d.isEmpty()) { return false; }

    mValueInput.clearAndDisableInput();
    mValueInput.setupShear();
    mValueInput.setForce1D(true);
    if (handle == Gizmos::ShearHandle::X) { mValueInput.setXOnlyMode(); }
    else { mValueInput.setYOnlyMode(); }

    mTransMode = TransformMode::shear;
    mDoubleClick = false;
    mStartTransform = true;

    mGizmos.fState.mShearConstraint = handle;
    mGizmos.fState.mShearHandleActive = true;
    setShearGizmoHover(handle, true);
    mRotPivot->setMousePos(e.fPos);
    setGizmosSuppressed(true);

    e.fGrabMouse();
    return true;
}

bool Canvas::startAxisConstrainedMove(const eMouseEvent &e,
                                      Gizmos::AxisConstraint axis)
{
    if (mCurrentMode != CanvasMode::boxTransform &&
        mCurrentMode != CanvasMode::pointTransform) { return false; }
    if (mSelectedBoxes.isEmpty() && mSelectedPoints_d.isEmpty()) { return false; }

    mValueInput.clearAndDisableInput();
    mValueInput.setupMove();
    mValueInput.setForce1D(true);
    if (axis == Gizmos::AxisConstraint::X) {
        mValueInput.setXOnlyMode();
    } else {
        if (axis == Gizmos::AxisConstraint::Y) { mValueInput.setYOnlyMode(); }
        else { mValueInput.setXYMode(); }
    }

    mTransMode = TransformMode::move;
    mDoubleClick = false;
    mStartTransform = true;
    mGizmos.fState.mAxisConstraint = axis;
    mGizmos.fState.mAxisHandleActive = true;
    setAxisGizmoHover(axis, true);
    setGizmosSuppressed(true);
    e.fGrabMouse();
    return true;
}

bool Canvas::pointOnScaleGizmo(Gizmos::ScaleHandle handle,
                               const QPointF &pos,
                               qreal invScale) const
{
    Q_UNUSED(invScale);
    if (!mGizmos.fState.mRotateHandleVisible) { return false; }

    const Gizmos::ScaleGeometry *geom = nullptr;
    switch (handle) {
    case Gizmos::ScaleHandle::X:
        geom = &mGizmos.fState.mScaleXGeom;
        break;
    case Gizmos::ScaleHandle::Y:
        geom = &mGizmos.fState.mScaleYGeom;
        break;
    case Gizmos::ScaleHandle::Uniform:
        geom = &mGizmos.fState.mScaleUniformGeom;
        break;
    case Gizmos::ScaleHandle::None:
    default:
        return false;
    }

    if (!geom->visible || geom->halfExtent <= 0.0) { return false; }

    if (geom->usePolygon && geom->polygonPoints.size() >= 3) {
        QPolygonF poly(geom->polygonPoints);
        return poly.containsPoint(pos, Qt::OddEvenFill);
    }

    const qreal half = geom->halfExtent;
    return std::abs(pos.x() - geom->center.x()) <= half &&
           std::abs(pos.y() - geom->center.y()) <= half;
}

bool Canvas::pointOnShearGizmo(Gizmos::ShearHandle handle,
                               const QPointF &pos,
                               qreal invScale) const
{
    Q_UNUSED(invScale);
    if (!mGizmos.fState.mRotateHandleVisible) { return false; }

    const Gizmos::ShearGeometry *geom = nullptr;
    switch (handle) {
    case Gizmos::ShearHandle::X:
        geom = &mGizmos.fState.mShearXGeom;
        break;
    case Gizmos::ShearHandle::Y:
        geom = &mGizmos.fState.mShearYGeom;
        break;
    case Gizmos::ShearHandle::None:
    default:
        return false;
    }

    if (!geom->visible) { return false; }

    if (geom->usePolygon && geom->polygonPoints.size() >= 3) {
        QPolygonF poly(geom->polygonPoints);
        return poly.containsPoint(pos, Qt::OddEvenFill);
    }

    if (geom->radius <= 0.0) { return false; }

    const qreal distance = std::hypot(pos.x() - geom->center.x(),
                                      pos.y() - geom->center.y());
    return distance <= geom->radius;
}

bool Canvas::pointOnAxisGizmo(Gizmos::AxisConstraint axis,
                              const QPointF &pos,
                              qreal invScale) const
{
    Q_UNUSED(invScale);
    if (!mGizmos.fState.mRotateHandleVisible) { return false; }

    const Gizmos::AxisGeometry* geomPtr;
    switch (axis) {
    case Gizmos::AxisConstraint::X:
        geomPtr = &mGizmos.fState.mAxisXGeom;
        break;
    case Gizmos::AxisConstraint::Y:
        geomPtr = &mGizmos.fState.mAxisYGeom;
        break;
    case Gizmos::AxisConstraint::Uniform:
        geomPtr = &mGizmos.fState.mAxisUniformGeom;
        break;
    default:
        return false;
    }
    const Gizmos::AxisGeometry &geom = *geomPtr;

    if (!geom.visible) { return false; }

    if (geom.usePolygon && geom.polygonPoints.size() >= 3) {
        QPolygonF poly(geom.polygonPoints);
        return poly.containsPoint(pos, Qt::OddEvenFill);
    }

    const QPointF relative = pos - geom.center;
    const qreal angleRadGeom = qDegreesToRadians(geom.angleDeg);
    const qreal cosG = std::cos(angleRadGeom);
    const qreal sinG = std::sin(angleRadGeom);
    const qreal localX = relative.x() * cosG + relative.y() * sinG;
    const qreal localY = -relative.x() * sinG + relative.y() * cosG;
    const qreal halfW = geom.size.width() * 0.5;
    const qreal halfH = geom.size.height() * 0.5;
    return std::abs(localX) <= halfW && std::abs(localY) <= halfH;
}

void Canvas::setScaleGizmoHover(Gizmos::ScaleHandle handle,
                                bool hovered)
{
    bool *target = nullptr;
    switch (handle) {
    case Gizmos::ScaleHandle::X:
        target = &mGizmos.fState.mScaleXHovered;
        break;
    case Gizmos::ScaleHandle::Y:
        target = &mGizmos.fState.mScaleYHovered;
        break;
    case Gizmos::ScaleHandle::Uniform:
        target = &mGizmos.fState.mScaleUniformHovered;
        break;
    case Gizmos::ScaleHandle::None:
    default:
        return;
    }

    if (*target == hovered) { return; }
    *target = hovered;
    emit requestUpdate();
}

void Canvas::setShearGizmoHover(Gizmos::ShearHandle handle,
                                bool hovered)
{
    bool *target = nullptr;
    switch (handle) {
    case Gizmos::ShearHandle::X:
        target = &mGizmos.fState.mShearXHovered;
        break;
    case Gizmos::ShearHandle::Y:
        target = &mGizmos.fState.mShearYHovered;
        break;
    case Gizmos::ShearHandle::None:
    default:
        return;
    }

    if (*target == hovered) { return; }
    *target = hovered;
    emit requestUpdate();
}

void Canvas::setAxisGizmoHover(Gizmos::AxisConstraint axis,
                               bool hovered)
{
    bool *target = nullptr;
    switch (axis) {
    case Gizmos::AxisConstraint::X:
        target = &mGizmos.fState.mAxisXHovered;
        break;
    case Gizmos::AxisConstraint::Y:
        target = &mGizmos.fState.mAxisYHovered;
        break;
    case Gizmos::AxisConstraint::Uniform:
        target = &mGizmos.fState.mAxisUniformHovered;
        break;
    case Gizmos::AxisConstraint::None:
    default:
        return;
    }

    if (*target == hovered) { return; }
    *target = hovered;
    emit requestUpdate();
}
