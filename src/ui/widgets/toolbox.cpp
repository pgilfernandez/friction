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

#include "toolbox.h"

using namespace Friction::Ui;

ToolBox::ToolBox(Actions &actions,
                 Document &document,
                 QWidget *parent)
    : QObject{parent}
    , mActions(actions)
    , mDocument(document)
    , mMain(nullptr)
    , mControls(nullptr)
    , mExtra(nullptr)
    , mGroupMain(nullptr)
{
    setupToolBox(parent);
}

QToolBar *ToolBox::getToolBar(const Type &type)
{
    switch (type) {
    case Type::Controls:
        return mControls;
    case Type::Extra:
        return mExtra;
    default:
        return mMain;
    }
    return nullptr;
}

void ToolBox::setupToolBox(QWidget *parent)
{
    if (!parent) { return; }

    mMain = new ToolBar(tr("ToolBox"),
                        "ToolBoxMain",
                        parent,
                        true);
    mControls = new ToolControls(parent);
    mExtra = new ToolBar(tr("Tool Extra"),
                         "ToolBoxExtra",
                         parent,
                         true);

    mGroupMain = new QActionGroup(this);

    setupDocument();
    setupActions(parent);
}

void ToolBox::setupDocument()
{
    // TODO
}

void ToolBox::setupMainAction(const QIcon &icon,
                              const QString &title,
                              const QKeySequence &shortcut,
                              const QList<CanvasMode> &modes,
                              const bool checked,
                              QObject *parent)
{
    if (modes.isEmpty() ||
        icon.isNull() ||
        title.isEmpty() ||
        !parent) { return; }

    const auto act = new QAction(icon,
                                 title,
                                 parent);
    act->setCheckable(true);
    act->setChecked(checked);
    act->setShortcut(shortcut);
    mGroupMain->addAction(act);

    connect(act,
            &QAction::triggered,
            this,
            [this, modes](bool checked) {
        if (!checked) { return; }
        switch (modes.at(0)) {
        case CanvasMode::boxTransform:
            mActions.setMovePathMode();
            break;
        case CanvasMode::pointTransform:
            mActions.setMovePointMode();
            break;
        case CanvasMode::pathCreate:
            mActions.setAddPointMode();
            break;
        case CanvasMode::drawPath:
            mActions.setDrawPathMode();
            break;
        case CanvasMode::circleCreate:
            mActions.setCircleMode();
            break;
        case CanvasMode::rectCreate:
            mActions.setRectangleMode();
            break;
        case CanvasMode::textCreate:
            mActions.setTextMode();
            break;
        case CanvasMode::nullCreate:
            mActions.setNullMode();
            break;
        case CanvasMode::pickFillStroke:
            mActions.setPickPaintSettingsMode();
            break;
        default:;
        }
    });
    connect(&mDocument,
            &Document::canvasModeSet,
            this,
            [act, modes](CanvasMode mode) {
        if (modes.contains(mode)) { act->setChecked(true); }
    });
}

void ToolBox::setupActions(QWidget *parent)
{
    if (!parent) { return; }

    setupMainAction(QIcon::fromTheme("boxTransform"),
                    tr("Object Mode"),
                    QKeySequence(AppSupport::getSettings("shortcuts",
                                                         "boxTransform",
                                                         "F1").toString()),
                    {CanvasMode::boxTransform},
                    true,
                    parent);
    setupMainAction(QIcon::fromTheme("pointTransform"),
                    tr("Point Mode"),
                    QKeySequence(AppSupport::getSettings("shortcuts",
                                                         "pointTransform",
                                                         "F2").toString()),
                    {CanvasMode::pointTransform},
                    false,
                    parent);
    setupMainAction(QIcon::fromTheme("pathCreate"),
                    tr("Add Path"),
                    QKeySequence(AppSupport::getSettings("shortcuts",
                                                         "pathCreate",
                                                         "F3").toString()),
                    {CanvasMode::pathCreate},
                    false,
                    parent);
    setupMainAction(QIcon::fromTheme("drawPath"),
                    tr("Draw Path"),
                    QKeySequence(AppSupport::getSettings("shortcuts",
                                                         "drawPath",
                                                         "F4").toString()),
                    {CanvasMode::drawPath},
                    false,
                    parent);
    setupMainAction(QIcon::fromTheme("circleCreate"),
                    tr("Add Circle"),
                    QKeySequence(AppSupport::getSettings("shortcuts",
                                                         "circleMode",
                                                         "F5").toString()),
                    {CanvasMode::circleCreate},
                    false,
                    parent);
    setupMainAction(QIcon::fromTheme("rectCreate"),
                    tr("Add Rectangle"),
                    QKeySequence(AppSupport::getSettings("shortcuts",
                                                         "rectMode",
                                                         "F6").toString()),
                    {CanvasMode::rectCreate},
                    false,
                    parent);
    setupMainAction(QIcon::fromTheme("textCreate"),
                    tr("Add Text"),
                    QKeySequence(AppSupport::getSettings("shortcuts",
                                                         "textMode",
                                                         "F7").toString()),
                    {CanvasMode::textCreate},
                    false,
                    parent);
    setupMainAction(QIcon::fromTheme("nullCreate"),
                    tr("Add Null Object"),
                    QKeySequence(AppSupport::getSettings("shortcuts",
                                                         "nullMode",
                                                         "F8").toString()),
                    {CanvasMode::nullCreate},
                    false,
                    parent);
    setupMainAction(QIcon::fromTheme("pick"),
                    tr("Color Pick Mode"),
                    QKeySequence(AppSupport::getSettings("shortcuts",
                                                         "pickMode",
                                                         "F9").toString()),
                    {CanvasMode::pickFillStroke,
                     CanvasMode::pickFillStrokeEvent},
                    false,
                    parent);

    {
        // pivot
        const auto act = new QAction(mDocument.fLocalPivot ?
                                         QIcon::fromTheme("pivotLocal") :
                                         QIcon::fromTheme("pivotGlobal"),
                                     tr("Pivot Global / Local"),
                                     parent);
        act->setShortcut(QKeySequence(AppSupport::getSettings("shortcuts",
                                      "localPivot",
                                      "P").toString()));
        connect(act, &QAction::triggered,
                this, [this, act]() {
            mDocument.fLocalPivot = !mDocument.fLocalPivot;
            for (const auto& scene : mDocument.fScenes) { scene->updatePivot(); }
            Document::sInstance->actionFinished();
            act->setIcon(mDocument.fLocalPivot ?
                             QIcon::fromTheme("pivotLocal") :
                             QIcon::fromTheme("pivotGlobal"));
        });
        mGroupMain->addAction(act);
    }

    mMain->addActions(mGroupMain->actions());
}
