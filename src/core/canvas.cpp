/*
# enve2d - https://github.com/enve2d
#
# Copyright (c) enve2d developers
# Copyright (C) 2016-2020 Maurycy Liebner
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
*/

#include "canvas.h"
#include <QPainter>
#include <QMouseEvent>
#include <QLineF>
#include <QtMath>
#include <cmath>
#include <QDebug>
#include <QApplication>
#include "Boxes/videobox.h"
#include "MovablePoints/pathpivot.h"
#include "Boxes/imagebox.h"
#include "Sound/soundcomposition.h"
#include "Boxes/textbox.h"
#include "GUI/global.h"
#include "appsupport.h"
#include "pointhelpers.h"
#include "Boxes/internallinkbox.h"
#include "clipboardcontainer.h"
//#include "Boxes/paintbox.h"
#include <QFile>
#include "MovablePoints/smartnodepoint.h"
#include "Boxes/internallinkcanvas.h"
#include "pointtypemenu.h"
#include "Animators/transformanimator.h"
#include "glhelpers.h"
#include "Private/document.h"
#include "svgexporter.h"
#include "ReadWrite/evformat.h"
#include "eevent.h"
#include "Boxes/nullobject.h"
#include "simpletask.h"
#include "themesupport.h"

namespace {
constexpr qreal kRotateGizmoSweepDeg = 90.0; // default sweep of gizmo arc
constexpr qreal kRotateGizmoBaseOffsetDeg = 270.0; // default angular offset for gizmo arc
constexpr qreal kRotateGizmoRadiusPx = 30.0; // gizmo radius in screen pixels
constexpr qreal kRotateGizmoStrokePx = 6.0; // arc stroke thickness in screen pixels
constexpr qreal kRotateGizmoHitWidthPx = kRotateGizmoStrokePx; // hit area thickness in screen pixels
constexpr qreal kAxisGizmoWidthPx = 5.0; // axis gizmo rectangle width in screen pixels
constexpr qreal kAxisGizmoHeightPx = 60.0; // axis gizmo rectangle height in screen pixels
constexpr qreal kAxisGizmoYOffsetPx = 40.0; // vertical distance of Y gizmo from pivot in pixels
constexpr qreal kAxisGizmoXOffsetPx = 40.0; // horizontal distance of X gizmo from pivot in pixels
constexpr qreal kScaleGizmoSizePx = 12.0; // scale gizmo square size in screen pixels
constexpr qreal kScaleGizmoGapPx = 4.0; // gap between position gizmos and scale gizmos in screen pixels
constexpr qreal kShearGizmoRadiusPx = 6.0; // shear gizmo circle radius in screen pixels
constexpr qreal kShearGizmoGapPx = 4.0; // gap between scale and shear gizmos in screen pixels
}

Canvas::Canvas(Document &document,
               const int canvasWidth,
               const int canvasHeight,
               const int frameCount,
               const qreal fps)
    : mDocument(document)
    //, mPaintTarget(this)
{
    SceneParentSelfAssign(this);
    connect(&mDocument, &Document::canvasModeSet,
            this, &Canvas::setCanvasMode);
    std::function<bool(int)> changeFrameFunc =
    [this](const int undoRedoFrame) {
        if (mDocument.fActiveScene != this) { return false; }
        if (undoRedoFrame != anim_getCurrentAbsFrame()) {
            mDocument.setActiveSceneFrame(undoRedoFrame);
            return true;
        }
        return false;
    };
    mUndoRedoStack = enve::make_shared<UndoRedoStack>(changeFrameFunc);
    mFps = fps;

    mBackgroundColor->setColor(QColor(75, 75, 75));
    ca_addChild(mBackgroundColor);
    mSoundComposition = qsptr<SoundComposition>::create(this);

    mRange = {0, frameCount};

    mWidth = canvasWidth;
    mHeight = canvasHeight;

    mCurrentContainer = this;
    setIsCurrentGroup_k(true);

    mRotPivot = enve::make_shared<PathPivot>(this);

    mTransformAnimator->SWT_hide();

    //anim_setAbsFrame(0);

    //setCanvasMode(MOVE_PATH);
}

Canvas::~Canvas()
{
    clearPointsSelection();
    clearBoxesSelection();
}

qreal Canvas::getResolution() const
{
    return mResolution;
}

void Canvas::setResolution(const qreal percent)
{
    mResolution = percent;
    prp_afterWholeInfluenceRangeChanged();
    updateAllBoxes(UpdateReason::userChange);
}

void Canvas::setCurrentGroupParentAsCurrentGroup()
{
    setCurrentBoxesGroup(mCurrentContainer->getParentGroup());
}

void Canvas::queTasks()
{
    if (Actions::sInstance->smoothChange() && mCurrentContainer) {
        if (!mDrawnSinceQue) { return; }
        mCurrentContainer->queChildrenTasks();
    } else ContainerBox::queTasks();
    mDrawnSinceQue = false;
}

void Canvas::addSelectedForGraph(const int widgetId,
                                 GraphAnimator* const anim)
{
    const auto it = mSelectedForGraph.find(widgetId);
    if (it == mSelectedForGraph.end()) {
        const auto list = std::make_shared<ConnContextObjList<GraphAnimator*>>();
        mSelectedForGraph.insert({widgetId, list});
    }
    auto &connCtxt = mSelectedForGraph[widgetId]->addObj(anim);
    connCtxt << connect(anim, &QObject::destroyed,
                        this, [this, widgetId, anim]() {
        removeSelectedForGraph(widgetId, anim);
    });
}

bool Canvas::removeSelectedForGraph(const int widgetId,
                                    GraphAnimator* const anim)
{
    return mSelectedForGraph[widgetId]->removeObj(anim);
}

const ConnContextObjList<GraphAnimator*>* Canvas::getSelectedForGraph(const int widgetId) const
{
    const auto it = mSelectedForGraph.find(widgetId);
    if (it == mSelectedForGraph.end()) { return nullptr; }
    return it->second.get();
}

void Canvas::setCurrentBoxesGroup(ContainerBox* const group)
{
    if (mCurrentContainer) {
        mCurrentContainer->setIsCurrentGroup_k(false);
    }
    clearBoxesSelection();
    clearPointsSelection();
    clearCurrentSmartEndPoint();
    clearLastPressedPoint();
    mCurrentContainer = group;
    group->setIsCurrentGroup_k(true);

    emit currentContainerSet(group);
}

void Canvas::updateHoveredBox(const eMouseEvent &e)
{
    mHoveredBox = mCurrentContainer->getBoxAt(e.fPos);
}

void Canvas::updateHoveredPoint(const eMouseEvent &e)
{
    mHoveredPoint_d = getPointAtAbsPos(e.fPos, mCurrentMode, 1/e.fScale);
}

void Canvas::updateHoveredEdge(const eMouseEvent &e)
{
    if (mCurrentMode != CanvasMode::pointTransform || mHoveredPoint_d) {
        return mHoveredNormalSegment.clear();
    }
    mHoveredNormalSegment = getSegment(e);
    if (mHoveredNormalSegment.isValid()) {
        mHoveredNormalSegment.generateSkPath();
    }
}

void Canvas::clearHovered()
{
    mHoveredBox.clear();
    mHoveredPoint_d.clear();
    mHoveredNormalSegment.clear();
}

bool Canvas::getPivotLocal() const
{
    return mDocument.fLocalPivot;
}

void Canvas::updateHovered(const eMouseEvent &e)
{
    updateHoveredPoint(e);
    updateHoveredEdge(e);
    updateHoveredBox(e);
}

void drawTransparencyMesh(SkCanvas* const canvas,
                          const SkRect &drawRect)
{
    SkPaint paint;
    SkBitmap bitmap;
    bitmap.setInfo(SkImageInfo::MakeA8(2, 2), 2);
    uint8_t pixels[4] = { 0, 255, 255, 0 };
    bitmap.setPixels(pixels);

    SkMatrix matr;
    const float scale = canvas->getTotalMatrix().getMinScale();
    const float dim = eSizesUI::widget*0.5f / (scale > 1.f ? 1.f : scale);
    matr.setScale(dim, dim);
    const auto shader = bitmap.makeShader(SkTileMode::kRepeat,
                                          SkTileMode::kRepeat, &matr);
    paint.setShader(shader);
    paint.setColor(SkColorSetARGB(255, 100, 100, 100));
    canvas->drawRect(drawRect, paint);
}

