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

// Fork of enve - Copyright (C) 2016-2020 Maurycy Liebner

#include "canvas.h"

#include "eevent.h"

#include "Private/document.h"
#include "GUI/dialogsinterface.h"

#include "Boxes/boundingbox.h"
#include "Boxes/circle.h"
#include "Boxes/rectangle.h"
#include "Boxes/imagebox.h"
#include "Boxes/textbox.h"
#include "Boxes/internallinkbox.h"
#include "Boxes/containerbox.h"
#include "Boxes/smartvectorpath.h"
//#include "Boxes/paintbox.h"
#include "Boxes/nullobject.h"

#include "pointtypemenu.h"
#include "pointhelpers.h"
#include "clipboardcontainer.h"

#include "PathEffects/patheffect.h"
#include "PathEffects/patheffectsinclude.h"
#include "RasterEffects/rastereffect.h"

#include "MovablePoints/smartnodepoint.h"
#include "MovablePoints/pathpivot.h"

#include <cmath>
#include <functional>

#include <QDesktopWidget>
#include <QScreen>
#include <QMouseEvent>
#include <QMenu>
#include <QInputDialog>
#include <QApplication>

using namespace Friction::Core;

namespace {
bool pointIsFinite(const QPointF& point)
{
    return std::isfinite(point.x()) && std::isfinite(point.y());
}
}

void Canvas::collectSnapTargets(bool includePivots,
                                bool includeBounds,
                                bool includeNodes,
                                std::vector<QPointF>& pivotTargets,
                                std::vector<QPointF>& boxTargets,
                                std::vector<QPointF>& nodeTargets,
                                bool includeSelectedBounds) const
{
    pivotTargets.clear();
    boxTargets.clear();
    nodeTargets.clear();

    if ((!includePivots && !includeBounds && !includeNodes) || !mCurrentContainer) {
        return;
    }

    auto addIfValid = [](std::vector<QPointF>& target, const QPointF& pt) {
        if (pointIsFinite(pt)) {
            target.push_back(pt);
        }
    };

    auto appendBoundsTargets = [&](const QRectF& rect) {
        const QRectF normalized = rect.normalized();
        if (normalized.isNull() || !normalized.isValid()) { return; }

        const QPointF topLeft = normalized.topLeft();
        const QPointF topRight = normalized.topRight();
        const QPointF bottomLeft = normalized.bottomLeft();
        const QPointF bottomRight = normalized.bottomRight();
        const QPointF topCenter((normalized.left() + normalized.right()) * 0.5, normalized.top());
        const QPointF bottomCenter((normalized.left() + normalized.right()) * 0.5, normalized.bottom());
        const QPointF leftCenter(normalized.left(), (normalized.top() + normalized.bottom()) * 0.5);
        const QPointF rightCenter(normalized.right(), (normalized.top() + normalized.bottom()) * 0.5);

        addIfValid(boxTargets, normalized.center());
        addIfValid(boxTargets, topLeft);
        addIfValid(boxTargets, topRight);
        addIfValid(boxTargets, bottomLeft);
        addIfValid(boxTargets, bottomRight);
        addIfValid(boxTargets, topCenter);
        addIfValid(boxTargets, bottomCenter);
        addIfValid(boxTargets, leftCenter);
        addIfValid(boxTargets, rightCenter);
    };

    const std::function<void(const ContainerBox*, bool)> recurse =
        [&](const ContainerBox* container, bool ancestorSelected) {
            if (!container) { return; }
            const auto& boxes = container->getContainedBoxes();
            for (auto* box : boxes) {
                if (!box) { continue; }
                const bool selectedBranch = ancestorSelected || box->isSelected();
                const bool visible = box->isVisible();

                if (!selectedBranch && visible) {
                    if (includePivots) {
                        addIfValid(pivotTargets, box->getPivotAbsPos());
                    }
                    if (includeBounds) {
                        appendBoundsTargets(box->getAbsBoundingRect());
                    }
                    if (includeNodes) {
                        auto* mutableBox = const_cast<BoundingBox*>(box);
                        const MovablePoint::PtOp gather = [&](MovablePoint* point) {
                            if (!point || !point->isSmartNodePoint()) { return; }
                            const auto* node = static_cast<SmartNodePoint*>(point);
                            addIfValid(nodeTargets, node->getAbsolutePos());
                        };
                        mutableBox->selectAllCanvasPts(gather, CanvasMode::pointTransform);
                    }
                }

                if (const auto* childContainer = enve_cast<const ContainerBox*>(box)) {
                    recurse(childContainer, selectedBranch);
                }
            }
        };

    recurse(mCurrentContainer, false);

    if (includeBounds && includeSelectedBounds) {
        for (const auto& selectedBox : mSelectedBoxes) {
            if (!selectedBox || !selectedBox->isVisible()) { continue; }
            appendBoundsTargets(selectedBox->getAbsBoundingRect());
        }
    }
}

QPointF Canvas::snapPosToGrid(const QPointF& pos,
                              Qt::KeyboardModifiers modifiers,
                              bool forceSnap) const
{
    if (!mHasWorldToScreen) { return pos; }

    if (!mDocument.isSnappingActive()) {
        return pos;
    }

    const auto& gridController = mDocument.gridController();
    const auto& settings = gridController.settings;

    const bool bypassSnap = modifiers & Qt::AltModifier;
    if (bypassSnap) { return pos; }

    const bool gridEnabled = settings.enabled;
    const bool canvasSnapEnabled = settings.snapToCanvas;
    const bool pivotsSnapEnabled = settings.snapToPivots;
    const bool boxesSnapEnabled = settings.snapToBoxes;
    const bool nodesSnapEnabled = settings.snapToNodes;

    std::vector<QPointF> pivotTargets;
    std::vector<QPointF> boxTargets;
    std::vector<QPointF> nodeTargets;
    if (pivotsSnapEnabled || boxesSnapEnabled || nodesSnapEnabled) {
        collectSnapTargets(pivotsSnapEnabled, boxesSnapEnabled, nodesSnapEnabled,
                           pivotTargets, boxTargets, nodeTargets);
    }
    const bool hasPivotTargets = pivotsSnapEnabled && !pivotTargets.empty();
    const bool hasBoxTargets = boxesSnapEnabled && !boxTargets.empty();
    const bool hasNodeTargets = nodesSnapEnabled && !nodeTargets.empty();

    const bool hasSnapSource = gridEnabled || canvasSnapEnabled ||
                               hasPivotTargets || hasBoxTargets || hasNodeTargets;
    const bool shouldForce = (forceSnap && hasSnapSource) ||
                             (modifiers & Qt::ControlModifier);

    if (!hasSnapSource && !shouldForce) { return pos; }

    QRectF canvasRect;
    const QRectF* canvasRectPtr = nullptr;
    if (canvasSnapEnabled) {
        canvasRect = QRectF(QPointF(0.0, 0.0), QSizeF(mWidth, mHeight));
        canvasRectPtr = &canvasRect;
    }

    return gridController.maybeSnapPivot(pos,
                                         mWorldToScreen,
                                         shouldForce,
                                         false,
                                         canvasRectPtr,
                                         nullptr,
                                         hasPivotTargets ? &pivotTargets : nullptr,
                                         hasBoxTargets ? &boxTargets : nullptr,
                                         hasNodeTargets ? &nodeTargets : nullptr);
}

