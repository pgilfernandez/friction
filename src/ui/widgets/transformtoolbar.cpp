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
#include "GUI/global.h"
#include "Private/document.h"
#include "Boxes/circle.h"

using namespace Friction::Ui;

TransformToolBar::TransformToolBar(QWidget *parent)
    : ToolBar("TransformToolBar", parent)
    , mTransformX(nullptr)
    , mTransformY(nullptr)
    , mTransformR(nullptr)
    , mTransformSX(nullptr)
    , mTransformSY(nullptr)
    , mTransformRX(nullptr)
    , mTransformRY(nullptr)
    , mTransformRAct(nullptr)
    , mTransformRXAct(nullptr)
    , mTransformRYAct(nullptr)
{
    setupWidgets();
}

void TransformToolBar::setCurrentCanvas(Canvas * const target)
{
    mCanvas.assign(target);
    if (target) {
        setCurrentBox(target->getCurrentBox());
        mCanvas << connect(mCanvas, &Canvas::currentBoxChanged,
                           this, &TransformToolBar::setCurrentBox);
    }
}

void TransformToolBar::setCurrentBox(BoundingBox * const target)
{
    if (!target) {
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

    const auto rot = animator->getRotAnimator();
    mTransformR->setTarget(rot);

    const auto scale = animator->getScaleAnimator();
    mTransformSX->setTarget(scale ? scale->getXAnimator() : nullptr);
    mTransformSY->setTarget(scale ? scale->getYAnimator() : nullptr);

    if (const auto &circle = enve_cast<Circle*>(target)) {
        mTransformRAct->setVisible(true);
        mTransformRXAct->setVisible(true);
        mTransformRYAct->setVisible(true);
        mTransformRX->setTarget(circle->getHRadiusAnimator()->getXAnimator());
        mTransformRY->setTarget(circle->getVRadiusAnimator()->getYAnimator());
    } else {
        mTransformRAct->setVisible(false);
        mTransformRXAct->setVisible(false);
        mTransformRYAct->setVisible(false);
        mTransformRX->setTarget(nullptr);
        mTransformRY->setTarget(nullptr);
    }

    setEnabled(true);
}

void TransformToolBar::clearTransform()
{
    mTransformX->setTarget(nullptr);
    mTransformX->setTarget(nullptr);
    mTransformR->setTarget(nullptr);
    mTransformSX->setTarget(nullptr);
    mTransformSY->setTarget(nullptr);
    mTransformRX->setTarget(nullptr);
    mTransformRY->setTarget(nullptr);
    setEnabled(false);
}

void TransformToolBar::setupWidgets()
{
    setEnabled(false);
    setWindowTitle(tr("Transform Toolbar"));

    mTransformX = new QrealAnimatorValueSlider(nullptr, this);
    mTransformY = new QrealAnimatorValueSlider(nullptr, this);
    mTransformR = new QrealAnimatorValueSlider(nullptr, this);
    mTransformSX = new QrealAnimatorValueSlider(nullptr, this);
    mTransformSY = new QrealAnimatorValueSlider(nullptr, this);
    mTransformRX = new QrealAnimatorValueSlider(nullptr, this);
    mTransformRY = new QrealAnimatorValueSlider(nullptr, this);

    addAction(QIcon::fromTheme("boxTransform"), tr("Move"));
    addWidget(mTransformX);
    addWidget(mTransformY);

    addAction(QIcon::fromTheme("loop3"), tr("Rotate"));
    addWidget(mTransformR);

    addAction(QIcon::fromTheme("fullscreen"), tr("Scale"));
    addWidget(mTransformSX);
    addWidget(mTransformSY);

    mTransformRAct = addAction(tr("Radius"));
    mTransformRXAct = addWidget(mTransformRX);
    mTransformRYAct = addWidget(mTransformRY);

    mTransformRAct->setVisible(false);
    mTransformRXAct->setVisible(false);
    mTransformRYAct->setVisible(false);
}