#include "efiltersettings.h"
void Canvas::renderSk(SkCanvas* const canvas,
                      const QRect& drawRect,
                      const QMatrix& viewTrans,
                      const bool mouseGrabbing) {
    mDrawnSinceQue = true;
    SkPaint paint;
    paint.setStyle(SkPaint::kFill_Style);
    const qreal pixelRatio = qApp->devicePixelRatio();
    const SkRect canvasRect = SkRect::MakeWH(mWidth, mHeight);
    const qreal zoom = viewTrans.m11();
    const auto filter = eFilterSettings::sDisplay(zoom, mResolution);
    const qreal qInvZoom = 1/viewTrans.m11() * pixelRatio;
    const float invZoom = toSkScalar(qInvZoom);
    const SkMatrix skViewTrans = toSkMatrix(viewTrans);
    const QColor bgColor = mBackgroundColor->getColor();
    const float intervals[2] = {eSizesUI::widget*0.25f*invZoom,
                                eSizesUI::widget*0.25f*invZoom};
    const auto dashPathEffect = SkDashPathEffect::Make(intervals, 2, 0);

    canvas->concat(skViewTrans);
    if(isPreviewingOrRendering()) {
        if(mSceneFrame) {
            canvas->clear(SK_ColorBLACK);
            canvas->save();
            if(bgColor.alpha() != 255)
                drawTransparencyMesh(canvas, canvasRect);
            const float reversedRes = toSkScalar(1/mSceneFrame->fResolution);
            canvas->scale(reversedRes, reversedRes);
            mSceneFrame->drawImage(canvas, filter);
            canvas->restore();
        }
        return;
    }
    canvas->save();
    if(mClipToCanvasSize) {
        canvas->clear(SK_ColorBLACK);
        canvas->clipRect(canvasRect);
    } else {
        canvas->clear(ThemeSupport::getThemeBaseSkColor());
        paint.setColor(SK_ColorGRAY);
        paint.setStyle(SkPaint::kStroke_Style);
        paint.setPathEffect(dashPathEffect);
        canvas->drawRect(toSkRect(getCurrentBounds()), paint);
    }
    const bool drawCanvas = mSceneFrame && mSceneFrame->fBoxState == mStateId;
    if(bgColor.alpha() != 255)
        drawTransparencyMesh(canvas, canvasRect);

    if(!mClipToCanvasSize || !drawCanvas) {
        canvas->saveLayer(nullptr, nullptr);
        if(bgColor.alpha() == 255 &&
           skViewTrans.mapRect(canvasRect).contains(toSkRect(drawRect))) {
            canvas->clear(toSkColor(bgColor));
        } else {
            paint.setStyle(SkPaint::kFill_Style);
            paint.setColor(toSkColor(bgColor));
            canvas->drawRect(canvasRect, paint);
        }
        drawContained(canvas, filter);
        canvas->restore();
    } else if(drawCanvas) {
        canvas->save();
        const float reversedRes = toSkScalar(1/mSceneFrame->fResolution);
        canvas->scale(reversedRes, reversedRes);
        mSceneFrame->drawImage(canvas, filter);
        canvas->restore();
    }

    canvas->restore();

    if (!enve_cast<Canvas*>(mCurrentContainer)) {
        mCurrentContainer->drawBoundingRect(canvas, invZoom);
    }
    //if(!mPaintTarget.isValid()) {
        const auto mods = QApplication::queryKeyboardModifiers();
        const bool ctrlPressed = mods & Qt::CTRL && mods & Qt::SHIFT;
        for(int i = mSelectedBoxes.count() - 1; i >= 0; i--) {
            const auto& iBox = mSelectedBoxes.at(i);
            canvas->save();
            iBox->drawBoundingRect(canvas, invZoom);
            iBox->drawAllCanvasControls(canvas, mCurrentMode, invZoom, ctrlPressed);
            canvas->restore();
        }
        for(const auto obj : mNullObjects) {
            canvas->save();
            obj->drawNullObject(canvas, mCurrentMode, invZoom, ctrlPressed);
            canvas->restore();
        }
    //}

    updateRotateHandleGeometry(qInvZoom);

    if (mRotateHandleVisible) {
        const QPointF center = mRotateHandleAnchor;
        const qreal radius = mRotateHandleRadius;
        const qreal strokeWorld = kRotateGizmoStrokePx * qInvZoom;

        const SkRect arcRect = SkRect::MakeLTRB(toSkScalar(center.x() - radius),
                                                toSkScalar(center.y() - radius),
                                                toSkScalar(center.x() + radius),
                                                toSkScalar(center.y() + radius));

        qreal startAngle = std::fmod(mRotateHandleStartOffsetDeg + mRotateHandleAngleDeg, 360.0); // base arc offset for gizmo draw
        if (startAngle < 0) { startAngle += 360.0; }
        const float startAngleF = static_cast<float>(startAngle);
        const float sweepAngleF = static_cast<float>(mRotateHandleSweepDeg); // arc spans mRotateHandleSweepDeg degrees

        SkPaint arcPaint;
        arcPaint.setAntiAlias(true);
        arcPaint.setStyle(SkPaint::kStroke_Style);
        arcPaint.setStrokeCap(SkPaint::kButt_Cap);
        arcPaint.setStrokeWidth(toSkScalar(strokeWorld));
        const SkColor arcColor = ThemeSupport::getThemeHighlightSkColor(mRotateHandleHovered ? 255 : 190);
        arcPaint.setColor(arcColor);
        canvas->drawArc(arcRect, startAngleF, sweepAngleF, false, arcPaint);

        auto drawAxisRect = [&](AxisConstraint axis, const AxisGizmoGeometry &geom, const QColor &baseColor) {
            if (!geom.visible) { return; }
            const bool hovered = axis == AxisConstraint::X ? mAxisXHovered : mAxisYHovered;
            const bool active = (mAxisConstraint == axis);
            QColor color = baseColor;
            color.setAlpha(active ? 255 : hovered ? 235 : baseColor.alpha());

            SkPaint fillPaint;
            fillPaint.setAntiAlias(true);
            fillPaint.setStyle(SkPaint::kFill_Style);
            fillPaint.setColor(toSkColor(color));

            SkPaint borderPaint;
            borderPaint.setAntiAlias(true);
            borderPaint.setStyle(SkPaint::kStroke_Style);
            borderPaint.setStrokeWidth(toSkScalar(kRotateGizmoStrokePx * invZoom * 0.2f));
            borderPaint.setColor(toSkColor(color.darker(150)));

            const qreal halfW = geom.size.width() * 0.5;
            const qreal halfH = geom.size.height() * 0.5;
            const qreal angleRadGeom = qDegreesToRadians(geom.angleDeg);
            const qreal cosG = std::cos(angleRadGeom);
            const qreal sinG = std::sin(angleRadGeom);
            auto mapPoint = [&](qreal localX, qreal localY) -> SkPoint {
                const qreal worldX = geom.center.x() + localX * cosG - localY * sinG;
                const qreal worldY = geom.center.y() + localX * sinG + localY * cosG;
                return SkPoint::Make(toSkScalar(worldX), toSkScalar(worldY));
            };

            SkPath path;
            path.moveTo(mapPoint(-halfW, -halfH));
            path.lineTo(mapPoint(halfW, -halfH));
            path.lineTo(mapPoint(halfW, halfH));
            path.lineTo(mapPoint(-halfW, halfH));
            path.close();

            canvas->drawPath(path, fillPaint);
            canvas->drawPath(path, borderPaint);
        };
        auto drawScaleSquare = [&](ScaleHandle handle, const ScaleGizmoGeometry &geom, const QColor &baseColor) {
            if (!geom.visible) { return; }
            bool hovered = false;
            switch (handle) {
            case ScaleHandle::X: hovered = mScaleXHovered; break;
            case ScaleHandle::Y: hovered = mScaleYHovered; break;
            case ScaleHandle::Uniform: hovered = mScaleUniformHovered; break;
            case ScaleHandle::None: default: hovered = false; break;
            }
            const bool active = (mScaleConstraint == handle);
            QColor color = baseColor;
            color.setAlpha(active ? 255 : hovered ? 235 : baseColor.alpha());

            SkPaint fillPaint;
            fillPaint.setAntiAlias(true);
            fillPaint.setStyle(SkPaint::kFill_Style);
            fillPaint.setColor(toSkColor(color));

            SkPaint borderPaint;
            borderPaint.setAntiAlias(true);
            borderPaint.setStyle(SkPaint::kStroke_Style);
            borderPaint.setStrokeWidth(toSkScalar(kRotateGizmoStrokePx * invZoom * 0.2f));
            borderPaint.setColor(toSkColor(color.darker(150)));

            const SkRect skRect = SkRect::MakeLTRB(toSkScalar(geom.center.x() - geom.halfExtent),
                                               toSkScalar(geom.center.y() - geom.halfExtent),
                                               toSkScalar(geom.center.x() + geom.halfExtent),
                                               toSkScalar(geom.center.y() + geom.halfExtent));
            canvas->drawRect(skRect, fillPaint);
            canvas->drawRect(skRect, borderPaint);
        };

        auto drawShearCircle = [&](ShearHandle handle, const ShearGizmoGeometry &geom, const QColor &baseColor) {
            if (!geom.visible) { return; }
            bool hovered = (handle == ShearHandle::X) ? mShearXHovered : mShearYHovered;
            const bool active = (mShearConstraint == handle);
            QColor color = baseColor;
            color.setAlpha(active ? 255 : hovered ? 235 : baseColor.alpha());

            SkPaint fillPaint;
            fillPaint.setAntiAlias(true);
            fillPaint.setStyle(SkPaint::kFill_Style);
            fillPaint.setColor(toSkColor(color));

            SkPaint borderPaint;
            borderPaint.setAntiAlias(true);
            borderPaint.setStyle(SkPaint::kStroke_Style);
            borderPaint.setStrokeWidth(toSkScalar(kRotateGizmoStrokePx * invZoom * 0.2f));
            borderPaint.setColor(toSkColor(color.darker(150)));

            const SkRect skRect = SkRect::MakeLTRB(toSkScalar(geom.center.x() - geom.radius),
                                               toSkScalar(geom.center.y() - geom.radius),
                                               toSkScalar(geom.center.x() + geom.radius),
                                               toSkScalar(geom.center.y() + geom.radius));
            canvas->drawOval(skRect, fillPaint);
            canvas->drawOval(skRect, borderPaint);
        };

        drawAxisRect(AxisConstraint::Y, mAxisYGeom, ThemeSupport::getThemeColorGreen(190));
        drawAxisRect(AxisConstraint::X, mAxisXGeom, ThemeSupport::getThemeColorRed(190));
        drawScaleSquare(ScaleHandle::Y, mScaleYGeom, ThemeSupport::getThemeColorGreen(190));
        drawScaleSquare(ScaleHandle::X, mScaleXGeom, ThemeSupport::getThemeColorRed(190));
        drawScaleSquare(ScaleHandle::Uniform, mScaleUniformGeom, ThemeSupport::getThemeColorYellow(190));
        drawShearCircle(ShearHandle::Y, mShearYGeom, ThemeSupport::getThemeColorGreen(190));
        drawShearCircle(ShearHandle::X, mShearXGeom, ThemeSupport::getThemeColorRed(190));
    }

    if(mCurrentMode == CanvasMode::boxTransform ||
       mCurrentMode == CanvasMode::pointTransform) {
        if(mTransMode == TransformMode::rotate ||
           mTransMode == TransformMode::scale ||
           mTransMode == TransformMode::shear) {
            mRotPivot->drawTransforming(canvas, mCurrentMode, invZoom,
                                        eSizesUI::widget*0.25f*invZoom);
        } else if(!mouseGrabbing || mRotPivot->isSelected()) {
            mRotPivot->drawSk(canvas, mCurrentMode, invZoom, false, false);
        }
    } else if(mCurrentMode == CanvasMode::drawPath) {
        const SkScalar nodeSize = 0.15f*eSizesUI::widget*invZoom;
        SkPaint paint;
        paint.setStyle(SkPaint::kFill_Style);
        paint.setAntiAlias(true);

        const auto& pts = mDrawPath.smoothPts();
        const auto drawColor = eSettings::instance().fLastUsedStrokeColor;
        paint.setARGB(255,
                      drawColor.red(),
                      drawColor.green(),
                      drawColor.blue());
        const SkScalar ptSize = 0.25*nodeSize;
        for(const auto& pt : pts) {
            canvas->drawCircle(pt.x(), pt.y(), ptSize, paint);
        }

        const bool drawFitted = mDocument.fDrawPathManual &&
                                mManualDrawPathState == ManualDrawPathState::drawn;
        if(drawFitted) {
            paint.setARGB(255, 255, 0, 0);
            const auto& highlightPts = mDrawPath.forceSplits();
            for(const int ptId : highlightPts) {
                const auto& pt = pts.at(ptId);
                canvas->drawCircle(pt.x(), pt.y(), nodeSize, paint);
            }
            const auto& fitted = mDrawPath.getFitted();
            paint.setARGB(255, 255, 0, 0);
            for(const auto& seg : fitted) {
                const auto path = seg.toSkPath();
                SkiaHelpers::drawOutlineOverlay(canvas, path, invZoom, SK_ColorWHITE);
                const auto& p0 = seg.p0();
                canvas->drawCircle(p0.x(), p0.y(), nodeSize, paint);
            }
            if(!mDrawPathTmp.isEmpty()) {
                SkiaHelpers::drawOutlineOverlay(canvas, mDrawPathTmp,
                                                invZoom, SK_ColorWHITE);
            }
        }

        paint.setARGB(255, 0, 75, 155);
        if(mHoveredPoint_d && mHoveredPoint_d->isSmartNodePoint()) {
            const QPointF pos = mHoveredPoint_d->getAbsolutePos();
            const qreal r = 0.5*qInvZoom*mHoveredPoint_d->getRadius();
            canvas->drawCircle(pos.x(), pos.y(), r, paint);
        }
        if(mDrawPathFirst) {
            const QPointF pos = mDrawPathFirst->getAbsolutePos();
            const qreal r = 0.5*qInvZoom*mDrawPathFirst->getRadius();
            canvas->drawCircle(pos.x(), pos.y(), r, paint);
        }
    }

    /*if(mPaintTarget.isValid()) {
        canvas->save();
        mPaintTarget.draw(canvas, viewTrans, invZoom, drawRect,
                          filter, mDocument.fOnionVisible);
        const SkIRect bRect = toSkIRect(mPaintTarget.pixelBoundingRect());
        paint.setStyle(SkPaint::kStroke_Style);
        paint.setColor(SK_ColorRED);
        paint.setPathEffect(dashPathEffect);
        canvas->drawIRect(bRect, paint);
        paint.setPathEffect(nullptr);
        canvas->restore();
    } else {*/
        if(mSelecting) {
            paint.setStyle(SkPaint::kStroke_Style);
            paint.setPathEffect(dashPathEffect);
            paint.setStrokeWidth(2*invZoom);
            paint.setColor(SkColorSetARGB(255, 0, 55, 255));
            canvas->drawRect(toSkRect(mSelectionRect), paint);
            paint.setStrokeWidth(invZoom);
            paint.setColor(SkColorSetARGB(255, 150, 150, 255));
            canvas->drawRect(toSkRect(mSelectionRect), paint);
            //paint.setPathEffect(nullptr);
        }

        if(mHoveredPoint_d) {
            mHoveredPoint_d->drawHovered(canvas, invZoom);
        } else if(mHoveredNormalSegment.isValid()) {
            mHoveredNormalSegment.drawHoveredSk(canvas, invZoom);
        } else if(mHoveredBox) {
            if(!mCurrentNormalSegment.isValid()) {
                mHoveredBox->drawHoveredSk(canvas, invZoom);
            }
        }
    //}

    paint.setStyle(SkPaint::kStroke_Style);
    paint.setStrokeWidth(invZoom);
    paint.setColor(SK_ColorGRAY);
    paint.setPathEffect(nullptr);
    canvas->drawRect(canvasRect, paint);

    canvas->resetMatrix();

    if(mTransMode != TransformMode::none || mValueInput.inputEnabled())
        mValueInput.draw(canvas, drawRect.height() - eSizesUI::widget);
}