QPointF Canvas::snapEventPos(const eMouseEvent& e,
                             bool forceSnap) const
{
    return snapPosToGrid(e.fPos, e.fModifiers, forceSnap);
}

void Canvas::handleMovePathMousePressEvent(const eMouseEvent& e) {
    mPressedBox = mCurrentContainer->getBoxAt(e.fPos);
    if(e.shiftMod()) return;
    if(mPressedBox ? !mPressedBox->isSelected() : true) {
        clearBoxesSelection();
    }
}

void Canvas::addActionsToMenu(QMenu *const menu)
{
    const auto clipboard = mDocument.getBoxesClipboard();
    if (clipboard) {
        QAction * const pasteAct = menu->addAction(tr("Paste"), this,
                                                   &Canvas::pasteAction);
        pasteAct->setShortcut(Qt::CTRL + Qt::Key_V);
    }

    const auto sceneIcon = QIcon::fromTheme("sequence");
    QMenu * const linkCanvasMenu = menu->addMenu(sceneIcon,
                                                 tr("Link Scene"));
    for (const auto& canvas : mDocument.fScenes) {
        const auto slot = [this, canvas]() {
            auto newLink = canvas->createLink(false);
            mCurrentContainer->addContained(newLink);
            newLink->centerPivotPosition();
        };
        QAction * const action = linkCanvasMenu->addAction(sceneIcon,
                                                           canvas->prp_getName(),
                                                           this,
                                                           slot);
        if (canvas == this) {
            action->setEnabled(false);
            action->setVisible(false);
        }
    }

    menu->addAction(QIcon::fromTheme("duplicate"),
                    tr("Duplicate Scene"), [this]() {
        const auto newScene = Document::sInstance->createNewScene();
        newScene->setCanvasSize(mWidth, mHeight);
        newScene->setFps(mFps);
        newScene->setFrameRange(mRange, false);
        BoxClipboard::sCopyAndPaste(this, newScene);
        newScene->prp_setNameAction(newScene->prp_getName() + " copy");
    });

    const auto parentWidget = menu->parentWidget();
    menu->addAction(QIcon::fromTheme("file_movie"),
                    tr("Map to Different Fps"), [this, parentWidget]() {
        bool ok;
        const qreal newFps = QInputDialog::getDouble(
                    parentWidget, "Map to Different Fps",
                    "New Fps:", mFps, 1, 999, 2, &ok);
        if (ok) { changeFpsTo(newFps); }
    });

    menu->addAction(QIcon::fromTheme("sequence"),
                    tr("Scene Properties"), [this]() {
        const auto& dialogs = DialogsInterface::instance();
        dialogs.showSceneSettingsDialog(this);
    });
}

void Canvas::handleRightButtonMouseRelease(const eMouseEvent& e) {
    if(e.fMouseGrabbing) {
        cancelCurrentTransform();
        e.fReleaseMouse();
        mValueInput.clearAndDisableInput();
    } else {
        mPressedBox = mHoveredBox;
        mPressedPoint = mHoveredPoint_d;
        if(mPressedPoint) {
            QMenu qMenu;
            PointTypeMenu menu(&qMenu, this, e.fWidget);
            if(mPressedPoint->selectionEnabled()) {
                if(!mPressedPoint->isSelected()) {
                    if(!e.shiftMod()) clearPointsSelection();
                    addPointToSelection(mPressedPoint);
                }
                for(const auto& pt : mSelectedPoints_d) {
                    pt->canvasContextMenu(&menu);
                }
            } else {
                mPressedPoint->canvasContextMenu(&menu);
            }
            qMenu.exec(e.fGlobalPos);
        } else if(mPressedBox) {
            if(!mPressedBox->isSelected()) {
                if(!e.shiftMod()) clearBoxesSelection();
                addBoxToSelection(mPressedBox);
            }

            QMenu qMenu(e.fWidget);
            PropertyMenu menu(&qMenu, this, e.fWidget);
            for(const auto& box : mSelectedBoxes) {
                box->setupCanvasMenu(&menu);
            }
            qMenu.exec(e.fGlobalPos);
        } else {
            clearPointsSelection();
            clearBoxesSelection();
            QMenu menu(e.fWidget);
            addActionsToMenu(&menu);
            menu.exec(e.fGlobalPos);
        }
    }
    mDocument.actionFinished();
}

void Canvas::clearHoveredEdge() {
    mHoveredNormalSegment.reset();
}

void Canvas::handleMovePointMousePressEvent(const eMouseEvent& e) {
    if(mHoveredNormalSegment.isValid()) {
        if(e.ctrlMod()) {
            clearPointsSelection();
            mPressedPoint = mHoveredNormalSegment.divideAtAbsPos(e.fPos);
        } else {
            mCurrentNormalSegment = mHoveredNormalSegment;
            mCurrentNormalSegmentT = mCurrentNormalSegment.closestAbsT(e.fPos);
            clearPointsSelection();
            clearCurrentSmartEndPoint();
            clearLastPressedPoint();
        }
        clearHovered();
    } else if(mPressedPoint) {
        if(mPressedPoint->isSelected()) return;
        if(!e.shiftMod() && mPressedPoint->selectionEnabled()) {
            clearPointsSelection();
        }
        if(!mPressedPoint->selectionEnabled()) {
            addPointToSelection(mPressedPoint);
        }
    }
}


