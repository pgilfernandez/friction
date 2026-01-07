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

#include "internallinkcanvas.h"
#include "linkcanvasrenderdata.h"
#include "Animators/transformanimator.h"
#include "canvas.h"
#include "Properties/comboboxproperty.h"
#include "Timeline/animationrect.h"

InternalLinkCanvas::InternalLinkCanvas(ContainerBox * const linkTarget,
                                       const bool innerLink) :
    InternalLinkGroupBox(linkTarget, innerLink) {
    mType = eBoxType::internalLinkCanvas;
    mFrameRemapping->disableAction();
    const QStringList remapModes{tr("manual"), tr("loop"), tr("bounce")};
    mFrameRemappingMode = enve::make_shared<ComboBoxProperty>("frame remapping mode",
                                                              remapModes);
    ca_prependChild(mTransformAnimator.data(), mClipToCanvas);
    ca_prependChild(mTransformAnimator.data(), mFrameRemapping);
    ca_prependChild(mFrameRemapping.get(), mFrameRemappingMode);

    connect(mFrameRemappingMode.get(), &ComboBoxProperty::valueChanged,
            this, [this](const int value) {
        mFrameRemapping->setMode(
                    static_cast<FrameRemappingBase::FrameRemappingMode>(value));
        updateFrameRemappingVisibility();
        updateDurationRangeForRemap();
    });
    connect(mFrameRemapping.get(), &QrealFrameRemapping::enabledChanged,
            this, &InternalLinkCanvas::updateFrameRemappingVisibility);
    connect(mFrameRemapping.get(), &FrameRemappingBase::modeChanged,
            this, [this](FrameRemappingBase::FrameRemappingMode mode) {
        mFrameRemappingMode->setCurrentValueNoUndo(static_cast<int>(mode));
        updateFrameRemappingVisibility();
        updateDurationRangeForRemap();
    });
    connect(mFrameRemapping.get(), &QrealFrameRemapping::enabledChanged,
            this, &InternalLinkCanvas::updateDurationRangeForRemap);
    updateFrameRemappingVisibility();
    updateDurationRangeForRemap();
}

void InternalLinkCanvas::enableFrameRemappingAction() {
    const auto finalTarget = static_cast<Canvas*>(getFinalTarget());
    const int minFrame = finalTarget->getMinFrame();
    const int maxFrame = finalTarget->getMaxFrame();
    mFrameRemapping->enableAction(minFrame, maxFrame, minFrame);
    updateDurationRangeForRemap();
}

void InternalLinkCanvas::disableFrameRemappingAction() {
    mFrameRemapping->disableAction();
    updateDurationRangeForRemap();
}

void InternalLinkCanvas::updateFrameRemappingVisibility() {
    const bool remappingEnabled = mFrameRemapping->enabled();
    if (mFrameRemappingMode) {
        mFrameRemappingMode->SWT_setVisible(remappingEnabled);
    }
}

void InternalLinkCanvas::updateDurationRangeForRemap() {
    const auto target = static_cast<Canvas*>(getFinalTarget());
    if (!target) { return; }
    const bool autoLoop = mFrameRemapping->enabled() &&
            mFrameRemapping->mode() != FrameRemappingBase::FrameRemappingMode::manual;

    auto durRect = getDurationRectangle();
    if(!durRect && !durationRectangleLocked()) {
        createDurationRectangle();
        durRect = getDurationRectangle();
    }
    if(!durRect) { return; }

    const int minFrame = target->getMinFrame();
    const int maxFrame = target->getMaxFrame();
    const int span = qMax(1, maxFrame - minFrame + 1);

    durRect->setMinRelFrame(minFrame);
    durRect->setFramesDuration(autoLoop ? FrameRange::EMAX : span);
    if (auto animRect = dynamic_cast<AnimationRect*>(durRect)) {
        animRect->setAnimationFrameDuration(span);
    }
}

