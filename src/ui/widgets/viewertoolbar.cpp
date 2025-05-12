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

#include "viewertoolbar.h"
#include "Private/document.h"

using namespace Friction::Ui;

ViewerToolBar::ViewerToolBar(const QString &name,
                             const QString &title,
                             QWidget *parent)
    : ToolBar(name, parent, true)
    , mGroupCommon(nullptr)
    , mGroupTransform(nullptr)
    , mGroupPath(nullptr)
    , mGroupCircle(nullptr)
    , mGroupRectangle(nullptr)
    , mGroupText(nullptr)
    , mGroupDraw(nullptr)
    , mGroupPick(nullptr)
{
    setToolButtonStyle(Qt::ToolButtonIconOnly);
    setContextMenuPolicy(Qt::NoContextMenu);
    setAllowedAreas(Qt::TopToolBarArea | Qt::BottomToolBarArea);
    setWindowTitle(title);

    mGroupCommon = new QActionGroup(this);
    mGroupTransform = new QActionGroup(this);
    mGroupPath = new QActionGroup(this);
    mGroupCircle = new QActionGroup(this);
    mGroupRectangle = new QActionGroup(this);
    mGroupText = new QActionGroup(this);
    mGroupDraw = new QActionGroup(this);
    mGroupPick = new QActionGroup(this);
}

void ViewerToolBar::setCurrentCanvas(Canvas * const target)
{
    mCanvas.assign(target);
    if (target) {
        mCanvas << connect(mCanvas, &Canvas::currentBoxChanged,
                           this, &ViewerToolBar::setCurrentBox);
        mCanvas << connect(mCanvas, &Canvas::canvasModeSet,
                           this, &ViewerToolBar::setCanvasMode);
    }
    setCurrentBox(target ? target->getCurrentBox() : nullptr);
}

void ViewerToolBar::setCurrentBox(BoundingBox * const target)
{
    Q_UNUSED(target)
}

void ViewerToolBar::setCanvasMode(const CanvasMode &mode)
{
    mGroupCommon->setVisible(true);
    mGroupTransform->setVisible(mode == CanvasMode::boxTransform);
    mGroupPath->setVisible(mode == CanvasMode::pointTransform);
    mGroupCircle->setVisible(mode == CanvasMode::circleCreate);
    mGroupRectangle->setVisible(mode == CanvasMode::rectCreate ||
                                mode == CanvasMode::circleCreate);
    mGroupText->setVisible(mode == CanvasMode::textCreate);
    mGroupDraw->setVisible(mode == CanvasMode::drawPath);
    mGroupPick->setVisible(mode == CanvasMode::pickFillStroke ||
                           mode == CanvasMode::pickFillStrokeEvent);
}

void ViewerToolBar::addCanvasAction(QAction *action)
{
    if (!action) { return; }
    addAction(mGroupCommon->addAction(action));
}

void ViewerToolBar::addCanvasAction(const CanvasMode &mode,
                                    QAction *action)
{
    if (!action) { return; }
    switch (mode) {
    case CanvasMode::boxTransform:
        addAction(mGroupTransform->addAction(action));
        break;
    case CanvasMode::pointTransform:
        addAction(mGroupPath->addAction(action));
        break;
    case CanvasMode::circleCreate:
        addAction(mGroupCircle->addAction(action));
        break;
    case CanvasMode::rectCreate:
        addAction(mGroupRectangle->addAction(action));
        break;
    case CanvasMode::textCreate:
        addAction(mGroupText->addAction(action));
        break;
    case CanvasMode::drawPath:
        addAction(mGroupPick->addAction(action));
        break;
    case CanvasMode::pickFillStroke:
    case CanvasMode::pickFillStrokeEvent:
        addAction(mGroupPick->addAction(action));
        break;
    default:;
    }
}

void ViewerToolBar::addCanvasWidget(QWidget *widget)
{
    if (!widget) { return; }
    mGroupCommon->addAction(addWidget(widget));
}

void ViewerToolBar::addCanvasWidget(const CanvasMode &mode,
                                    QWidget *widget)
{
    if (!widget) { return; }
    switch (mode) {
    case CanvasMode::boxTransform:
        mGroupTransform->addAction(addWidget(widget));
        break;
    case CanvasMode::pointTransform:
        mGroupPath->addAction(addWidget(widget));
        break;
    case CanvasMode::circleCreate:
        mGroupCircle->addAction(addWidget(widget));
        break;
    case CanvasMode::rectCreate:
        mGroupRectangle->addAction(addWidget(widget));
        break;
    case CanvasMode::textCreate:
        mGroupText->addAction(addWidget(widget));
        break;
    case CanvasMode::drawPath:
        mGroupPick->addAction(addWidget(widget));
        break;
    case CanvasMode::pickFillStroke:
    case CanvasMode::pickFillStrokeEvent:
        mGroupPick->addAction(addWidget(widget));
        break;
    default:;
    }
}