void Canvas::setCanvasSize(const int width,
                           const int height)
{
    if (width == mWidth && height == mHeight) { return; }
    {
        prp_pushUndoRedoName(tr("Scene Dimension Changed"));
        UndoRedo ur;
        const QSize origSize{mWidth, mHeight};
        const QSize newSize{width, height};
        ur.fUndo = [this, origSize]() {
            setCanvasSize(origSize.width(),
                          origSize.height());
        };
        ur.fRedo = [this, newSize]() {
            setCanvasSize(newSize.width(),
                          newSize.height());
        };
        prp_addUndoRedo(ur);
    }
    mWidth = width;
    mHeight = height;
    prp_afterWholeInfluenceRangeChanged();
    emit dimensionsChanged(width, height);
}

void Canvas::setFrameRange(const FrameRange &range,
                           const bool undo)
{
    if (undo) {
        {
            prp_pushUndoRedoName(tr("Frame Range Changed"));
            UndoRedo ur;
            const FrameRange origRange(mRange);
            const FrameRange newRange(range);
            ur.fUndo = [this, origRange]() {
                setFrameRange(origRange);
            };
            ur.fRedo = [this, newRange]() {
                setFrameRange(newRange);
            };
            prp_addUndoRedo(ur);
        }
    }
    mRange = range;
    emit newFrameRange(range);
}

void Canvas::setFrameIn(const bool enabled,
                        const int frameIn)
{
    if (enabled && mOut.enabled && frameIn >= mOut.frame) { return; }
    const auto oIn = mIn;
    mIn.enabled = enabled;
    mIn.frame = frameIn;
    emit requestUpdate();
    {
        prp_pushUndoRedoName(tr("Frame In Changed"));
        UndoRedo ur;
        ur.fUndo = [this, oIn]() { setFrameIn(oIn.enabled, oIn.frame); };
        ur.fRedo = [this, enabled, frameIn]() { setFrameIn(enabled, frameIn); };
        prp_addUndoRedo(ur);
    }
}

void Canvas::setFrameOut(const bool enabled,
                         const int frameOut)
{
    if (enabled && mIn.enabled && frameOut <= mIn.frame) { return; }
    const auto oOut = mOut;
    mOut.enabled = enabled;
    mOut.frame = frameOut;
    emit requestUpdate();
    {
        prp_pushUndoRedoName(tr("Frame Out Changed"));
        UndoRedo ur;
        ur.fUndo = [this, oOut]() { setFrameOut(oOut.enabled, oOut.frame); };
        ur.fRedo = [this, enabled, frameOut]() { setFrameOut(enabled, frameOut); };
        prp_addUndoRedo(ur);
    }
}

const FrameMarker Canvas::getFrameIn() const
{
    return mIn;
}

const FrameMarker Canvas::getFrameOut() const
{
    return mOut;
}

void Canvas::clearFrameInOut()
{
    const auto oIn = mIn;
    const auto oOut = mOut;

    mIn.frame = 0;
    mIn.enabled = false;
    mOut.frame = 0;
    mOut.enabled = false;

    emit requestUpdate();
    {
        prp_pushUndoRedoName(tr("Cleared Frame In/Out"));
        UndoRedo ur;
        ur.fUndo = [this, oIn, oOut]() { restoreFrameInOut(oIn, oOut); };
        ur.fRedo = [this]() { clearFrameInOut(); };
        prp_addUndoRedo(ur);
    }
}

void Canvas::restoreFrameInOut(const FrameMarker &frameIn,
                               const FrameMarker &frameOut)
{
    mIn = frameIn;
    mOut = frameOut;
    emit requestUpdate();
}

void Canvas::setMarker(const QString &title,
                       const int frame)
{
    if (hasMarker(frame)) {
        if (!hasMarkerEnabled(frame)) {
            setMarkerEnabled(frame, true);
        } else { removeMarker(frame); }
        return;
    }
    const QString mark = title.isEmpty() ? QString::number(mMarkers.size()) : title;
    mMarkers.push_back({mark, true, frame});
    emit requestUpdate();
    {
        prp_pushUndoRedoName(tr("Added Marker"));
        UndoRedo ur;;
        ur.fUndo = [this, frame]() { removeMarker(frame); };
        ur.fRedo = [this, mark, frame]() { setMarker(mark, frame); };
        prp_addUndoRedo(ur);
    }
}

void Canvas::setMarker(const int frame)
{
    setMarker(QString::number(mMarkers.size()), frame);
    emit markersChanged();
}

void Canvas::setMarkerEnabled(const int frame,
                              const bool &enabled)
{
    const int index = getMarkerIndex(frame);
    if (index < 0) { return; }
    mMarkers.at(index).enabled = enabled;
    updateMarkers();
    {
        prp_pushUndoRedoName(tr("Changed Marker State"));
        UndoRedo ur;;
        ur.fUndo = [this, frame, enabled]() { setMarkerEnabled(frame, !enabled); };
        ur.fRedo = [this, frame, enabled]() { setMarkerEnabled(frame, enabled); };
        prp_addUndoRedo(ur);
    }
}

bool Canvas::hasMarker(const int frame,
                       const bool removeExists)
{
    int index = 0;
    for (const auto &mark: mMarkers) {
        if (mark.frame == frame) {
            if (removeExists) {
                mMarkers.erase(mMarkers.begin() + index);
                emit newFrameRange(mRange);
                {
                    prp_pushUndoRedoName(tr("Removed Marker"));
                    UndoRedo ur;;
                    ur.fUndo = [this, mark]() { setMarker(mark.title, mark.frame); };
                    ur.fRedo = [this, mark]() { removeMarker(mark.frame); };
                    prp_addUndoRedo(ur);
                }
            }
            return true;
        }
        index++;
    }
    return false;
}

bool Canvas::hasMarkerIn(const int frame)
{
    return mIn.enabled && mIn.frame == frame;
}

bool Canvas::hasMarkerOut(const int frame)
{
    return mOut.enabled && mOut.frame == frame;
}

bool Canvas::hasMarkerEnabled(const int frame)
{
    for (const auto &mark : mMarkers) {
        if (mark.frame == frame) { return mark.enabled; }
    }
    return false;
}