void Canvas::handleLeftButtonMousePress(const eMouseEvent& e) {
    if(e.fMouseGrabbing) {
        //handleMouseRelease(event->pos());
        //releaseMouseAndDontTrack();
        return;
    }

    mDoubleClick = false;
    //mMovesToSkip = 2;
    mStartTransform = true;
    mHasCreationPressPos = false;

    const qreal invScale = 1/e.fScale;
    const qreal invScaleUi = (qApp ? qApp->devicePixelRatio() : 1.0) * invScale;

    if (tryStartShearGizmo(e, invScaleUi)) {
        mPressedPoint = nullptr;
        return;
    }
    if (tryStartScaleGizmo(e, invScaleUi)) {
        mPressedPoint = nullptr;
        return;
    }
    if (tryStartAxisGizmo(e, invScaleUi)) {
        mPressedPoint = nullptr;
        return;
    }
    if (tryStartRotateWithGizmo(e, invScaleUi)) {
        mPressedPoint = nullptr;
        return;
    }

    mPressedPoint = getPointAtAbsPos(e.fPos, mCurrentMode, invScale);

    if(mRotPivot->isPointAtAbsPos(e.fPos, mCurrentMode, invScale)) {
        return mRotPivot->setSelected(true);
    }
    if(mCurrentMode == CanvasMode::boxTransform) {
        if(mHoveredPoint_d) {
            handleMovePointMousePressEvent(e);
        } else {
            handleMovePathMousePressEvent(e);
        }
    } else if(mCurrentMode == CanvasMode::pathCreate) {
        handleAddSmartPointMousePress(e);
    } else if(mCurrentMode == CanvasMode::pointTransform) {
        handleMovePointMousePressEvent(e);
    } else if(mCurrentMode == CanvasMode::drawPath) {
        const bool manual = mDocument.fDrawPathManual;
        bool start;
        if(manual) {
            start = mManualDrawPathState == ManualDrawPathState::none;
            if(mManualDrawPathState == ManualDrawPathState::drawn) {
                qreal dist;
                const int forceSplit = mDrawPath.nearestForceSplit(e.fPos, &dist);
                const int maxDist = 10;
                if(dist < maxDist) mDrawPath.removeForceSplit(forceSplit);
                else {
                    const int smoothPt = mDrawPath.nearestSmoothPt(e.fPos, &dist);
                    if(dist < maxDist) mDrawPath.addForceSplit(smoothPt);
                }
                mDrawPath.fit(DBL_MAX/5, false);
            }
        } else start = true;
        if(start) {
            mDrawPathFirst = getPointAtAbsPos(e.fPos, mCurrentMode, invScale);
            mDrawPathFit = 0;
            drawPathClear();
            mDrawPath.lineTo(e.fPos);
        }
    } else if (mCurrentMode == CanvasMode::pickFillStroke ||
               mCurrentMode == CanvasMode::pickFillStrokeEvent) {
        //mPressedBox = getBoxAtFromAllDescendents(e.fPos);
    } else if(mCurrentMode == CanvasMode::circleCreate) {
        const auto newPath = enve::make_shared<Circle>();
        newPath->planCenterPivotPosition();
        mCurrentContainer->addContained(newPath);
        const QPointF snappedPos = snapEventPos(e, false);
        newPath->setAbsolutePos(snappedPos);
        clearBoxesSelection();
        addBoxToSelection(newPath.get());

        mCurrentCircle = newPath.get();
        mCreationPressPos = snappedPos;
        mHasCreationPressPos = true;

    } else if(mCurrentMode == CanvasMode::nullCreate) {
        const auto newPath = enve::make_shared<NullObject>();
        newPath->planCenterPivotPosition();
        mCurrentContainer->addContained(newPath);
        newPath->setAbsolutePos(e.fPos);
        clearBoxesSelection();
        addBoxToSelection(newPath.get());
    } else if(mCurrentMode == CanvasMode::rectCreate) {
        const auto newPath = enve::make_shared<RectangleBox>();
        newPath->planCenterPivotPosition();
        mCurrentContainer->addContained(newPath);
        const QPointF snappedPos = snapEventPos(e, false);
        newPath->setAbsolutePos(snappedPos);
        clearBoxesSelection();
        addBoxToSelection(newPath.get());

        mCurrentRectangle = newPath.get();
        mCreationPressPos = snappedPos;
        mHasCreationPressPos = true;
    } else if (mCurrentMode == CanvasMode::textCreate) {
        if (enve_cast<TextBox*>(mHoveredBox)) {
            setCurrentBox(mHoveredBox);
            emit openTextEditor();
        } else {
            const auto newPath = enve::make_shared<TextBox>();
            newPath->planCenterPivotPosition();
            newPath->setFontFamilyAndStyle(mDocument.fFontFamily,
                                           mDocument.fFontStyle);
            newPath->setFontSize(mDocument.fFontSize);
            mCurrentContainer->addContained(newPath);
            newPath->setAbsolutePos(e.fPos);

            mCurrentTextBox = newPath.get();

            clearBoxesSelection();
            addBoxToSelection(newPath.get());
        }
    }
}

void Canvas::cancelCurrentTransform()
{
    mGizmos.fState.rotatingFromHandle = false;

    if(mCurrentMode == CanvasMode::pointTransform) {
        if(mCurrentNormalSegment.isValid()) {
            mCurrentNormalSegment.cancelPassThroughTransform();
        } else {
            cancelSelectedPointsTransform();
        }
    } else if(mCurrentMode == CanvasMode::boxTransform) {
        if(mRotPivot->isSelected()) {
            mRotPivot->cancelTransform();
        } else {
            cancelSelectedBoxesTransform();
        }
    } else if(mCurrentMode == CanvasMode::pathCreate) {

    } else if (mCurrentMode == CanvasMode::pickFillStroke ||
               mCurrentMode == CanvasMode::pickFillStrokeEvent) {
        //mCanvasWindow->setCanvasMode(MOVE_PATH);
    }
    mValueInput.clearAndDisableInput();
    mTransMode = TransformMode::none;
    cancelCurrentTransformGimzos();
}

