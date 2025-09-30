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

#ifndef CANVASGIZMOS_H
#define CANVASGIZMOS_H

#include <QtGlobal>

class Canvas;
class SkCanvas;

inline constexpr qreal kRotateGizmoSweepDeg = 90.0;
inline constexpr qreal kRotateGizmoBaseOffsetDeg = 270.0;
inline constexpr qreal kRotateGizmoRadiusPx = 40.0;
inline constexpr qreal kRotateGizmoStrokePx = 6.0;
inline constexpr qreal kRotateGizmoHitWidthPx = kRotateGizmoStrokePx;
inline constexpr qreal kAxisGizmoWidthPx = 5.0;
inline constexpr qreal kAxisGizmoHeightPx = 60.0;
inline constexpr qreal kAxisGizmoYOffsetPx = 40.0;
inline constexpr qreal kAxisGizmoXOffsetPx = 40.0;
inline constexpr qreal kScaleGizmoSizePx = 10.0;
inline constexpr qreal kScaleGizmoGapPx = 4.0;
inline constexpr qreal kShearGizmoRadiusPx = 6.0;
inline constexpr qreal kShearGizmoGapPx = 4.0;

void drawCanvasGizmos(Canvas& canvas,
                       SkCanvas* const surface,
                       float invZoom,
                       qreal qInvZoom);

#endif // CANVASGIZMOS_H