bool Canvas::removeMarker(const int frame)
{
    return hasMarker(frame, true);
}

bool Canvas::editMarker(const int frame,
                        const QString &title,
                        const bool enabled)
{
    int index = getMarkerIndex(frame);
    if (index >= 0) {
        const auto mark = mMarkers.at(index);
        mMarkers.at(index).title = title;
        mMarkers.at(index).enabled = enabled;
        emit newFrameRange(mRange);
        {
            prp_pushUndoRedoName(tr("Changed Marker"));
            UndoRedo ur;;
            ur.fUndo = [this, mark]() { editMarker(mark.frame, mark.title, mark.enabled); };
            ur.fRedo = [this, frame, title, enabled]() { editMarker(frame, title, enabled); };
            prp_addUndoRedo(ur);
        }
        return true;
    }
    return false;
}

void Canvas::moveMarkerFrame(const int markerFrame,
                             const int newFrame)
{
    if (markerFrame == newFrame) { return; }
    int index = getMarkerIndex(markerFrame);
    if (index >= 0) {
        mMarkers.at(index).frame = newFrame;
        emit newFrameRange(mRange);
        emit markersChanged();
        {
            prp_pushUndoRedoName(tr("Moved Marker"));
            UndoRedo ur;
            ur.fUndo = [this, markerFrame, newFrame]() { moveMarkerFrame(newFrame, markerFrame); };
            ur.fRedo = [this, markerFrame, newFrame]() { moveMarkerFrame(markerFrame, newFrame); };
            prp_addUndoRedo(ur);
        }
    }
}

const QString Canvas::getMarkerText(int frame)
{
    for (const auto &mark: mMarkers) {
        if (mark.frame == frame) { return mark.title; }
    }
    return QString();
}

int Canvas::getMarkerIndex(const int frame)
{
    for (size_t i = 0; i < mMarkers.size(); i++) {
        if (mMarkers.at(i).frame == frame) { return i; }
    }
    return -1;
}

const std::vector<FrameMarker> Canvas::getMarkers()
{
    return mMarkers;
}

void Canvas::clearMarkers()
{
    const auto markers = mMarkers;
    mMarkers.clear();
    emit markersChanged();
    emit requestUpdate();
    {
        prp_pushUndoRedoName(tr("Cleared Markers"));
        UndoRedo ur;
        ur.fUndo = [this, markers]() { restoreMarkers(markers); };
        ur.fRedo = [this]() { clearMarkers(); };
        prp_addUndoRedo(ur);
    }
}

void Canvas::updateMarkers()
{
    emit newFrameRange(mRange);
    emit requestUpdate();
}

void Canvas::restoreMarkers(const std::vector<FrameMarker> &markers)
{
    mMarkers = markers;
    updateMarkers();
}

void Canvas::addKeySelectedProperties()
{
    for (const auto &prop : mSelectedProps.getList()) {
        const auto asAnim = enve_cast<Animator*>(prop);
        if (!asAnim) { continue; }
        asAnim->anim_saveCurrentValueAsKey();
    }
    mDocument.actionFinished();
}

stdsptr<BoxRenderData> Canvas::createRenderData() {
    return enve::make_shared<CanvasRenderData>(this);
}

QSize Canvas::getCanvasSize() {
    return QSize(mWidth, mHeight);
}

void Canvas::setPreviewing(const bool bT) {
    mPreviewing = bT;
}

void Canvas::setRenderingPreview(const bool bT) {
    mRenderingPreview = bT;
}

void Canvas::anim_scaleTime(const int pivotAbsFrame, const qreal scale) {
    ContainerBox::anim_scaleTime(pivotAbsFrame, scale);
    //        int newAbsPos = qRound(scale*pivotAbsFrame);
    //        anim_shiftAllKeys(newAbsPos - pivotAbsFrame);
    const int newMin = qRound((mRange.fMin - pivotAbsFrame)*scale);
    const int newMax = qRound((mRange.fMax - pivotAbsFrame)*scale);
    setFrameRange({newMin, newMax});
}

void Canvas::setOutputRendering(const bool bT) {
    mRenderingOutput = bT;
}

void Canvas::setSceneFrame(const int relFrame) {
    const auto cont = mSceneFramesHandler.atFrame(relFrame);
    setSceneFrame(enve::shared<SceneFrameContainer>(cont));
}

void Canvas::setSceneFrame(const stdsptr<SceneFrameContainer>& cont) {
    setLoadingSceneFrame(nullptr);
    mSceneFrame = cont;
    emit requestUpdate();
}

void Canvas::setLoadingSceneFrame(const stdsptr<SceneFrameContainer>& cont) {
    if(mLoadingSceneFrame == cont) return;
    mLoadingSceneFrame = cont;
    if(cont) {
        Q_ASSERT(!cont->storesDataInMemory());
        cont->scheduleLoadFromTmpFile();
    }
}

FrameRange Canvas::prp_getIdenticalRelRange(const int relFrame) const {
    const auto groupRange = ContainerBox::prp_getIdenticalRelRange(relFrame);
    //FrameRange canvasRange{0, mMaxFrame};
    return groupRange;//*canvasRange;
}

void Canvas::renderDataFinished(BoxRenderData *renderData) {
    const bool currentState = renderData->fBoxStateId == mStateId;
    if(currentState) mRenderDataHandler.removeItemAtRelFrame(renderData->fRelFrame);
    else if(renderData->fBoxStateId < mLastStateId) return;
    const int relFrame = qRound(renderData->fRelFrame);
    mLastStateId = renderData->fBoxStateId;

    const auto range = prp_getIdenticalRelRange(relFrame);
    const auto cont = enve::make_shared<SceneFrameContainer>(
                this, renderData, range,
                currentState ? &mSceneFramesHandler : nullptr);
    if(currentState) mSceneFramesHandler.add(cont);

    if(!mPreviewing && !mRenderingOutput){
        bool newerSate = true;
        bool closerFrame = true;
        if(mSceneFrame) {
            newerSate = mSceneFrame->fBoxState < renderData->fBoxStateId;
            const int cRelFrame = anim_getCurrentRelFrame();
            const int finishedFrameDist = qMin(qAbs(cRelFrame - range.fMin),
                                               qAbs(cRelFrame - range.fMax));
            const FrameRange cRange = mSceneFrame->getRange();
            const int oldFrameDist = qMin(qAbs(cRelFrame - cRange.fMin),
                                          qAbs(cRelFrame - cRange.fMax));
            closerFrame = finishedFrameDist < oldFrameDist;
        }
        if(newerSate || closerFrame) {
            mSceneFrameOutdated = !currentState;
            setSceneFrame(cont);
        }
    }
}

void Canvas::prp_afterChangedAbsRange(const FrameRange &range, const bool clip) {
    Property::prp_afterChangedAbsRange(range, clip);
    mSceneFramesHandler.remove(range);
    if(!mSceneFramesHandler.atFrame(anim_getCurrentRelFrame())) {
        mSceneFrameOutdated = true;
        planUpdate(UpdateReason::userChange);
    }
}

void Canvas::saveSceneSVG(SvgExporter& exp) const
{
    auto &svg = exp.svg();
    if (exp.fColors11) {
        svg.setAttribute("version", "1.1");
    }
    svg.setAttribute("xmlns", "http://www.w3.org/2000/svg");
    svg.setAttribute("xmlns:xlink", "http://www.w3.org/1999/xlink");

    const auto viewBox = QString("0 0 %1 %2").
                         arg(mWidth).arg(mHeight);
    svg.setAttribute("viewBox", viewBox);

    if (exp.fFixedSize) {
        svg.setAttribute("width", mWidth);
        svg.setAttribute("height", mHeight);
    }

    for (const auto &grad : mGradients) {
        grad->saveSVG(exp);
    }

    if (exp.fBackground) {
        auto bg = exp.createElement("rect");
        bg.setAttribute("width", mWidth);
        bg.setAttribute("height", mHeight);
        mBackgroundColor->saveColorSVG(exp, bg, exp.fAbsRange, "fill");
        svg.appendChild(bg);
    }

    const auto task = enve::make_shared<DomEleTask>(exp, exp.fAbsRange);
    exp.addNextTask(task);
    saveBoxesSVG(exp, task.get(), svg);
    task->queTask();
}

qsptr<BoundingBox> Canvas::createLink(const bool inner)
{
    return enve::make_shared<InternalLinkCanvas>(this, inner);
}

void Canvas::schedulePivotUpdate()
{
    if (mTransMode == TransformMode::rotate ||
        mTransMode == TransformMode::scale ||
        mRotPivot->isSelected()) { return; }
    mPivotUpdateNeeded = true;
}

void Canvas::updatePivotIfNeeded()
{
    if (mPivotUpdateNeeded) {
        mPivotUpdateNeeded = false;
        updatePivot();
    }
}

void Canvas::makePointCtrlsSymmetric()
{
    prp_pushUndoRedoName(tr("Make Nodes Symmetric"));
    setPointCtrlsMode(CtrlsMode::symmetric);
}

void Canvas::makePointCtrlsSmooth()
{
    prp_pushUndoRedoName(tr("Make Nodes Smooth"));
    setPointCtrlsMode(CtrlsMode::smooth);
}

void Canvas::makePointCtrlsCorner()
{
    prp_pushUndoRedoName(tr("Make Nodes Corner"));
    setPointCtrlsMode(CtrlsMode::corner);
}

void Canvas::newEmptyPaintFrameAction()
{
    //if(mPaintTarget.isValid()) mPaintTarget.newEmptyFrame();
}

void Canvas::moveSecondSelectionPoint(const QPointF &pos)
{
    mSelectionRect.setBottomRight(pos);
}

void Canvas::startSelectionAtPoint(const QPointF &pos)
{
    mSelecting = true;
    mSelectionRect.setTopLeft(pos);
    mSelectionRect.setBottomRight(pos);
}

void Canvas::updatePivot()
{
    if (mCurrentMode == CanvasMode::pointTransform) {
        mRotPivot->setAbsolutePos(getSelectedPointsAbsPivotPos());
    } else if (mCurrentMode == CanvasMode::boxTransform) {
        mRotPivot->setAbsolutePos(getSelectedBoxesAbsPivotPos());
    }
}