void Canvas::handleMovePointMouseRelease(const eMouseEvent &e) {
    if(mRotPivot->isSelected()) {
        mRotPivot->setSelected(false);
    } else if(mTransMode == TransformMode::rotate ||
              mTransMode == TransformMode::scale ||
              mTransMode == TransformMode::shear) {
        finishSelectedPointsTransform();
        mTransMode = TransformMode::none;
    } else if(mSelecting) {
        mSelecting = false;
        if(!e.shiftMod()) clearPointsSelection();
        moveSecondSelectionPoint(e.fPos);
        selectAndAddContainedPointsToSelection(mSelectionRect);
    } else if(mStartTransform) {
        if(mPressedPoint) {
            if(mPressedPoint->isCtrlPoint()) {
                removePointFromSelection(mPressedPoint);
            } else if(e.shiftMod()) {
                if(mPressedPoint->isSelected()) {
                    removePointFromSelection(mPressedPoint);
                } else {
                    addPointToSelection(mPressedPoint);
                }
            } else {
                selectOnlyLastPressedPoint();
            }
        } else {
            mPressedBox = mCurrentContainer->getBoxAt(e.fPos);
            if(mPressedBox ? !!enve_cast<ContainerBox*>(mPressedBox) : true) {
                const auto pressedBox = getBoxAtFromAllDescendents(e.fPos);
                if(!pressedBox) {
                    if(!e.shiftMod()) {
                        clearPointsSelectionOrDeselect();
                    }
                } else {
                    clearPointsSelection();
                    clearCurrentSmartEndPoint();
                    clearLastPressedPoint();
                    setCurrentBoxesGroup(pressedBox->getParentGroup());
                    addBoxToSelection(pressedBox);
                    mPressedBox = pressedBox;
                }
            }
            if(mPressedBox) {
                if(e.shiftMod()) {
                    if(mPressedBox->isSelected()) {
                        removeBoxFromSelection(mPressedBox);
                    } else {
                        addBoxToSelection(mPressedBox);
                    }
                } else {
                    clearPointsSelection();
                    clearCurrentSmartEndPoint();
                    clearLastPressedPoint();
                    selectOnlyLastPressedBox();
                }
            }
        }
    } else {
        finishSelectedPointsTransform();
        if(mPressedPoint) {
            if(!mPressedPoint->selectionEnabled()) {
                removePointFromSelection(mPressedPoint);
            }
        }
    }
}

void Canvas::handleMovePathMouseRelease(const eMouseEvent &e) {
    if(mRotPivot->isSelected()) {
        if(!mStartTransform) mRotPivot->finishTransform();
        mRotPivot->setSelected(false);
    } else if(mTransMode == TransformMode::rotate) {
        pushUndoRedoName("Rotate Objects");
        finishSelectedBoxesTransform();
    } else if(mTransMode == TransformMode::scale) {
        pushUndoRedoName("Scale Objects");
        finishSelectedBoxesTransform();
    } else if(mTransMode == TransformMode::shear) {
        pushUndoRedoName("Shear Objects");
        finishSelectedBoxesTransform();
    } else if(mStartTransform) {
        mSelecting = false;
        if(e.shiftMod() && mPressedBox) {
            if(mPressedBox->isSelected()) {
                removeBoxFromSelection(mPressedBox);
            } else {
                addBoxToSelection(mPressedBox);
            }
        } else {
            selectOnlyLastPressedBox();
        }
    } else if(mSelecting) {
        moveSecondSelectionPoint(e.fPos);
        mCurrentContainer->addContainedBoxesToSelection(mSelectionRect);
        mSelecting = false;
    } else {
        pushUndoRedoName("Move Objects");
        finishSelectedBoxesTransform();
    }
}

SmartNodePoint* drawPathAppend(const QList<qCubicSegment2D>& fitted,
                               SmartNodePoint* endPoint) {
    for(int i = 0; i < fitted.count(); i++) {
        const auto& seg = fitted.at(i);
        endPoint->moveC2ToAbsPos(seg.c1());
        endPoint = endPoint->actionAddPointAbsPos(seg.p3());
        endPoint->moveC0ToAbsPos(seg.c2());
    }
    return endPoint;
}

qsptr<SmartVectorPath> drawPathNew(QList<qCubicSegment2D>& fitted) {
    const QPointF& begin = fitted.first().p0();
    const QPointF& end = fitted.last().p3();
    const qreal beginEndDist = pointToLen(end - begin);
    const bool close = beginEndDist < 7 && fitted.count() > 1;
    if(close) fitted.last().setP3(begin);
    const auto newPath = enve::make_shared<SmartVectorPath>();
    CubicList fittedList(fitted);
    newPath->loadSkPath(fittedList.toSkPath());
    newPath->planCenterPivotPosition();
    return newPath;
}

void Canvas::drawPathClear() {
    mManualDrawPathState = ManualDrawPathState::none;
    mDrawPathFirst.clear();
    mDrawPath.clear();
    mDrawPathTmp.reset();
}

