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

#ifndef GRIDCONTROLLER_H
#define GRIDCONTROLLER_H

#include "core_global.h"
#include "Animators/coloranimator.h"
#include "smartPointers/ememory.h"


#include <QRectF>
#include <QTransform>
#include <QPointF>
#include <QColor>

#include <cmath>

class QPainter;
class SkCanvas;

namespace Friction {
namespace Core {

struct CORE_EXPORT GridSettings {
    GridSettings()
        : colorAnimator(enve::make_shared<ColorAnimator>())
    {
        colorAnimator->setColor(QColor(255, 255, 255, 96));
    }

    double sizeX = 50.0;
    double sizeY = 50.0;
    double originX = 0.0;
    double originY = 0.0;
    int snapThresholdPx = 8;
    bool enabled = true;
    bool show = true;
    int majorEvery = 5;
    qsptr<ColorAnimator> colorAnimator;

    bool operator==(const GridSettings& other) const;
    bool operator!=(const GridSettings& other) const { return !(*this == other); }
};

class CORE_EXPORT GridController {
public:
    GridSettings settings;

    void drawGrid(QPainter* painter,
                  const QRectF& worldViewport,
                  const QTransform& worldToScreen,
                  qreal devicePixelRatio = 1.0) const;
    void drawGrid(SkCanvas* canvas,
                  const QRectF& worldViewport,
                  const QTransform& worldToScreen,
                  qreal devicePixelRatio = 1.0) const;

    QPointF maybeSnapPivot(const QPointF& pivotWorld,
                           const QTransform& worldToScreen,
                           bool forceSnap,
                           bool bypassSnap) const;

private:
    enum class Orientation {
        Vertical,
        Horizontal
    };

    template<typename DrawLineFunc>
    void forEachGridLine(const QRectF& worldViewport,
                         const QTransform& worldToScreen,
                         qreal devicePixelRatio,
                         DrawLineFunc&& drawLine) const;

};

} // namespace Core
} // namespace Friction

#endif // GRIDCONTROLLER_H
