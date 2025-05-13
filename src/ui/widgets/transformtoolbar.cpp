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
    , mCanvasMode(CanvasMode::boxTransform)
    , mTransformX(nullptr)
    , mTransformY(nullptr)
    , mTransformR(nullptr)
    , mTransformSX(nullptr)
    , mTransformSY(nullptr)
    , mTransformRX(nullptr)
    , mTransformRY(nullptr)
    , mTransformPX(nullptr)
    , mTransformPY(nullptr)
    , mTransformOX(nullptr)
    , mTransformMove(nullptr)
    , mTransformRotate(nullptr)
    , mTransformScale(nullptr)
    , mTransformRadius(nullptr)
    , mTransformPivot(nullptr)
    , mTransformOpacity(nullptr)
{
    setToolButtonStyle(Qt::ToolButtonIconOnly);
    setContextMenuPolicy(Qt::NoContextMenu);
    setAllowedAreas(Qt::TopToolBarArea | Qt::BottomToolBarArea);
    setWindowTitle(tr("Transform Toolbar"));

    setupWidgets();
}

void TransformToolBar::setCurrentCanvas(Canvas * const target)
{
    mCanvas.assign(target);
    if (target) {
        mCanvas << connect(mCanvas, &Canvas::currentBoxChanged,
                           this, &TransformToolBar::setCurrentBox);
        mCanvas << connect(mCanvas, &Canvas::canvasModeSet,
                           this, &TransformToolBar::setCanvasMode);
    }
    setCurrentBox(target ? target->getCurrentBox() : nullptr);
}

void TransformToolBar::setCurrentBox(BoundingBox * const target)
{
    setTransform(target);
}

void TransformToolBar::setCanvasMode(const CanvasMode &mode)
{
    mCanvasMode = mode;

    const bool hasPivot = mTransformPX->hasTarget() && mTransformPY->hasTarget();
    const bool hasOpacity = mTransformOX->hasTarget();
    mTransformPivot->setVisible(hasPivot && mode == CanvasMode::boxTransform);
    mTransformOpacity->setVisible(hasOpacity && mode == CanvasMode::boxTransform);

    const bool hasRadius = mTransformRX->hasTarget() && mTransformRY->hasTarget();
    const bool showRadius = mode == CanvasMode::boxTransform ||
                            mode == CanvasMode::circleCreate ||
                            mode == CanvasMode::rectCreate;
    mTransformRadius->setVisible(hasRadius && showRadius);
}

void TransformToolBar::setTransform(BoundingBox * const target)
{
    const bool multiple = target ? mCanvas->getSelectedBoxesCount() > 1 : false;
    // TODO: add support for multiple boxes

    if (!target || multiple) {
        clearTransform();
        return;
    }

    const auto animator = target->getBoxTransformAnimator();
    if (!animator) {
        clearTransform();
        return;
    }

    const auto pos = animator->getPosAnimator();
    mTransformX->setTarget(pos ? pos->getXAnimator() : nullptr);
    mTransformY->setTarget(pos ? pos->getYAnimator() : nullptr);

    mTransformMove->setEnabled(pos);

    const auto rot = animator->getRotAnimator();
    mTransformR->setTarget(rot);

    mTransformRotate->setEnabled(rot);

    const auto scale = animator->getScaleAnimator();
    mTransformSX->setTarget(scale ? scale->getXAnimator() : nullptr);
    mTransformSY->setTarget(scale ? scale->getYAnimator() : nullptr);

    mTransformScale->setEnabled(scale);

    const auto pivot = animator->getPivotAnimator();
    mTransformPX->setTarget(pivot ? pivot->getXAnimator() : nullptr);
    mTransformPY->setTarget(pivot ? pivot->getYAnimator() : nullptr);

    mTransformPivot->setEnabled(pivot);

    const auto opacity = animator->getOpacityAnimator();
    mTransformOX->setTarget(opacity);

    mTransformOpacity->setEnabled(opacity);

    const auto circle = enve_cast<Circle*>(target);
    const auto rectangle = enve_cast<RectangleBox*>(target);

    mTransformRX->setTarget(circle ?
                                circle->getHRadiusAnimator()->getXAnimator() :
                                (rectangle ? rectangle->getRadiusAnimator()->getXAnimator() : nullptr));
    mTransformRY->setTarget(circle ?
                                circle->getVRadiusAnimator()->getYAnimator() :
                                (rectangle ? rectangle->getRadiusAnimator()->getYAnimator() : nullptr));

    const bool hasRadius = (circle || rectangle);
    mTransformRadius->setEnabled(hasRadius);

    setCanvasMode(mCanvasMode);
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
    mTransformPX->setTarget(nullptr);
    mTransformPY->setTarget(nullptr);
    mTransformOX->setTarget(nullptr);

    mTransformMove->setEnabled(false);
    mTransformRotate->setEnabled(false);
    mTransformScale->setEnabled(false);
    mTransformRadius->setEnabled(false);
    mTransformPivot->setEnabled(false);
    mTransformOpacity->setEnabled(false);

    mTransformRadius->setVisible(false);
}