void Canvas::drawPathFinish(const qreal invScale) {
    mDrawPath.smooth(mDocument.fDrawPathSmooth);
    const bool manual = mDocument.fDrawPathManual;
    const qreal error = manual ? DBL_MAX/5 :
                                 mDocument.fDrawPathMaxError;
    mDrawPath.fit(error, !manual);

    auto& fitted = mDrawPath.getFitted();
    if(!fitted.isEmpty()) {
        const QPointF& begin = fitted.first().p0();
        const QPointF& end = fitted.last().p3();
        const auto beginHover = getPointAtAbsPos(begin, mCurrentMode, invScale);
        const auto beginNode = enve_cast<SmartNodePoint*>(beginHover);
        const auto endHover = getPointAtAbsPos(end, mCurrentMode, invScale);
        const auto endNode = enve_cast<SmartNodePoint*>(endHover);
        const bool beginEndPoint = beginNode ? beginNode->isEndPoint() : false;
        const bool endEndPoint = endNode ? endNode->isEndPoint() : false;
        bool createNew = false;

        if(beginNode && endNode && beginNode != endNode) {
            const auto beginParent = beginNode->getTargetAnimator();
            const auto endParent = endNode->getTargetAnimator();
            const bool sampeParent = beginParent == endParent;

            if(sampeParent) {
                const auto transform = beginNode->getTransform();
                const auto matrix = transform->getTotalTransform();
                const auto invMatrix = matrix.inverted();
                std::for_each(fitted.begin(), fitted.end(),
                              [&invMatrix](qCubicSegment2D& seg) {
                    seg.transform(invMatrix);
                });
                const int beginId = beginNode->getNodeId();
                const int endId = endNode->getNodeId();
                beginParent->actionReplaceSegments(beginId, endId, fitted);
            } else if(beginEndPoint && endEndPoint) {
                const bool reverse = endNode->hasNextPoint();

                const auto orderedBegin = reverse ? endNode : beginNode;
                const auto orderedEnd = reverse ? beginNode : endNode;

                if(orderedEnd->hasNextPoint() || !endNode->hasNextPoint()) {
                    std::reverse(fitted.begin(), fitted.end());
                    std::for_each(fitted.begin(), fitted.end(),
                                  [](qCubicSegment2D& seg) { seg.reverse(); });
                }

                const auto& lastSeg = fitted.last();
                const auto mid = fitted.mid(0, fitted.count() - 1);
                const auto last = drawPathAppend(mid, orderedEnd);
                last->moveC2ToAbsPos(lastSeg.c1());
                orderedBegin->moveC0ToAbsPos(lastSeg.c2());
                last->actionConnectToNormalPoint(orderedBegin);
            } else createNew = true;
        } else if(beginNode && beginEndPoint) {
            drawPathAppend(fitted, beginNode);
        } else if(endNode && endEndPoint) {
            drawPathAppend(fitted, endNode);
        } else createNew = true;
        if(createNew) {
            const auto matrix = mCurrentContainer->getTotalTransform();
            const auto invMatrix = matrix.inverted();
            std::for_each(fitted.begin(), fitted.end(),
                          [&invMatrix](qCubicSegment2D& seg) {
                seg.transform(invMatrix);
            });
            const auto newPath = drawPathNew(fitted);
            mCurrentContainer->addContained(newPath);
            clearBoxesSelection();
            addBoxToSelection(newPath.get());
        }
    }

    drawPathClear();
}

const QColor Canvas::pickPixelColor(const QPoint &pos)
{
    // try the "safe" option first
    if (QApplication::activeWindow()) {
        const auto nPos = QApplication::activeWindow()->mapFromGlobal(pos);
        return QApplication::activeWindow()->grab(QRect(QPoint(nPos.x(), nPos.y()),
                                                        QSize(1, 1))).toImage().pixel(0, 0);
    }

    // "insecure" fallback (will not work in a sandbox or wayland)
    // will prompt for permissions on macOS
    // Windows and X11 don't care
    QScreen *screen = QApplication::screenAt(pos);
    if (!screen) { return QColor(); }
    WId wid = QApplication::desktop()->winId();
    const auto pix = screen->grabWindow(wid, pos.x(), pos.y(), 1, 1);
    return QColor(pix.toImage().pixel(0, 0));
}

void Canvas::applyPixelColor(const QColor &color,
                             const bool &fill)
{
    if (!color.isValid()) { return; }
    for (const auto& box : mSelectedBoxes) {
        if (fill) {
            auto settings = box->getFillSettings();
            if (settings) {
                if (settings->getPaintType() == PaintType::NOPAINT) {
                    settings->setPaintType(PaintType::FLATPAINT);
                }
                settings->setCurrentColor(color, true);
                box->fillStrokeSettingsChanged();
            }
        } else {
            auto settings = box->getStrokeSettings();
            if (settings) {
                if (settings->getPaintType() == PaintType::NOPAINT) {
                    settings->setPaintType(PaintType::FLATPAINT);
                }
                settings->setCurrentColor(color, true);
                box->fillStrokeSettingsChanged();
            }
        }
    }
}

void Canvas::handleLeftMouseRelease(const eMouseEvent &e)
{
    if (e.fMouseGrabbing) { e.fReleaseMouse(); }

    handleLeftMouseGizmos();

    if(mCurrentNormalSegment.isValid()) {
        if(!mStartTransform) mCurrentNormalSegment.finishPassThroughTransform();
        mHoveredNormalSegment = mCurrentNormalSegment;
        mHoveredNormalSegment.generateSkPath();
        mCurrentNormalSegment.reset();
        return;
    }
    if(mDoubleClick) return;
    if(mCurrentMode == CanvasMode::pointTransform) {
        handleMovePointMouseRelease(e);
    } else if(mCurrentMode == CanvasMode::boxTransform) {
        if(!mPressedPoint) {
            handleMovePathMouseRelease(e);
        } else {
            handleMovePointMouseRelease(e);
            clearPointsSelection();
        }
    } else if(mCurrentMode == CanvasMode::pathCreate) {
        handleAddSmartPointMouseRelease(e);
    } else if(mCurrentMode == CanvasMode::drawPath) {
        const bool manual = mDocument.fDrawPathManual;
        if(manual) {
            mManualDrawPathState = ManualDrawPathState::drawn;
        } else {
            drawPathFinish(1/e.fScale);
        }
    } else if (mCurrentMode == CanvasMode::pickFillStrokeEvent) {
        emit currentPickedColor(pickPixelColor(e.fGlobalPos));
    }
    mValueInput.clearAndDisableInput();
    mTransMode = TransformMode::none;
}

QPointF Canvas::getMoveByValueForEvent(const eMouseEvent &e) {
    if(mValueInput.inputEnabled())
        return mValueInput.getPtValue();
    const QPointF moveByPoint = e.fPos - e.fLastPressPos;
    mValueInput.setDisplayedValue(moveByPoint);
    if(mValueInput.yOnlyMode()) return {0, moveByPoint.y()};
    else if(mValueInput.xOnlyMode()) return {moveByPoint.x(), 0};
    return moveByPoint;
}

