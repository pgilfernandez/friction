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

#include "transformtoolbar.h"
#include "Animators/qpointfanimator.h"
#include "Animators/transformanimator.h"
#include "Private/document.h"
#include "Boxes/circle.h"
#include "Boxes/rectangle.h"

using namespace Friction::Ui;

TransformToolBar::TransformToolBar(QWidget *parent)
    : ToolBar("TransformToolBar", parent, true)
    , mTransformX(nullptr)
    , mTransformY(nullptr)
    , mTransformR(nullptr)
    , mTransformSX(nullptr)
    , mTransformSY(nullptr)
    , mTransformRX(nullptr)
    , mTransformRY(nullptr)
    , mTransformMoveLabelAct(nullptr)
    , mTransformRotateLabelAct(nullptr)
    , mTransformScaleLabelAct(nullptr)
    , mTransformRadiusLabelAct(nullptr)
    , mTransformXAct(nullptr)
    , mTransformYAct(nullptr)
    , mTransformRAct(nullptr)
    , mTransformSXAct(nullptr)
    , mTransformSYAct(nullptr)
    , mTransformRXAct(nullptr)
    , mTransformRYAct(nullptr)
{
    setupWidgets();
}

void TransformToolBar::setCurrentCanvas(Canvas * const target)
{
    mCanvas.assign(target);
    if (target) {
        mCanvas << connect(mCanvas, &Canvas::currentBoxChanged,
                           this, &TransformToolBar::setCurrentBox);
    }
    setCurrentBox(target ? target->getCurrentBox() : nullptr);
}

void TransformToolBar::setCurrentBox(BoundingBox * const target)
{
    const bool multiple = target ? mCanvas->getSelectedBoxesCount() > 1 : false;
    // TODO: add support for multiple boxes

    if (!target || multiple) {
        clearTransform();
        return;
    }

    const auto animator = target->getTransformAnimator();
    if (!animator) {
        clearTransform();
        return;
    }

    const auto pos = animator->getPosAnimator();
    mTransformX->setTarget(pos ? pos->getXAnimator() : nullptr);
    mTransformY->setTarget(pos ? pos->getYAnimator() : nullptr);

    const bool canMove = (pos);
    mTransformMoveLabelAct->setEnabled(canMove);
    mTransformXAct->setEnabled(canMove);
    mTransformYAct->setEnabled(canMove);

    const auto rot = animator->getRotAnimator();
    mTransformR->setTarget(rot);

    const bool canRotate = (rot);
    mTransformRotateLabelAct->setEnabled(canRotate);
    mTransformRAct->setEnabled(canRotate);

    const auto scale = animator->getScaleAnimator();
    mTransformSX->setTarget(scale ? scale->getXAnimator() : nullptr);
    mTransformSY->setTarget(scale ? scale->getYAnimator() : nullptr);

    const bool canScale = (scale);
    mTransformScaleLabelAct->setEnabled(canScale);
    mTransformSXAct->setEnabled(canScale);
    mTransformSYAct->setEnabled(canScale);

    const auto circle = enve_cast<Circle*>(target);
    const auto rectangle = enve_cast<RectangleBox*>(target);

    mTransformRX->setTarget(circle ?
                                circle->getHRadiusAnimator()->getXAnimator() :
                                (rectangle ? rectangle->getRadiusAnimator()->getXAnimator() : nullptr));
    mTransformRY->setTarget(circle ?
                                circle->getVRadiusAnimator()->getYAnimator() :
                                (rectangle ? rectangle->getRadiusAnimator()->getYAnimator() : nullptr));

    const bool hasRadius = (circle || rectangle);
    mTransformRadiusLabelAct->setEnabled(hasRadius);
    mTransformRXAct->setEnabled(hasRadius);
    mTransformRYAct->setEnabled(hasRadius);
}

void TransformToolBar::clearTransform()
{
    mTransformX->setTarget(nullptr);
    mTransformY->setTarget(nullptr);
    mTransformR->setTarget(nullptr);
    mTransformSX->setTarget(nullptr);
    mTransformSY->setTarget(nullptr);
    mTransformRX->setTarget(nullptr);
    mTransformRY->setTarget(nullptr);

    mTransformMoveLabelAct->setEnabled(false);
    mTransformXAct->setEnabled(false);
    mTransformYAct->setEnabled(false);

    mTransformRotateLabelAct->setEnabled(false);
    mTransformRAct->setEnabled(false);

    mTransformScaleLabelAct->setEnabled(false);
    mTransformSXAct->setEnabled(false);
    mTransformSYAct->setEnabled(false);

    mTransformRadiusLabelAct->setEnabled(false);
    mTransformRXAct->setEnabled(false);
    mTransformRYAct->setEnabled(false);
}

void TransformToolBar::setupWidgets()
{
    setToolButtonStyle(Qt::ToolButtonIconOnly);
    setContextMenuPolicy(Qt::NoContextMenu);
    setAllowedAreas(Qt::TopToolBarArea | Qt::BottomToolBarArea);

    setWindowTitle(tr("Transform Toolbar"));

    mTransformX = new QrealAnimatorValueSlider(nullptr, this);
    mTransformY = new QrealAnimatorValueSlider(nullptr, this);
    mTransformR = new QrealAnimatorValueSlider(nullptr, this);
    mTransformSX = new QrealAnimatorValueSlider(nullptr, this);
    mTransformSY = new QrealAnimatorValueSlider(nullptr, this);
    mTransformRX = new QrealAnimatorValueSlider(nullptr, this);
    mTransformRY = new QrealAnimatorValueSlider(nullptr, this);

    mTransformMoveLabelAct = addAction(QIcon::fromTheme("boxTransform"),
                                       tr("Move"));
    mTransformXAct = addWidget(mTransformX);
    addSeparator();
    mTransformYAct = addWidget(mTransformY);

    mTransformRotateLabelAct = addAction(QIcon::fromTheme("loop3"),
                                         tr("Rotate"));
    mTransformRAct = addWidget(mTransformR);

    mTransformScaleLabelAct = addAction(QIcon::fromTheme("fullscreen"),
                                        tr("Scale"));
    mTransformSXAct = addWidget(mTransformSX);
    addSeparator();
    mTransformSYAct = addWidget(mTransformSY);

    mTransformRadiusLabelAct = addAction(QIcon::fromTheme("circleCreate"),
                                         tr("Radius"));
    mTransformRXAct = addWidget(mTransformRX);
    addSeparator();
    mTransformRYAct = addWidget(mTransformRY);

    clearTransform();

    mTransformX->setValueRange(0, 1);
    mTransformX->setDisplayedValue(0);
    mTransformY->setValueRange(0, 1);
    mTransformY->setDisplayedValue(0);
    mTransformR->setValueRange(0, 1);
    mTransformR->setDisplayedValue(0);
    mTransformSX->setValueRange(0, 1);
    mTransformSX->setDisplayedValue(0);
    mTransformSY->setValueRange(0, 1);
    mTransformSY->setDisplayedValue(0);
    mTransformRX->setValueRange(0, 1);
    mTransformRX->setDisplayedValue(0);
    mTransformRY->setValueRange(0, 1);
    mTransformRY->setDisplayedValue(0);
}