void Canvas::setCanvasMode(const CanvasMode mode)
{
    if (mCurrentMode == CanvasMode::pickFillStroke ||
        mCurrentMode == CanvasMode::pickFillStrokeEvent) {
        emit currentPickedColor(QColor());
        emit currentHoverColor(QColor());
    }
    mCurrentMode = mode;
    mSelecting = false;
    mStylusDrawing = false;
    clearPointsSelection();
    clearCurrentSmartEndPoint();
    clearLastPressedPoint();
    updatePivot();
    //updatePaintBox();
    emit canvasModeSet(mode);
}

/*void Canvas::updatePaintBox()
{
    mPaintTarget.setPaintBox(nullptr);
    if (mCurrentMode != CanvasMode::paint) { return; }
    for (int i = mSelectedBoxes.count() - 1; i >= 0; i--) {
        const auto& iBox = mSelectedBoxes.at(i);
        if (enve_cast<PaintBox*>(iBox)) {
            mPaintTarget.setPaintBox(static_cast<PaintBox*>(iBox));
            break;
        }
    }
}*/

/*bool Canvas::handlePaintModeKeyPress(const eKeyEvent &e)
{
    if (mCurrentMode != CanvasMode::paint) { return false; }
    if (e.fKey == Qt::Key_N && mPaintTarget.isValid()) {
        newEmptyPaintFrameAction();
    } else { return false; }
    return true;
}*/

bool Canvas::handleModifierChange(const eKeyEvent &e)
{
    if (mCurrentMode == CanvasMode::pointTransform) {
        if (e.fKey == Qt::Key_Alt ||
            e.fKey == Qt::Key_Shift ||
            e.fKey == Qt::Key_Meta) {
            handleMovePointMouseMove(e);
            return true;
        } else if (e.fKey == Qt::Key_Control) { return true; }
    }
    return false;
}

bool Canvas::handleTransormationInputKeyEvent(const eKeyEvent &e)
{
    if (mValueInput.handleTransormationInputKeyEvent(e.fKey)) {
        if (mTransMode == TransformMode::rotate) { mValueInput.setupRotate(); }
        updateTransformation(e);
        mStartTransform = false;
    } else if (e.fKey == Qt::Key_Escape) {
        if (!e.fMouseGrabbing) { return false; }
        cancelCurrentTransform();
        e.fReleaseMouse();
    } else if (e.fKey == Qt::Key_Return ||
               e.fKey == Qt::Key_Enter) {
        handleLeftMouseRelease(e);
    } else if (e.fKey == Qt::Key_X) {
        if (e.fAutorepeat) { return false; }
        mValueInput.switchXOnlyMode();
        updateTransformation(e);
    } else if (e.fKey == Qt::Key_Y) {
        if (e.fAutorepeat) { return false; }
        mValueInput.switchYOnlyMode();
        updateTransformation(e);
    } else { return false; }
    return true;
}

void Canvas::deleteAction()
{
    switch(mCurrentMode) {
    case CanvasMode::pointTransform:
        removeSelectedPointsAndClearList();
        break;
    case CanvasMode::boxTransform:
    case CanvasMode::circleCreate:
    case CanvasMode::rectCreate:
    case CanvasMode::textCreate:
    case CanvasMode::nullCreate:
    case CanvasMode::drawPath:
    case CanvasMode::pathCreate:
        removeSelectedBoxesAndClearList();
        break;
    default:;
    }
}

void Canvas::copyAction()
{
    if (mSelectedBoxes.isEmpty()) { return; }
    const auto container = enve::make_shared<BoxesClipboard>(mSelectedBoxes.getList());
    Document::sInstance->replaceClipboard(container);
}

void Canvas::pasteAction()
{
    const auto container = Document::sInstance->getBoxesClipboard();
    if (!container) { return; }
    clearBoxesSelection();
    container->pasteTo(mCurrentContainer);
}

void Canvas::cutAction()
{
    if (mSelectedBoxes.isEmpty()) { return; }
    copyAction();
    deleteAction();
}

void Canvas::splitAction()
{
    if (mSelectedBoxes.isEmpty() || mSelectedBoxes.count() > 1) { return; }

    const auto bBox = enve_cast<BoundingBox*>(mSelectedBoxes.getList().at(0));
    if (!bBox) { return; }

    const auto dRect = bBox->getDurationRectangle();
    if (!dRect) { return; }

    const auto frame = getCurrentFrame();
    const auto values = dRect->getValues();
    const auto range = dRect->getAbsFrameRange();

    if (!range.inRange(frame)) { return; }

    int offset = values.fMax - (range.fMax - frame);

    copyAction();
    pasteAction();

    if (mCurrentContainer->getContainedBoxesCount() < 1) { return; }

    const auto box = mCurrentContainer->getContainedBoxes().at(0);
    if (!box) { return; }

    const auto cRect = box->getDurationRectangle();
    if (!cRect) { return; }

    dRect->setValues({values.fShift, offset, values.fMax});
    cRect->setValues({values.fShift, values.fMin, offset});

    for (int i = box->getZIndex(); i < bBox->getZIndex(); i = box->getZIndex()) {
        box->moveDown();
    }

    mSelectedBoxes.removeObj(box);
    box->setSelected(false);
    mSelectedBoxes.addObj(bBox);
    bBox->setSelected(true);

    mDocument.actionFinished();
}

void Canvas::duplicateAction()
{
    copyAction();
    pasteAction();
}

void Canvas::selectAllAction()
{
    if (mCurrentMode == CanvasMode::pointTransform) {
        selectAllPointsAction();
    } else {//if(mCurrentMode == MOVE_PATH) {
        selectAllBoxesFromBoxesGroup();
    }
}

void Canvas::invertSelectionAction()
{
    if (mCurrentMode == CanvasMode::pointTransform) {
        QList<MovablePoint*> selectedPts = mSelectedPoints_d;
        selectAllPointsAction();
        for (const auto &pt : selectedPts) { removePointFromSelection(pt); }
    } else {//if(mCurrentMode == MOVE_PATH) {
        QList<BoundingBox*> boxes = mSelectedBoxes.getList();
        selectAllBoxesFromBoxesGroup();
        for (const auto &box : boxes) { removeBoxFromSelection(box); }
    }
}

void Canvas::anim_setAbsFrame(const int frame)
{
    if (frame == anim_getCurrentAbsFrame()) { return; }
    ContainerBox::anim_setAbsFrame(frame);
    const int newRelFrame = anim_getCurrentRelFrame();

    const auto cont = mSceneFramesHandler.atFrame<SceneFrameContainer>(newRelFrame);
    if (cont) {
        if (cont->storesDataInMemory()) {
            setSceneFrame(cont->ref<SceneFrameContainer>());
        } else {
            setLoadingSceneFrame(cont->ref<SceneFrameContainer>());
        }
        mSceneFrameOutdated = !cont->storesDataInMemory();
    } else {
        mSceneFrameOutdated = true;
        planUpdate(UpdateReason::frameChange);
    }

    mUndoRedoStack->setFrame(frame);

    //if (mCurrentMode == CanvasMode::paint) { mPaintTarget.setupOnionSkin(); }
    emit currentFrameChanged(frame);

    schedulePivotUpdate();
}

void Canvas::clearSelectionAction()
{
    if (mCurrentMode == CanvasMode::pointTransform) {
        clearPointsSelection();
    } else {//if(mCurrentMode == MOVE_PATH) {
        clearPointsSelection();
        clearBoxesSelection();
    }
}

void Canvas::finishedAction()
{
    mDocument.actionFinished();
}

void Canvas::clearParentForSelected()
{
    for (int i = 0; i < mSelectedBoxes.count(); i++) {
        mSelectedBoxes.at(i)->clearParent();
    }
}

void Canvas::setParentToLastSelected()
{
    if (mSelectedBoxes.count() > 1) {
        const auto& lastBox = mSelectedBoxes.last();
        const auto trans = lastBox->getTransformAnimator();
        for (int i = 0; i < mSelectedBoxes.count() - 1; i++) {
            mSelectedBoxes.at(i)->setParentTransform(trans);
        }
    }
}

bool Canvas::prepareRotation(const QPointF &startPos, bool fromHandle)
{
    if (mCurrentMode != CanvasMode::boxTransform &&
        mCurrentMode != CanvasMode::pointTransform) { return false; }
    if (mSelectedBoxes.isEmpty()) { return false; }
    if (mCurrentMode == CanvasMode::pointTransform) {
        if (mSelectedPoints_d.isEmpty()) { return false; }
    }

    mRotatingFromHandle = fromHandle;
    mValueInput.clearAndDisableInput();
    mValueInput.setupRotate();

    if (fromHandle) {
        setGizmosSuppressed(true);
    }

    mRotPivot->setMousePos(startPos);
    mTransMode = TransformMode::rotate;
    mRotHalfCycles = 0;
    mLastDRot = 0;

    mDoubleClick = false;
    mStartTransform = true;
    return true;
}

bool Canvas::startRotatingAction(const eKeyEvent &e)
{
    if (!prepareRotation(e.fPos)) { return false; }
    e.fGrabMouse();
    return true;
}