#include <QApplication>
#include "MovablePoints/smartctrlpoint.h"
#include "MovablePoints/pathpointshandler.h"
#include "Boxes/smartvectorpath.h"
void Canvas::handleMovePointMouseMove(const eMouseEvent &e) {
    if(mRotPivot->isSelected()) {
        if(mStartTransform) mRotPivot->startTransform();
        mRotPivot->moveByAbs(getMoveByValueForEvent(e));
    } else if(mTransMode == TransformMode::scale) {
        scaleSelected(e);
    } else if(mTransMode == TransformMode::shear) {
        shearSelected(e);
    } else if(mTransMode == TransformMode::rotate) {
        rotateSelected(e);
    } else if(mCurrentNormalSegment.isValid()) {
        if(mStartTransform) mCurrentNormalSegment.startPassThroughTransform();
        mCurrentNormalSegment.makePassThroughAbs(e.fPos, mCurrentNormalSegmentT);
    } else {
        const auto& gridSettings = mDocument.gridController().settings;
        const bool snappingActive = mDocument.isSnappingActive();
        const bool pivotsSnapEnabled = snappingActive && gridSettings.snapToPivots;
        const bool boxesSnapEnabled = snappingActive && gridSettings.snapToBoxes;
        const bool nodesSnapEnabled = snappingActive && gridSettings.snapToNodes;
        std::vector<QPointF> pivotTargets;
        std::vector<QPointF> boxTargets;
        std::vector<QPointF> nodeTargets;
        const bool includeSelectedBounds =
            boxesSnapEnabled && mPressedPoint && mPressedPoint->isPivotPoint();
        if (snappingActive && (pivotsSnapEnabled || boxesSnapEnabled || nodesSnapEnabled)) {
            collectSnapTargets(pivotsSnapEnabled, boxesSnapEnabled, nodesSnapEnabled,
                               pivotTargets, boxTargets, nodeTargets,
                               includeSelectedBounds);
        }
        const bool hasPivotTargets = pivotsSnapEnabled && !pivotTargets.empty();
        const bool hasBoxTargets = boxesSnapEnabled && !boxTargets.empty();
        const bool hasNodeTargets = nodesSnapEnabled && !nodeTargets.empty();
        const bool snapSourcesAvailable = snappingActive && (gridSettings.enabled ||
                                          gridSettings.snapToCanvas ||
                                          hasPivotTargets || hasBoxTargets || hasNodeTargets);

        if(mPressedPoint) {
            addPointToSelection(mPressedPoint);
            const auto mods = QGuiApplication::queryKeyboardModifiers();
            if(mPressedPoint->isSmartNodePoint()) {
                if(mods & Qt::ControlModifier) {
                    const auto nodePt = static_cast<SmartNodePoint*>(mPressedPoint.data());
                    if(nodePt->isDissolved()) {
                        const int selId = nodePt->moveToClosestSegment(e.fPos);
                        const auto handler = nodePt->getHandler();
                        const auto dissPt = handler->getPointWithId<SmartNodePoint>(selId);
                        if(nodePt->getNodeId() != selId) {
                            removePointFromSelection(nodePt);
                            addPointToSelection(dissPt);
                        }
                        mPressedPoint = dissPt;
                        return;
                    }
                } else if(mods & Qt::ShiftModifier) {
                    const auto nodePt = static_cast<SmartNodePoint*>(mPressedPoint.data());
                    const auto nodePtAnim = nodePt->getTargetAnimator();
                    if(nodePt->isNormal()) {
                        SmartNodePoint* closestNode = nullptr;
                        qreal minDist = 10/e.fScale;
                        for(const auto& sBox : mSelectedBoxes) {
                            if(!enve_cast<SmartVectorPath*>(sBox)) continue;
                            const auto sPatBox = static_cast<SmartVectorPath*>(sBox);
                            const auto sAnim = sPatBox->getPathAnimator();
                            for(int i = 0; i < sAnim->ca_getNumberOfChildren(); i++) {
                                const auto sPath = sAnim->getChild(i);
                                if(sPath == nodePtAnim) continue;
                                const auto sHandler = static_cast<PathPointsHandler*>(sPath->getPointsHandler());
                                const auto node = sHandler->getClosestNode(e.fPos, minDist);
                                if(node) {
                                    closestNode = node;
                                    minDist = pointToLen(closestNode->getAbsolutePos() - e.fPos);
                                }
                            }
                        }
                        if(closestNode) {
                            const bool reverse = mods & Qt::ALT;

                            const auto sC0 = reverse ? closestNode->getC2Pt() : closestNode->getC0Pt();
                            const auto sC2 = reverse ? closestNode->getC0Pt() : closestNode->getC2Pt();

                            nodePt->setCtrlsMode(closestNode->getCtrlsMode());
                            nodePt->setC0Enabled(sC0->enabled());
                            nodePt->setC2Enabled(sC2->enabled());
                            nodePt->setAbsolutePos(closestNode->getAbsolutePos());
                            nodePt->getC0Pt()->setAbsolutePos(sC0->getAbsolutePos());
                            nodePt->getC2Pt()->setAbsolutePos(sC2->getAbsolutePos());
                        } else {
                            if(mStartTransform) mPressedPoint->startTransform();
                            mPressedPoint->moveByAbs(getMoveByValueForEvent(e));
                        }
                        return;
                    }
                }
            }

            if(!mPressedPoint->selectionEnabled()) {
                if(mStartTransform) {
                    mPressedPoint->startTransform();
                    mGridMoveStartPivot = mPressedPoint->getAbsolutePos();
                }

                QPointF moveBy = getMoveByValueForEvent(e);
                const bool bypassSnap = e.fModifiers & Qt::AltModifier;
                const bool forceSnap = e.fModifiers & Qt::ControlModifier;

                if(snappingActive && mHasWorldToScreen &&
                   (snapSourcesAvailable || forceSnap)) {
                    const QPointF targetPos = mGridMoveStartPivot + moveBy;
                    QRectF canvasRect;
                    const QRectF* canvasPtr = nullptr;
                    if (gridSettings.snapToCanvas) {
                        canvasRect = QRectF(QPointF(0.0, 0.0), QSizeF(mWidth, mHeight));
                        canvasPtr = &canvasRect;
                    }
                    const auto snapped = mDocument.gridController().maybeSnapPivot(
                        targetPos,
                        mWorldToScreen,
                        forceSnap,
                        bypassSnap,
                        canvasPtr,
                        nullptr,
                        hasPivotTargets ? &pivotTargets : nullptr,
                        hasBoxTargets ? &boxTargets : nullptr,
                        hasNodeTargets ? &nodeTargets : nullptr);
                    if(snapped != targetPos) {
                        moveBy = snapped - mGridMoveStartPivot;
                    }
                }

                mPressedPoint->moveByAbs(moveBy);
                return;
            }
        }

        if(mStartTransform && !mSelectedPoints_d.isEmpty()) {
            mGridMoveStartPivot = getSelectedPointsAbsPivotPos();
        }

        QPointF moveBy = getMoveByValueForEvent(e);
        const bool bypassSnap = e.fModifiers & Qt::AltModifier;
        const bool forceSnap = e.fModifiers & Qt::ControlModifier;

        if(!mSelectedPoints_d.isEmpty() && snappingActive && mHasWorldToScreen &&
           (snapSourcesAvailable || forceSnap)) {
            const QPointF targetPivot = mGridMoveStartPivot + moveBy;
            QRectF canvasRect;
            const QRectF* canvasPtr = nullptr;
            if (gridSettings.snapToCanvas) {
                canvasRect = QRectF(QPointF(0.0, 0.0), QSizeF(mWidth, mHeight));
                canvasPtr = &canvasRect;
            }
            const auto snapped = mDocument.gridController().maybeSnapPivot(
                targetPivot,
                mWorldToScreen,
                forceSnap,
                bypassSnap,
                canvasPtr,
                nullptr,
                hasPivotTargets ? &pivotTargets : nullptr,
                hasBoxTargets ? &boxTargets : nullptr,
                hasNodeTargets ? &nodeTargets : nullptr);
            if(snapped != targetPivot) {
                moveBy = snapped - mGridMoveStartPivot;
            }
        }

        moveSelectedPointsByAbs(moveBy, mStartTransform);
    }
}

