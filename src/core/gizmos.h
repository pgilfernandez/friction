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

#ifndef FRICTION_GIZMOS
#define FRICTION_GIZMOS

#include "core_global.h"

#include <QObject>
#include <QColor>
#include <QPointF>
#include <QSizeF>
#include <QVector>

namespace Friction
{
    namespace Core
    {
        class CORE_EXPORT Gizmos : public QObject
        {
        public:
            enum class AxisConstraint
            {
                None,
                X,
                Y,
                Uniform
            };
            enum class ScaleHandle
            {
                None,
                X,
                Y,
                Uniform
            };
            enum class ShearHandle
            {
                None,
                X,
                Y
            };
            struct AxisGeometry {
                QPointF center;
                QSizeF size;
                qreal angleDeg = 0.0;
                bool visible = false;
                bool usePolygon = false;
                QVector<QPointF> polygonPoints;
            };
            struct ScaleGeometry {
                QPointF center;
                qreal halfExtent = 0.0;
                bool visible = false;
                bool usePolygon = false;
                QVector<QPointF> polygonPoints;
            };
            struct ShearGeometry {
                QPointF center;
                qreal radius = 0.0;
                bool visible = false;
                bool usePolygon = false;
                QVector<QPointF> polygonPoints;
            };
            struct Config
            {
                qreal kRotateGizmoSweepDeg = 90.0; // default sweep of gizmo arc
                qreal kRotateGizmoBaseOffsetDeg = 270.0; // default angular offset for gizmo arc
                qreal kRotateGizmoRadiusPx = 45.0; // gizmo radius in screen pixels
                qreal kRotateGizmoStrokePx = 4.0; // arc stroke thickness in screen pixels
                qreal kRotateGizmoHitWidthPx = kRotateGizmoStrokePx + 2.0; // hit area thickness in screen pixels
                qreal kAxisGizmoWidthPx = 5.0; // axis gizmo rectangle width in screen pixels
                qreal kAxisGizmoHeightPx = 60.0; // axis gizmo rectangle height in screen pixels
                qreal kAxisGizmoYOffsetPx = 40.0; // vertical distance of Y gizmo from pivot in pixels
                qreal kAxisGizmoUniformOffsetPx = 7.0; // XY offset from pivot for Uniform position gizmo in pixels
                qreal kAxisGizmoUniformWidthPx = 26.0; // XY square width for Uniform position gizmo in pixels
                qreal kAxisGizmoUniformChamferPx = 1.0; // XY square chamfer for Uniform position gizmo in pixels
                qreal kAxisGizmoXOffsetPx = 40.0; // horizontal distance of X gizmo from pivot in pixels
                qreal kScaleGizmoSizePx = 10.0; // scale gizmo square size in screen pixels
                qreal kScaleGizmoGapPx = 2.0; // gap between position gizmos and scale gizmos in screen pixels
                qreal kShearGizmoRadiusPx = 6.0; // shear gizmo circle radius in screen pixels
                qreal kShearGizmoGapPx = 2.0; // gap between scale and shear gizmos in screen pixels
            };
            struct Theme
            {
                QColor kGizmoColorX = QColor(232, 32, 45);
                QColor kGizmoColorY = QColor(134, 232, 32);
                QColor kGizmoColorZ = QColor(32, 139, 232);
                QColor kGizmoColorUniform = QColor(232, 215, 32);
                qreal kGizmoColorAlphaFillNormal = 100.0;
                qreal kGizmoColorAlphaFillHover = 200.0;
                qreal kGizmoColorAlphaStrokeNormal = 200.0;
                qreal kGizmoColorAlphaStrokeHover = 0.0;
                int kGizmoColorLightenNormal = 120;
                int kGizmoColorLightenHover = 100;
            };
            struct State
            {
                bool mRotateHandleVisible = false;
                QPointF mRotateHandlePos;
                QPointF mRotateHandleAnchor;
                qreal mRotateHandleRadius = 0;
                qreal mRotateHandleAngleDeg = 0; // cached visual rotation of the gizmo
                qreal mRotateHandleSweepDeg = 90.0; // cached arc span used for draw + hit-test
                qreal mRotateHandleStartOffsetDeg = 45.0; // cached base offset applied before box rotation
                bool mRotateHandleHovered = false; // true when pointer hovers the gizmo
                QVector<QPointF> mRotateHandlePolygon;
                QVector<QPointF> mRotateHandleHitPolygon;
                AxisGeometry mAxisXGeom;
                AxisGeometry mAxisYGeom;
                AxisGeometry mAxisUniformGeom;
                ScaleGeometry mScaleXGeom;
                ScaleGeometry mScaleYGeom;
                ScaleGeometry mScaleUniformGeom;
                ShearGeometry mShearXGeom;
                ShearGeometry mShearYGeom;
                bool mAxisXHovered = false;
                bool mAxisYHovered = false;
                bool mAxisUniformHovered = false;
                bool mScaleXHovered = false;
                bool mScaleYHovered = false;
                bool mScaleUniformHovered = false;
                bool mShearXHovered = false;
                bool mShearYHovered = false;
                AxisConstraint mAxisConstraint = AxisConstraint::None;
                ScaleHandle mScaleConstraint = ScaleHandle::None;
                ShearHandle mShearConstraint = ShearHandle::None;
                bool mAxisHandleActive = false;
                bool mScaleHandleActive = false;
                bool mShearHandleActive = false;
                bool mGizmosSuppressed = false;
                bool mShowRotateGizmo = true;
                bool mShowPositionGizmo = true;
                bool mShowScaleGizmo = false;
                bool mShowShearGizmo = false;
                bool mRotatingFromHandle = false;
            };

            Config fConfig;
            Theme fTheme;
            State fState;
        };
    }
}

#endif // FRICTION_GIZMOS
