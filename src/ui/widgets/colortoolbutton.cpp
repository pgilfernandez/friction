/*
#
# Friction - https://friction.graphics
#
# Copyright (c) Friction contributors
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

#include "widgets/colortoolbutton.h"
#include "Animators/coloranimator.h"
#include "colorsetting.h"
#include "GUI/ewidgets.h"

#include <QVBoxLayout>

using namespace Friction::Ui;

ColorToolButton::ColorToolButton(QWidget * const parent)
    : ToolButton(parent)
    , mColorLabel(nullptr)
    , mColorWidget(nullptr)
    , mColorAct(nullptr)
{
    setAutoPopup(false); // TODO: add to settings (auto/click popup)
    setPopupMode(ToolButtonPopupMode::InstantPopup);

    mColorAct = new QWidgetAction(this);
    mColorLabel = new ColorLabel(this, false);

    const auto lay = new QVBoxLayout(this);
    lay->setContentsMargins(0, 0, 0, 0);
    lay->setMargin(0);
    lay->addWidget(mColorLabel);

    addAction(mColorAct);
    updateColor();
}

ColorToolButton::ColorToolButton(ColorAnimator * const colorTarget,
                                 QWidget * const parent)
    : ColorToolButton(parent)
{
    setColorTarget(colorTarget);
}

void ColorToolButton::setColorTarget(ColorAnimator * const target)
{
    if (mColorWidget) {
        mColorAct->releaseWidget(mColorWidget);
        delete mColorWidget;
        mColorWidget = nullptr;
    }

    if (target) {
        mColorWidget = eWidgets::sColorWidget(this, target, false, false, 1);
        mColorWidget->setContentsMargins(10, 10, 10, 10);
        mColorWidget->setMinimumWidth(250);
        mColorAct->setDefaultWidget(mColorWidget);
    }

    mColorTarget.assign(target);
    if (target) {
        mColorTarget << connect(target->getVal1Animator(),
                                &QrealAnimator::effectiveValueChanged,
                                this, qOverload<>(&ColorToolButton::updateColor));
        mColorTarget << connect(target->getVal2Animator(),
                                &QrealAnimator::effectiveValueChanged,
                                this, qOverload<>(&ColorToolButton::updateColor));
        mColorTarget << connect(target->getVal3Animator(),
                                &QrealAnimator::effectiveValueChanged,
                                this, qOverload<>(&ColorToolButton::updateColor));
    }
    updateColor();
}

void ColorToolButton::updateColor()
{
    const QColor color = mColorTarget ? mColorTarget->getColor() : Qt::black;
    mColorLabel->setColor(color);
    mColorLabel->setAlpha(color.alphaF());
}

QColor ColorToolButton::color() const
{
    if (mColorTarget) { return mColorTarget->getColor(); }
    return Qt::black;
}