void Canvas::scaleSelected(const eMouseEvent& e) {
    const QPointF absPos = mRotPivot->getAbsolutePos();
    const QPointF distMoved = e.fPos - e.fLastPressPos;

    qreal scaleBy;
    if(mValueInput.inputEnabled()) {
        scaleBy = mValueInput.getValue();
    } else {
        scaleBy = 1 + distSign({distMoved.x(), -distMoved.y()})*0.003;
    }
    qreal scaleX;
    qreal scaleY;
    if(mValueInput.xOnlyMode()) {
        scaleX = scaleBy;
        scaleY = 1;
    } else if(mValueInput.yOnlyMode()) {
        scaleX = 1;
        scaleY = scaleBy;
    } else {
        scaleX = scaleBy;
        scaleY = scaleBy;
    }

    if(mCurrentMode == CanvasMode::boxTransform) {
        scaleSelectedBy(scaleX, scaleY, absPos, mStartTransform);
    } else {
        scaleSelectedPointsBy(scaleX, scaleY, absPos, mStartTransform);
    }

    if(!mValueInput.inputEnabled())
        mValueInput.setDisplayedValue({scaleX, scaleY});
    mRotPivot->setMousePos(e.fPos);
}

void Canvas::shearSelected(const eMouseEvent& e)
{
    const QPointF absPos = mRotPivot->getAbsolutePos();
    const QPointF distMoved = e.fPos - e.fLastPressPos;

    qreal shearBy;
    if (mValueInput.inputEnabled()) {
        shearBy = mValueInput.getValue();
    } else {
        qreal axisDelta;
        if (mValueInput.xOnlyMode()) {
            axisDelta = -distMoved.x();
        } else {
            axisDelta = distMoved.y();
        }
        shearBy = axisDelta * 0.01;
    }
    qreal shearX = 0;
    qreal shearY = 0;
    if (mValueInput.xOnlyMode()) {
        shearX = shearBy;
    } else if (mValueInput.yOnlyMode()) {
        shearY = shearBy;
    } else {
        shearX = shearBy;
        shearY = shearBy;
    }

    if (mCurrentMode == CanvasMode::boxTransform) {
        shearSelectedBy(shearX, shearY, absPos, mStartTransform);
    } else {
        shearSelectedPointsBy(shearX, shearY, absPos, mStartTransform);
    }

    if (!mValueInput.inputEnabled()) {
        mValueInput.setDisplayedValue({shearX, shearY});
    }
    mRotPivot->setMousePos(e.fPos);
}

void Canvas::rotateSelected(const eMouseEvent& e) {
    const QPointF absPos = mRotPivot->getAbsolutePos();
    qreal rot;
    if(mValueInput.inputEnabled()) {
        rot = mValueInput.getValue();
    } else {
        const QLineF dest_line(absPos, e.fPos);
        const QLineF prev_line(absPos, e.fLastPressPos);
        qreal d_rot = dest_line.angleTo(prev_line);
        if(d_rot > 180) d_rot -= 360;

        if(mLastDRot - d_rot > 90) {
            mRotHalfCycles += 2;
        } else if(mLastDRot - d_rot < -90) {
            mRotHalfCycles -= 2;
        }
        mLastDRot = d_rot;
        rot = d_rot + mRotHalfCycles*180;
    }

    if(mCurrentMode == CanvasMode::boxTransform) {
        rotateSelectedBy(rot, absPos, mStartTransform);
    } else {
        rotateSelectedPointsBy(rot, absPos, mStartTransform);
    }

    if(!mValueInput.inputEnabled())
        mValueInput.setDisplayedValue(rot);
    mRotPivot->setMousePos(e.fPos);
}

bool Canvas::prepareRotation(const QPointF &startPos,
                             bool fromHandle)
{
    if (mCurrentMode != CanvasMode::boxTransform &&
        mCurrentMode != CanvasMode::pointTransform) { return false; }
    if (mSelectedBoxes.isEmpty()) { return false; }
    if (mCurrentMode == CanvasMode::pointTransform) {
        if (mSelectedPoints_d.isEmpty()) { return false; }
    }

    mGizmos.fState.rotatingFromHandle = fromHandle;
    mValueInput.clearAndDisableInput();
    mValueInput.setupRotate();

    if (fromHandle) { setGizmosSuppressed(true); }

    mRotPivot->setMousePos(startPos);
    mTransMode = TransformMode::rotate;
    mRotHalfCycles = 0;
    mLastDRot = 0;

    mDoubleClick = false;
    mStartTransform = true;
    return true;
}