void TransformToolBar::setupWidgets()
{
    setupTransform();
}

void TransformToolBar::setupTransform()
{
    mTransformMove = new QActionGroup(this);
    mTransformRotate = new QActionGroup(this);
    mTransformScale = new QActionGroup(this);
    mTransformRadius = new QActionGroup(this);
    mTransformPivot = new QActionGroup(this);
    mTransformOpacity = new QActionGroup(this);

    mTransformX = new QrealAnimatorValueSlider(nullptr, this);
    mTransformY = new QrealAnimatorValueSlider(nullptr, this);
    mTransformR = new QrealAnimatorValueSlider(nullptr, this);
    mTransformSX = new QrealAnimatorValueSlider(nullptr, this);
    mTransformSY = new QrealAnimatorValueSlider(nullptr, this);
    mTransformRX = new QrealAnimatorValueSlider(nullptr, this);
    mTransformRY = new QrealAnimatorValueSlider(nullptr, this);
    mTransformPX = new QrealAnimatorValueSlider(nullptr, this);
    mTransformPY = new QrealAnimatorValueSlider(nullptr, this);
    mTransformOX =new QrealAnimatorValueSlider(nullptr, this);

    mTransformMove->addAction(addAction(QIcon::fromTheme("boxTransform"),
                                        tr("Move")));
    mTransformMove->addAction(addWidget(mTransformX));
    mTransformMove->addAction(addSeparator());
    mTransformMove->addAction(addWidget(mTransformY));

    mTransformRotate->addAction(addAction(QIcon::fromTheme("loop3"),
                                          tr("Rotate")));
    mTransformRotate->addAction(addWidget(mTransformR));

    mTransformScale->addAction(addAction(QIcon::fromTheme("fullscreen"),
                                         tr("Scale")));
    mTransformScale->addAction(addWidget(mTransformSX));
    mTransformScale->addAction(addSeparator());
    mTransformScale->addAction(addWidget(mTransformSY));

    mTransformPivot->addAction(addAction(QIcon::fromTheme("pivot"),
                                         tr("Pivot")));
    mTransformPivot->addAction(addWidget(mTransformPX));
    mTransformPivot->addAction(addSeparator());
    mTransformPivot->addAction(addWidget(mTransformPY));

    mTransformOpacity->addAction(addAction(QIcon::fromTheme("alpha"),
                                           tr("Opacity")));
    mTransformOpacity->addAction(addWidget(mTransformOX));

    mTransformRadius->addAction(addAction(QIcon::fromTheme("circleCreate"),
                                          tr("Radius")));
    mTransformRadius->addAction(addWidget(mTransformRX));
    mTransformRadius->addAction(addSeparator());
    mTransformRadius->addAction(addWidget(mTransformRY));

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
    mTransformPX->setValueRange(0, 1);
    mTransformPX->setDisplayedValue(0);
    mTransformPY->setValueRange(0, 1);
    mTransformPY->setDisplayedValue(0);
    mTransformOX->setValueRange(0, 100);
    mTransformOX->setDisplayedValue(100);
}
