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

extern const qreal kRotateGizmoSweepDeg;
extern const qreal kRotateGizmoBaseOffsetDeg;
extern const qreal kRotateGizmoRadiusPx;
extern const qreal kRotateGizmoStrokePx;
extern const qreal kRotateGizmoHitWidthPx;
extern const qreal kAxisGizmoWidthPx;
extern const qreal kAxisGizmoHeightPx;
extern const qreal kAxisGizmoYOffsetPx;
extern const qreal kAxisGizmoXOffsetPx;
extern const qreal kScaleGizmoSizePx;
extern const qreal kScaleGizmoGapPx;
extern const qreal kShearGizmoRadiusPx;
extern const qreal kShearGizmoGapPx;

void drawCanvasGizmos(Canvas& canvas,
                       SkCanvas* const surface,
                       float invZoom,
                       qreal qInvZoom,
                       bool useLocalOrigin = false);

#endif // CANVASGIZMOS_H