void Canvas::handleMovePathMouseMove(const eMouseEvent& e) {
    if(mRotPivot->isSelected()) {
        if(mStartTransform) mRotPivot->startTransform();
        mRotPivot->moveByAbs(getMoveByValueForEvent(e));
    } else if(mTransMode == TransformMode::scale) {
        scaleSelected(e);
    } else if(mTransMode == TransformMode::shear) {
        shearSelected(e);
    } else if(mTransMode == TransformMode::rotate) {
        rotateSelected(e);
    } else {
        if(mPressedBox) {
            addBoxToSelection(mPressedBox);
            mPressedBox = nullptr;
        }

        const auto& gridSettings = mDocument.gridController().settings;

        if (mStartTransform && !mSelectedBoxes.isEmpty()) {
            mGridMoveStartPivot = getSelectedBoxesAbsPivotPos();

            mGridSnapAnchorOffsets.clear();
            if (gridSettings.snapAnchorPivot) {
                mGridSnapAnchorOffsets.emplace_back(QPointF(0.0, 0.0));
            }

            QRectF combinedRect;
            bool hasRect = false;
            for (const auto& box : mSelectedBoxes) {
                const QRectF rect = box->getAbsBoundingRect();
                if (rect.width() < 0.0 || rect.height() < 0.0) {
                    continue;
                }
                if (!hasRect) {
                    combinedRect = rect;
                    hasRect = true;
                } else {
                    combinedRect = combinedRect.united(rect);
                }
            }

            if (hasRect && gridSettings.snapAnchorBounds) {
                const QPointF topLeft = combinedRect.topLeft();
                const QPointF topRight = combinedRect.topRight();
                const QPointF bottomLeft = combinedRect.bottomLeft();
                const QPointF bottomRight = combinedRect.bottomRight();
                const QPointF topCenter((topLeft.x() + topRight.x()) * 0.5, topLeft.y());
                const QPointF bottomCenter((bottomLeft.x() + bottomRight.x()) * 0.5, bottomLeft.y());
                const QPointF leftCenter(topLeft.x(), (topLeft.y() + bottomLeft.y()) * 0.5);
                const QPointF rightCenter(topRight.x(), (topRight.y() + bottomRight.y()) * 0.5);
                const QPointF center = combinedRect.center();

                mGridSnapAnchorOffsets.emplace_back(topLeft - mGridMoveStartPivot);
                mGridSnapAnchorOffsets.emplace_back(topRight - mGridMoveStartPivot);
                mGridSnapAnchorOffsets.emplace_back(bottomLeft - mGridMoveStartPivot);
                mGridSnapAnchorOffsets.emplace_back(bottomRight - mGridMoveStartPivot);
                mGridSnapAnchorOffsets.emplace_back(topCenter - mGridMoveStartPivot);
                mGridSnapAnchorOffsets.emplace_back(bottomCenter - mGridMoveStartPivot);
                mGridSnapAnchorOffsets.emplace_back(leftCenter - mGridMoveStartPivot);
                mGridSnapAnchorOffsets.emplace_back(rightCenter - mGridMoveStartPivot);
                mGridSnapAnchorOffsets.emplace_back(center - mGridMoveStartPivot);
            }

            if (gridSettings.snapAnchorNodes) {
                const MovablePoint::PtOp gatherOffsets = [&](MovablePoint* point) {
                    if (!point || !point->isSmartNodePoint()) { return; }
                    const auto* node = static_cast<SmartNodePoint*>(point);
                    mGridSnapAnchorOffsets.emplace_back(node->getAbsolutePos() - mGridMoveStartPivot);
                };
                for (const auto& box : mSelectedBoxes) {
                    if (!box) { continue; }
                    auto* mutableBox = const_cast<BoundingBox*>(box);
                    mutableBox->selectAllCanvasPts(gatherOffsets, CanvasMode::pointTransform);
                }
            }
        }

        auto moveBy = getMoveByValueForEvent(e);
        const bool bypassSnap = e.fModifiers & Qt::AltModifier;
        const bool forceSnap = e.fModifiers & Qt::ControlModifier;
        const bool hasAnchorOffsets = !mGridSnapAnchorOffsets.empty();
        const bool snappingActive = mDocument.isSnappingActive();
        const bool pivotsSnapEnabled = snappingActive && gridSettings.snapToPivots;
        const bool boxesSnapEnabled = snappingActive && gridSettings.snapToBoxes;
        const bool nodesSnapEnabled = snappingActive && gridSettings.snapToNodes;
        std::vector<QPointF> pivotTargets;
        std::vector<QPointF> boxTargets;
        std::vector<QPointF> nodeTargets;
        if (snappingActive && (pivotsSnapEnabled || boxesSnapEnabled || nodesSnapEnabled)) {
            collectSnapTargets(pivotsSnapEnabled, boxesSnapEnabled, nodesSnapEnabled,
                               pivotTargets, boxTargets, nodeTargets);
        }
        const bool hasPivotTargets = pivotsSnapEnabled && !pivotTargets.empty();
        const bool hasBoxTargets = boxesSnapEnabled && !boxTargets.empty();
        const bool hasNodeTargets = nodesSnapEnabled && !nodeTargets.empty();
        const bool snapSourcesAvailable = snappingActive && (gridSettings.enabled ||
                                          gridSettings.snapToCanvas ||
                                          hasPivotTargets || hasBoxTargets || hasNodeTargets ||
                                          hasAnchorOffsets);
        if (!mSelectedBoxes.isEmpty() && snappingActive && mHasWorldToScreen &&
            (snapSourcesAvailable || forceSnap)) {
            const QPointF targetPivot = mGridMoveStartPivot + moveBy;
            QRectF canvasRect;
            const QRectF* canvasPtr = nullptr;
            if (gridSettings.snapToCanvas) {
                canvasRect = QRectF(QPointF(0.0, 0.0), QSizeF(mWidth, mHeight));
                canvasPtr = &canvasRect;
            }
            const auto snapped = mDocument.gridController().maybeSnapPivot(targetPivot,
                                                                           mWorldToScreen,
                                                                           forceSnap,
                                                                           bypassSnap,
                                                                           canvasPtr,
                                                                           &mGridSnapAnchorOffsets,
                                                                           hasPivotTargets ? &pivotTargets : nullptr,
                                                                           hasBoxTargets ? &boxTargets : nullptr,
                                                                           hasNodeTargets ? &nodeTargets : nullptr);
            if (snapped != targetPivot) {
                moveBy = snapped - mGridMoveStartPivot;
            }
        }

        moveSelectedBoxesByAbs(moveBy, mStartTransform);
    }
}

void Canvas::updateTransformation(const eKeyEvent &e) {
    if(mSelecting) {
        moveSecondSelectionPoint(e.fPos);
    } else if(mCurrentMode == CanvasMode::pointTransform) {
        handleMovePointMouseMove(e);
    } else if(mCurrentMode == CanvasMode::boxTransform) {
        if(!mPressedPoint) {
            handleMovePathMouseMove(e);
        } else {
            handleMovePointMouseMove(e);
        }
    } else if(mCurrentMode == CanvasMode::pathCreate) {
        handleAddSmartPointMouseMove(e);
    }
}