void Canvas::updateRotateHandleGeometry(qreal invScale)
{
    mRotateHandleVisible = false;
    mRotateHandleRadius = 0;

    auto resetAllGizmos = [&]() {
        setAxisGizmoHover(AxisConstraint::X, false);
        setAxisGizmoHover(AxisConstraint::Y, false);
        setScaleGizmoHover(ScaleHandle::X, false);
        setScaleGizmoHover(ScaleHandle::Y, false);
        setScaleGizmoHover(ScaleHandle::Uniform, false);
        setShearGizmoHover(ShearHandle::X, false);
        setShearGizmoHover(ShearHandle::Y, false);

        mAxisXGeom = AxisGizmoGeometry();
        mAxisYGeom = AxisGizmoGeometry();
        mScaleXGeom = ScaleGizmoGeometry();
        mScaleYGeom = ScaleGizmoGeometry();
        mScaleUniformGeom = ScaleGizmoGeometry();
        mShearXGeom = ShearGizmoGeometry();
        mShearYGeom = ShearGizmoGeometry();

        mAxisConstraint = AxisConstraint::None;
        mScaleConstraint = ScaleHandle::None;
        mShearConstraint = ShearHandle::None;
        mAxisHandleActive = false;
        mScaleHandleActive = false;
        mShearHandleActive = false;
        mValueInput.setForce1D(false);
        mValueInput.setXYMode();
    };

    if (mGizmosSuppressed) {
        return;
    }

    if ((mCurrentMode != CanvasMode::boxTransform &&
         mCurrentMode != CanvasMode::pointTransform)) {
        setRotateHandleHover(false);
        resetAllGizmos();
        return;
    }

    if (mSelectedBoxes.isEmpty()) {
        setRotateHandleHover(false);
        resetAllGizmos();
        return;
    }

    if (!mRotPivot) {
        setRotateHandleHover(false);
        resetAllGizmos();
        return;
    }

    mRotateHandleAngleDeg = 0.0; // keep gizmo orientation screen-aligned

    const QPointF pivot = mRotPivot->getAbsolutePos();
    mRotateHandleAnchor = pivot + QPointF(kAxisGizmoWidthPx * 0.5 * invScale,
                                          -kAxisGizmoWidthPx * 0.5 * invScale);

    const qreal baseRotateRadiusWorld = kRotateGizmoRadiusPx * invScale;
    mRotateHandleRadius = baseRotateRadiusWorld;
    mRotateHandleSweepDeg = kRotateGizmoSweepDeg; // default sweep angle
    mRotateHandleStartOffsetDeg = kRotateGizmoBaseOffsetDeg; // default base angle offset

    const qreal axisWidthWorld = kAxisGizmoWidthPx * invScale;
    const qreal axisHeightWorld = kAxisGizmoHeightPx * invScale;
    const qreal axisGapYWorld = kAxisGizmoYOffsetPx * invScale;
    const qreal axisGapXWorld = kAxisGizmoXOffsetPx * invScale;

    mAxisYGeom.center = pivot + QPointF(0.0, -axisGapYWorld);
    mAxisYGeom.size = QSizeF(axisWidthWorld, axisHeightWorld);
    mAxisYGeom.angleDeg = 0.0;
    mAxisYGeom.visible = true;

    mAxisXGeom.center = pivot + QPointF(axisGapXWorld, 0.0);
    mAxisXGeom.size = QSizeF(axisHeightWorld, axisWidthWorld);
    mAxisXGeom.angleDeg = 0.0;
    mAxisXGeom.visible = true;

    const qreal rotateOffsetWorld = mAxisYGeom.size.width() * 0.5;
    mRotateHandleRadius = baseRotateRadiusWorld + rotateOffsetWorld;

    const QPointF offset(0.0, -mRotateHandleRadius);
    mRotateHandlePos = pivot + offset;

    const qreal scaleSizeWorld = kScaleGizmoSizePx * invScale;
    const qreal scaleHalf = scaleSizeWorld * 0.5;
    const qreal scaleGapWorld = kScaleGizmoGapPx * invScale;
    const qreal axisYTop = mAxisYGeom.center.y() - (mAxisYGeom.size.height() * 0.5);
    const qreal axisXRight = mAxisXGeom.center.x() + (mAxisXGeom.size.width() * 0.5);
    const qreal scaleCenterY = axisYTop - scaleGapWorld - scaleHalf;
    const qreal scaleCenterX = axisXRight + scaleGapWorld + scaleHalf;

    mScaleYGeom.center = QPointF(mAxisYGeom.center.x(), scaleCenterY);
    mScaleYGeom.halfExtent = scaleHalf;
    mScaleYGeom.visible = true;

    mScaleXGeom.center = QPointF(scaleCenterX, mAxisXGeom.center.y());
    mScaleXGeom.halfExtent = scaleHalf;
    mScaleXGeom.visible = true;

    mScaleUniformGeom.center = QPointF(scaleCenterX, scaleCenterY);
    mScaleUniformGeom.halfExtent = scaleHalf;
    mScaleUniformGeom.visible = true;

    const qreal shearRadiusWorld = kShearGizmoRadiusPx * invScale;

    const QPointF scaleYCenter = mScaleYGeom.center;
    const QPointF scaleXCenter = mScaleXGeom.center;
    const QPointF scaleUniformCenter = mScaleUniformGeom.center;

    mShearXGeom.center = QPointF((scaleYCenter.x() + scaleUniformCenter.x()) * 0.5,
                                 scaleCenterY);
    mShearXGeom.radius = shearRadiusWorld;
    mShearXGeom.visible = true;

    mShearYGeom.center = QPointF(scaleCenterX,
                                 (scaleXCenter.y() + scaleUniformCenter.y()) * 0.5);
    mShearYGeom.radius = shearRadiusWorld;
    mShearYGeom.visible = true;

    mRotateHandleVisible = true;
}

void Canvas::setRotateHandleHover(bool hovered)
{
    if (mRotateHandleHovered == hovered) { return; }
    mRotateHandleHovered = hovered;
    emit requestUpdate();
}

void Canvas::setGizmosSuppressed(bool suppressed)
{
    if (mGizmosSuppressed == suppressed) { return; }
    mGizmosSuppressed = suppressed;
    if (suppressed) {
        mRotateHandleHovered = false;
        mAxisXHovered = false;
        mAxisYHovered = false;
        mScaleXHovered = false;
        mScaleYHovered = false;
        mScaleUniformHovered = false;
        mShearXHovered = false;
        mShearYHovered = false;
    }
    emit requestUpdate();
}

void Canvas::setAxisGizmoHover(AxisConstraint axis, bool hovered)
{
    bool *target = axis == AxisConstraint::X ? &mAxisXHovered : &mAxisYHovered;
    if (*target == hovered) { return; }
    *target = hovered;
    emit requestUpdate();
}

void Canvas::setScaleGizmoHover(ScaleHandle handle, bool hovered)
{
    bool *target = nullptr;
    switch (handle) {
    case ScaleHandle::X:
        target = &mScaleXHovered;
        break;
    case ScaleHandle::Y:
        target = &mScaleYHovered;
        break;
    case ScaleHandle::Uniform:
        target = &mScaleUniformHovered;
        break;
    case ScaleHandle::None:
    default:
        return;
    }

    if (*target == hovered) { return; }
    *target = hovered;
    emit requestUpdate();
}

void Canvas::setShearGizmoHover(ShearHandle handle, bool hovered)
{
    bool *target = nullptr;
    switch (handle) {
    case ShearHandle::X:
        target = &mShearXHovered;
        break;
    case ShearHandle::Y:
        target = &mShearYHovered;
        break;
    case ShearHandle::None:
    default:
        return;
    }

    if (*target == hovered) { return; }
    *target = hovered;
    emit requestUpdate();
}

bool Canvas::pointOnScaleGizmo(ScaleHandle handle, const QPointF &pos, qreal invScale) const
{
    Q_UNUSED(invScale);
    if (!mRotateHandleVisible) { return false; }

    const ScaleGizmoGeometry *geom = nullptr;
    switch (handle) {
    case ScaleHandle::X:
        geom = &mScaleXGeom;
        break;
    case ScaleHandle::Y:
        geom = &mScaleYGeom;
        break;
    case ScaleHandle::Uniform:
        geom = &mScaleUniformGeom;
        break;
    case ScaleHandle::None:
    default:
        return false;
    }

    if (!geom->visible || geom->halfExtent <= 0.0) { return false; }

    const qreal half = geom->halfExtent;
    return std::abs(pos.x() - geom->center.x()) <= half &&
           std::abs(pos.y() - geom->center.y()) <= half;
}

bool Canvas::pointOnShearGizmo(ShearHandle handle, const QPointF &pos, qreal invScale) const
{
    Q_UNUSED(invScale);
    if (!mRotateHandleVisible) { return false; }

    const ShearGizmoGeometry *geom = nullptr;
    switch (handle) {
    case ShearHandle::X:
        geom = &mShearXGeom;
        break;
    case ShearHandle::Y:
        geom = &mShearYGeom;
        break;
    case ShearHandle::None:
    default:
        return false;
    }

    if (!geom->visible || geom->radius <= 0.0) { return false; }

    const qreal distance = std::hypot(pos.x() - geom->center.x(),
                                      pos.y() - geom->center.y());
    return distance <= geom->radius;
}

bool Canvas::pointOnAxisGizmo(AxisConstraint axis, const QPointF &pos, qreal invScale) const
{
    Q_UNUSED(invScale);
    if (!mRotateHandleVisible) { return false; }

    const AxisGizmoGeometry &geom = axis == AxisConstraint::X ? mAxisXGeom : mAxisYGeom;
    if (!geom.visible) { return false; }

    const QPointF relative = pos - geom.center;
    const qreal angleRadGeom = qDegreesToRadians(geom.angleDeg);
    const qreal cosG = std::cos(angleRadGeom);
    const qreal sinG = std::sin(angleRadGeom);
    const qreal localX = relative.x() * cosG + relative.y() * sinG;
    const qreal localY = -relative.x() * sinG + relative.y() * cosG;
    const qreal halfW = geom.size.width() * 0.5;
    const qreal halfH = geom.size.height() * 0.5;
    return std::abs(localX) <= halfW && std::abs(localY) <= halfH;
}

bool Canvas::pointOnRotateGizmo(const QPointF &pos, qreal invScale) const
{
    if (!mRotateHandleVisible) { return false; }
    const qreal halfThicknessWorld = (kRotateGizmoHitWidthPx * invScale) * 0.5;
    const QPointF center = mRotateHandleAnchor;
    const qreal radius = mRotateHandleRadius;
    if (radius <= 0) { return false; }

    const qreal dx = pos.x() - center.x();
    const qreal dy = pos.y() - center.y();
    const qreal distance = std::hypot(dx, dy);
    if (distance < radius - halfThicknessWorld || distance > radius + halfThicknessWorld) {
        return false;
    }

    const double angleCCW = qRadiansToDegrees(std::atan2(center.y() - pos.y(), pos.x() - center.x()));
    const double skAngle = std::fmod(360.0 + (360.0 - angleCCW), 360.0);
    const double arcStart = std::fmod(mRotateHandleStartOffsetDeg + mRotateHandleAngleDeg, 360.0);
    const double normalizedStart = arcStart < 0 ? arcStart + 360.0 : arcStart;
    const double delta = std::fmod((skAngle - normalizedStart + 360.0), 360.0);

    double extraAngleDeg = 0.0;
    if (radius > 0) {
        const qreal halfStrokeWorld = (kRotateGizmoStrokePx * invScale) * 0.5;
        extraAngleDeg = qRadiansToDegrees(halfStrokeWorld / radius);
    }

    if (delta <= mRotateHandleSweepDeg + extraAngleDeg) { return true; }
    if (extraAngleDeg > 0.0 && delta >= 360.0 - extraAngleDeg) { return true; }
    return false;
}