void InternalLinkCanvas::prp_setupTreeViewMenu(PropertyMenu * const menu) {
    const PropertyMenu::CheckSelectedOp<InternalLinkCanvas> remapOp =
    [](InternalLinkCanvas* const box, const bool checked) {
        if(checked) box->enableFrameRemappingAction();
        else box->disableFrameRemappingAction();
    };
    menu->addCheckableAction("Frame Remapping",
                             mFrameRemapping->enabled(),
                             remapOp);

    menu->addSeparator();

    InternalLinkGroupBox::prp_setupTreeViewMenu(menu);
}

void InternalLinkCanvas::setupRenderData(const qreal relFrame,
                                         const QMatrix& parentM,
                                         BoxRenderData * const data,
                                         Canvas* const scene) {
    {
        BoundingBox::setupRenderData(relFrame, parentM, data, scene);
        const qreal remapped = mFrameRemapping->frame(relFrame);
        const auto thisM = getTotalTransformAtFrame(relFrame);
        processChildrenData(remapped, thisM, data, scene);
    }

    ContainerBox* finalTarget = getFinalTarget();
    auto canvasData = static_cast<LinkCanvasRenderData*>(data);
    const auto canvasTarget = static_cast<Canvas*>(finalTarget);
    canvasData->fBgColor = toSkColor(canvasTarget->getBgColorAnimator()->
            getColor(relFrame));
    //qreal res = mParentScene->getResolution();
    canvasData->fCanvasHeight = canvasTarget->getCanvasHeight();//*res;
    canvasData->fCanvasWidth = canvasTarget->getCanvasWidth();//*res;
    if(getParentGroup()->isLink()) {
        const auto ilc = static_cast<InternalLinkCanvas*>(getLinkTarget());
        canvasData->fClipToCanvas = ilc->clipToCanvas();
    } else {
        canvasData->fClipToCanvas = mClipToCanvas->getValue();
    }
}

bool InternalLinkCanvas::clipToCanvas() {
    return mClipToCanvas->getValue();
}

bool InternalLinkCanvas::isFrameInDurationRect(const int relFrame) const {
    const auto target = getFinalTarget();
    if(!target) return false;
    const bool ownRange = InternalLinkGroupBox::isFrameInDurationRect(relFrame);
    if(!ownRange) return false;
    const qreal remapped = mFrameRemapping->enabled()
            ? mFrameRemapping->frame(relFrame)
            : relFrame;
    return target->isFrameInDurationRect(qRound(remapped));
}

bool InternalLinkCanvas::isFrameFInDurationRect(const qreal relFrame) const {
    const auto target = getFinalTarget();
    if(!target) return false;
    const bool ownRange = InternalLinkGroupBox::isFrameFInDurationRect(relFrame);
    if(!ownRange) return false;
    const qreal remapped = mFrameRemapping->enabled()
            ? mFrameRemapping->frame(relFrame)
            : relFrame;
    return target->isFrameFInDurationRect(remapped);
}

qsptr<BoundingBox> InternalLinkCanvas::createLink(const bool inner) {
    auto linkBox = enve::make_shared<InternalLinkCanvas>(this, inner);
    copyTransformationTo(linkBox.get());
    return std::move(linkBox);
}

stdsptr<BoxRenderData> InternalLinkCanvas::createRenderData() {
    return enve::make_shared<LinkCanvasRenderData>(this);
}

bool InternalLinkCanvas::relPointInsidePath(const QPointF &relPos) const {
    if(mClipToCanvas->getValue()) return getRelBoundingRect().contains(relPos);
    return InternalLinkGroupBox::relPointInsidePath(relPos);
}

void InternalLinkCanvas::anim_setAbsFrame(const int frame) {
    InternalLinkGroupBox::anim_setAbsFrame(frame);
    const auto canvasTarget = static_cast<Canvas*>(getFinalTarget());
    if(!canvasTarget) return;
    canvasTarget->anim_setAbsFrame(anim_getCurrentRelFrame());
}