void Canvas::updateRotateHandleHover(const QPointF &pos, qreal invScale)
{
    updateRotateHandleGeometry(invScale);
    setRotateHandleHover(pointOnRotateGizmo(pos, invScale));
    setAxisGizmoHover(AxisConstraint::X, pointOnAxisGizmo(AxisConstraint::X, pos, invScale));
    setAxisGizmoHover(AxisConstraint::Y, pointOnAxisGizmo(AxisConstraint::Y, pos, invScale));
    setScaleGizmoHover(ScaleHandle::X, pointOnScaleGizmo(ScaleHandle::X, pos, invScale));
    setScaleGizmoHover(ScaleHandle::Y, pointOnScaleGizmo(ScaleHandle::Y, pos, invScale));
    setScaleGizmoHover(ScaleHandle::Uniform, pointOnScaleGizmo(ScaleHandle::Uniform, pos, invScale));
    setShearGizmoHover(ShearHandle::X, pointOnShearGizmo(ShearHandle::X, pos, invScale));
    setShearGizmoHover(ShearHandle::Y, pointOnShearGizmo(ShearHandle::Y, pos, invScale));
}

bool Canvas::startScaleConstrainedMove(const eMouseEvent &e, ScaleHandle handle)
{
    if (mCurrentMode != CanvasMode::boxTransform &&
        mCurrentMode != CanvasMode::pointTransform) { return false; }
    if (mSelectedBoxes.isEmpty() && mSelectedPoints_d.isEmpty()) { return false; }

    mValueInput.clearAndDisableInput();
    mValueInput.setupScale();

    if (handle == ScaleHandle::Uniform) {
        mValueInput.setForce1D(false);
        mValueInput.setXYMode();
    } else {
        mValueInput.setForce1D(true);
        if (handle == ScaleHandle::X) {
            mValueInput.setXOnlyMode();
        } else {
            mValueInput.setYOnlyMode();
        }
    }

    mTransMode = TransformMode::scale;
    mDoubleClick = false;
    mStartTransform = true;

    mScaleConstraint = handle;
    mScaleHandleActive = true;
    setScaleGizmoHover(handle, true);
    mRotPivot->setMousePos(e.fPos);
    setGizmosSuppressed(true);

    e.fGrabMouse();
    return true;
}

bool Canvas::startShearConstrainedMove(const eMouseEvent &e, ShearHandle handle)
{
    if (mCurrentMode != CanvasMode::boxTransform &&
        mCurrentMode != CanvasMode::pointTransform) { return false; }
    if (mSelectedBoxes.isEmpty() && mSelectedPoints_d.isEmpty()) { return false; }

    mValueInput.clearAndDisableInput();
    mValueInput.setupShear();
    mValueInput.setForce1D(true);
    if (handle == ShearHandle::X) {
        mValueInput.setXOnlyMode();
    } else {
        mValueInput.setYOnlyMode();
    }

    mTransMode = TransformMode::shear;
    mDoubleClick = false;
    mStartTransform = true;

    mShearConstraint = handle;
    mShearHandleActive = true;
    setShearGizmoHover(handle, true);
    mRotPivot->setMousePos(e.fPos);
    setGizmosSuppressed(true);

    e.fGrabMouse();
    return true;
}

bool Canvas::tryStartShearGizmo(const eMouseEvent &e, qreal invScale)
{
    updateRotateHandleGeometry(invScale);
    if (!mRotateHandleVisible) { return false; }

    if (pointOnShearGizmo(ShearHandle::Y, e.fPos, invScale)) {
        return startShearConstrainedMove(e, ShearHandle::Y);
    }
    if (pointOnShearGizmo(ShearHandle::X, e.fPos, invScale)) {
        return startShearConstrainedMove(e, ShearHandle::X);
    }
    return false;
}

bool Canvas::tryStartScaleGizmo(const eMouseEvent &e, qreal invScale)
{
    updateRotateHandleGeometry(invScale);
    if (!mRotateHandleVisible) { return false; }

    if (pointOnScaleGizmo(ScaleHandle::Y, e.fPos, invScale)) {
        return startScaleConstrainedMove(e, ScaleHandle::Y);
    }
    if (pointOnScaleGizmo(ScaleHandle::X, e.fPos, invScale)) {
        return startScaleConstrainedMove(e, ScaleHandle::X);
    }
    if (pointOnScaleGizmo(ScaleHandle::Uniform, e.fPos, invScale)) {
        return startScaleConstrainedMove(e, ScaleHandle::Uniform);
    }
    return false;
}

bool Canvas::startAxisConstrainedMove(const eMouseEvent &e, AxisConstraint axis)
{
    if (mCurrentMode != CanvasMode::boxTransform &&
        mCurrentMode != CanvasMode::pointTransform) { return false; }
    if (mSelectedBoxes.isEmpty() && mSelectedPoints_d.isEmpty()) { return false; }

    mValueInput.clearAndDisableInput();
    mValueInput.setupMove();
    mValueInput.setForce1D(true);
    if (axis == AxisConstraint::X) {
        mValueInput.setXOnlyMode();
    } else {
        mValueInput.setYOnlyMode();
    }

    mTransMode = TransformMode::move;
    mDoubleClick = false;
    mStartTransform = true;
    mAxisConstraint = axis;
    mAxisHandleActive = true;
    setAxisGizmoHover(axis, true);
    setGizmosSuppressed(true);
    e.fGrabMouse();
    return true;
}

bool Canvas::tryStartAxisGizmo(const eMouseEvent &e, qreal invScale)
{
    updateRotateHandleGeometry(invScale);
    if (!mRotateHandleVisible) { return false; }

    if (pointOnAxisGizmo(AxisConstraint::Y, e.fPos, invScale)) {
        return startAxisConstrainedMove(e, AxisConstraint::Y);
    }
    if (pointOnAxisGizmo(AxisConstraint::X, e.fPos, invScale)) {
        return startAxisConstrainedMove(e, AxisConstraint::X);
    }
    return false;
}

bool Canvas::tryStartRotateWithGizmo(const eMouseEvent &e, qreal invScale)
{
    updateRotateHandleGeometry(invScale);
    const bool hovered = pointOnRotateGizmo(e.fPos, invScale);
    setRotateHandleHover(hovered);
    if (!hovered) { return false; }

    if (!prepareRotation(e.fPos, true)) {
        setRotateHandleHover(false);
        return false;
    }
    e.fGrabMouse();
    return true;
}

bool Canvas::startScalingAction(const eKeyEvent &e)
{
    if (mCurrentMode != CanvasMode::boxTransform &&
        mCurrentMode != CanvasMode::pointTransform) { return false; }

    if (mSelectedBoxes.isEmpty()) { return false; }
    if (mCurrentMode == CanvasMode::pointTransform) {
        if (mSelectedPoints_d.isEmpty()) { return false; }
    }
    mValueInput.clearAndDisableInput();
    mValueInput.setupScale();

    mRotPivot->setMousePos(e.fPos);
    mTransMode = TransformMode::scale;
    mDoubleClick = false;
    mStartTransform = true;
    e.fGrabMouse();
    return true;
}

bool Canvas::startMovingAction(const eKeyEvent &e)
{
    if (mCurrentMode != CanvasMode::boxTransform &&
        mCurrentMode != CanvasMode::pointTransform) { return false; }
    mValueInput.clearAndDisableInput();
    mValueInput.setupMove();

    mTransMode = TransformMode::move;
    mDoubleClick = false;
    mStartTransform = true;
    e.fGrabMouse();
    return true;
}

void Canvas::selectAllBoxesAction()
{
    mCurrentContainer->selectAllBoxesFromBoxesGroup();
}

void Canvas::deselectAllBoxesAction()
{
    mCurrentContainer->deselectAllBoxesFromBoxesGroup();
}

void Canvas::selectAllPointsAction()
{
    const auto adder = [this](MovablePoint* const pt) {
        addPointToSelection(pt);
    };
    for (const auto& box : mSelectedBoxes) {
        box->selectAllCanvasPts(adder, mCurrentMode);
    }
}

void Canvas::selectOnlyLastPressedBox()
{
    clearBoxesSelection();
    if (mPressedBox) { addBoxToSelection(mPressedBox); }
}

void Canvas::selectOnlyLastPressedPoint()
{
    clearPointsSelection();
    if (mPressedPoint) { addPointToSelection(mPressedPoint); }
}

bool Canvas::SWT_shouldBeVisible(const SWT_RulesCollection &rules,
                                 const bool parentSatisfies,
                                 const bool parentMainTarget) const
{
    Q_UNUSED(parentSatisfies)
    Q_UNUSED(parentMainTarget)
    const SWT_BoxRule rule = rules.fRule;
    const bool alwaysShowChildren = rules.fAlwaysShowChildren;
    if (alwaysShowChildren) {
        return false;
    } else {
        if (rules.fType == SWT_Type::sound) { return false; }

        if (rule == SWT_BoxRule::all) {
            return true;
        } else if (rule == SWT_BoxRule::selected) {
            return false;
        } else if (rule == SWT_BoxRule::animated) {
            return false;
        } else if (rule == SWT_BoxRule::notAnimated) {
            return false;
        } else if (rule == SWT_BoxRule::visible) {
            return true;
        } else if (rule == SWT_BoxRule::hidden) {
            return false;
        } else if (rule == SWT_BoxRule::locked) {
            return false;
        } else if (rule == SWT_BoxRule::unlocked) {
            return true;
        }
    }
    return false;
}

int Canvas::getCurrentFrame() const
{
    return anim_getCurrentAbsFrame();
}

HddCachableCacheHandler &Canvas::getSoundCacheHandler()
{
    return mSoundComposition->getCacheHandler();
}

void Canvas::startDurationRectPosTransformForAllSelected()
{
    for (const auto &box : mSelectedBoxes) {
        box->startDurationRectPosTransform();
    }
}

void Canvas::finishDurationRectPosTransformForAllSelected()
{
    for (const auto &box : mSelectedBoxes) {
        box->finishDurationRectPosTransform();
    }
}

void Canvas::cancelDurationRectPosTransformForAllSelected()
{
    for (const auto &box : mSelectedBoxes) {
        box->cancelDurationRectPosTransform();
    }
}

void Canvas::moveDurationRectForAllSelected(const int dFrame)
{
    for (const auto& box : mSelectedBoxes) {
        box->moveDurationRect(dFrame);
    }
}

void Canvas::startMinFramePosTransformForAllSelected()
{
    for (const auto& box : mSelectedBoxes) {
        box->startMinFramePosTransform();
    }
}

void Canvas::finishMinFramePosTransformForAllSelected()
{
    for (const auto& box : mSelectedBoxes) {
        box->finishMinFramePosTransform();
    }
}

void Canvas::cancelMinFramePosTransformForAllSelected()
{
    for (const auto& box : mSelectedBoxes) {
        box->cancelMinFramePosTransform();
    }
}

void Canvas::moveMinFrameForAllSelected(const int dFrame)
{
    for (const auto& box : mSelectedBoxes) {
        box->moveMinFrame(dFrame);
    }
}

void Canvas::startMaxFramePosTransformForAllSelected()
{
    for (const auto& box : mSelectedBoxes) {
        box->startMaxFramePosTransform();
    }
}

void Canvas::finishMaxFramePosTransformForAllSelected()
{
    for (const auto& box : mSelectedBoxes) {
        box->finishMaxFramePosTransform();
    }
}


void Canvas::cancelMaxFramePosTransformForAllSelected()
{
    for (const auto& box : mSelectedBoxes) {
        box->cancelMaxFramePosTransform();
    }
}

void Canvas::moveMaxFrameForAllSelected(const int dFrame)
{
    for (const auto& box : mSelectedBoxes) {
        box->moveMaxFrame(dFrame);
    }
}

bool Canvas::newUndoRedoSet()
{
    return mUndoRedoStack->newCollection();
}

void Canvas::undo()
{
    mUndoRedoStack->undo();
}

void Canvas::redo()
{
    mUndoRedoStack->redo();
}

UndoRedoStack::StackBlock Canvas::blockUndoRedo()
{
    return mUndoRedoStack->blockUndoRedo();
}

void Canvas::addUndoRedo(const QString& name,
                         const stdfunc<void()>& undo,
                         const stdfunc<void()>& redo)
{
    qDebug() << "addUndoRedo" << name;
    mUndoRedoStack->addUndoRedo(name, undo, redo);
}

void Canvas::pushUndoRedoName(const QString& name) const
{
    qDebug() << "pushUndoRedoName" << name;
    mUndoRedoStack->pushName(name);
}

SoundComposition *Canvas::getSoundComposition()
{
    return mSoundComposition.get();
}

void Canvas::writeSettings(eWriteStream& dst) const
{
    dst << getCurrentFrame();
    dst << mClipToCanvasSize;
    dst << mWidth;
    dst << mHeight;
    dst << mFps;
    dst << mRange;

    writeMarkers(dst);
}

void Canvas::readSettings(eReadStream& src)
{
    int currFrame;
    src >> currFrame;
    src >> mClipToCanvasSize;
    src >> mWidth;
    src >> mHeight;
    src >> mFps;
    FrameRange range;
    src >> range;
    if (src.evFileVersion() >= EvFormat::markers) {
        readMarkers(src);
    }
    setFrameRange(range, false);
    anim_setAbsFrame(currFrame);
}

void Canvas::writeBoundingBox(eWriteStream& dst) const
{
    writeGradients(dst);
    ContainerBox::writeBoundingBox(dst);
    clearGradientRWIds();
}

void Canvas::readBoundingBox(eReadStream& src)
{
    if (src.evFileVersion() > 5) { readGradients(src); }
    ContainerBox::readBoundingBox(src);
    if (src.evFileVersion() < EvFormat::readSceneSettingsBeforeContent) {
        readSettings(src);
    }
    clearGradientRWIds();
}

void Canvas::writeMarkers(eWriteStream &dst) const
{
    dst << mIn.enabled;
    dst << mIn.frame;
    dst << mOut.enabled;
    dst << mOut.frame;
    QStringList markers;
    for (auto &marker: mMarkers) {
        QString title = marker.title.isEmpty() ? tr("Marker") : marker.title;
        markers << QString("%1:%2:%3").arg(title,
                                           QString::number(marker.frame),
                                           QString::number(marker.enabled ? 1 : 0));
    }
    dst << markers.join(",").toUtf8();
}

void Canvas::readMarkers(eReadStream &src)
{
    src >> mIn.enabled;
    src >> mIn.frame;
    src >> mOut.enabled;
    src >> mOut.frame;
    QByteArray markerData;
    src >> markerData;
    mMarkers.clear();
    const auto markers = QString::fromUtf8(markerData).split(",");
    for (auto &marker: markers) {
        const auto content = marker.split(":");
        if (content.size() >= 2) {
            const QString title = content.at(0).isEmpty() ? tr("Marker") : content.at(0);
            const bool enabled = content.size() > 2 ? content.at(2).toInt() : true;
            const int frame = content.at(1).toInt();
            if (hasMarker(frame)) { continue; }
            mMarkers.push_back({title.simplified(), enabled, frame});
        }
    }
}

void Canvas::writeBoxOrSoundXEV(const stdsptr<XevZipFileSaver>& xevFileSaver,
                                const RuntimeIdToWriteId& objListIdConv,
                                const QString& path) const
{
    ContainerBox::writeBoxOrSoundXEV(xevFileSaver, objListIdConv, path);
    auto& fileSaver = xevFileSaver->fileSaver();
    fileSaver.processText(path + "gradients.xml",
                          [&](QTextStream& stream) {
        QDomDocument doc;
        auto gradients = doc.createElement("Gradients");
        int id = 0;
        const auto exp = enve::make_shared<XevExporter>(
                    doc, xevFileSaver, objListIdConv, path);
        for (const auto &grad : mGradients) {
            auto gradient = grad->prp_writePropertyXEV(*exp);
            gradient.setAttribute("id", id++);
            gradients.appendChild(gradient);
        }
        doc.appendChild(gradients);

        stream << doc.toString();
    });
}

void Canvas::readBoxOrSoundXEV(XevReadBoxesHandler& boxReadHandler,
                               ZipFileLoader &fileLoader,
                               const QString &path,
                               const RuntimeIdToWriteId& objListIdConv)
{
    ContainerBox::readBoxOrSoundXEV(boxReadHandler, fileLoader, path, objListIdConv);
    fileLoader.process(path + "gradients.xml",
                       [&](QIODevice* const src) {
        QDomDocument doc;
        doc.setContent(src);
        const auto root = doc.firstChildElement("Gradients");
        const auto gradients = root.elementsByTagName("Gradient");
        for (int i = 0; i < gradients.count(); i++) {
            const auto node = gradients.at(i);
            const auto ele = node.toElement();
            const XevImporter imp(boxReadHandler, fileLoader, objListIdConv, path);
            createNewGradient()->prp_readPropertyXEV(ele, imp);
        }
    });
}

int Canvas::getByteCountPerFrame()
{
    return qCeil(mWidth*mResolution)*qCeil(mHeight*mResolution)*4;
}

void Canvas::readGradients(eReadStream& src)
{
    int nGrads; src >> nGrads;
    for (int i = 0; i < nGrads; i++) {
        createNewGradient()->read(src);
    }
}

void Canvas::writeGradients(eWriteStream &dst) const
{
    dst << mGradients.count();
    int id = 0;
    for (const auto &grad : mGradients) {
        grad->write(id++, dst);
    }
}

SceneBoundGradient *Canvas::createNewGradient()
{
    prp_pushUndoRedoName(tr("Create Gradient"));
    const auto grad = enve::make_shared<SceneBoundGradient>(this);
    addGradient(grad);
    return grad.get();
}

void Canvas::addGradient(const qsptr<SceneBoundGradient>& grad)
{
    prp_pushUndoRedoName(tr("Add Gradient"));
    mGradients.append(grad);
    emit gradientCreated(grad.get());
    {
        UndoRedo ur;
        ur.fUndo = [this, grad]() {
            removeGradient(grad);
        };
        ur.fRedo = [this, grad]() {
            addGradient(grad);
        };
        prp_addUndoRedo(ur);
    }
}

bool Canvas::removeGradient(const qsptr<SceneBoundGradient> &gradient)
{
    const auto guard = gradient;
    if (mGradients.removeOne(gradient)) {
        prp_pushUndoRedoName(tr("Remove Gradient"));
        {
            UndoRedo ur;
            ur.fUndo = [this, guard]() {
                addGradient(guard);
            };
            ur.fRedo = [this, guard]() {
                removeGradient(guard);
            };
            prp_addUndoRedo(ur);
        }
        emit gradient->removed();
        emit gradientRemoved(gradient.data());
        return true;
    }
    return false;
}

SceneBoundGradient *Canvas::getGradientWithRWId(const int rwId) const
{
    for (const auto &grad : mGradients) {
        if (grad->getReadWriteId() == rwId) { return grad.get(); }
    }
    return nullptr;
}

SceneBoundGradient *Canvas::getGradientWithDocumentId(const int id) const
{
    for (const auto &grad : mGradients) {
        if (grad->getDocumentId() == id) { return grad.get(); }
    }
    return nullptr;
}

SceneBoundGradient *Canvas::getGradientWithDocumentSceneId(const int id) const
{
    for (const auto &scene : mDocument.fScenes) {
        for (const auto &grad : scene->mGradients) {
            if (grad->getDocumentId() == id) { return grad.get(); }
        }
    }
    return nullptr;
}

void Canvas::addNullObject(NullObject* const obj)
{
    mNullObjects.append(obj);
}

void Canvas::removeNullObject(NullObject* const obj)
{
    mNullObjects.removeOne(obj);
}

void Canvas::clearGradientRWIds() const
{
    SimpleTask::sScheduleContexted(this, [this]() {
        for (const auto &grad : mGradients) {
            grad->clearReadWriteId();
        }
    });
}
