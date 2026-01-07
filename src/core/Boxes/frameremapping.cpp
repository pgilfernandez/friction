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

#include "frameremapping.h"
#include "Animators/qrealkey.h"
#include "ReadWrite/evformat.h"
#include "XML/xmlexporthelpers.h"

#include <cmath>

namespace {
FrameRemappingBase::FrameRemappingMode toFrameRemapMode(const int value) {
    using Mode = FrameRemappingBase::FrameRemappingMode;
    switch (value) {
    case static_cast<int>(Mode::loop):
        return Mode::loop;
    case static_cast<int>(Mode::bounce):
        return Mode::bounce;
    default:
        return Mode::manual;
    }
}
}

FrameRemappingBase::FrameRemappingBase() :
    QrealAnimator("frame") {}

void FrameRemappingBase::disableAction() {
    setEnabled(false);
    anim_setRecording(false);
}

void FrameRemappingBase::setFrameCount(const int count) {
    setValueRange(0, count - 1);
}

void FrameRemappingBase::setMode(FrameRemappingMode mode) {
    if (mMode == mode) { return; }
    mMode = mode;
    updateVisibility();
    prp_afterWholeInfluenceRangeChanged();
    emit modeChanged(mMode);
}

void FrameRemappingBase::prp_readProperty_impl(eReadStream &src) {
    bool enabled; src >> enabled;
    FrameRemappingMode mode = FrameRemappingMode::manual;
    if (src.evFileVersion() >= EvFormat::frameRemappingMode) {
        int storedMode; src >> storedMode;
        mode = toFrameRemapMode(storedMode);
    }
    setMode(mode);
    setEnabled(enabled);
    QrealAnimator::prp_readProperty_impl(src);
}

void FrameRemappingBase::prp_writeProperty_impl(eWriteStream &dst) const {
    dst << mEnabled;
    dst << static_cast<int>(mMode);
    QrealAnimator::prp_writeProperty_impl(dst);
}

QDomElement FrameRemappingBase::prp_writePropertyXEV_impl(const XevExporter& exp) const {
    auto result = QrealAnimator::prp_writePropertyXEV_impl(exp);
    result.setAttribute("enabled", mEnabled ? "true" : "false");
    result.setAttribute("mode", static_cast<int>(mMode));
    return result;
}

void FrameRemappingBase::prp_readPropertyXEV_impl(const QDomElement& ele, const XevImporter& imp) {
    QrealAnimator::prp_readPropertyXEV_impl(ele, imp);
    const auto enabled = ele.attribute("enabled");
    const auto modeAttr = ele.attribute("mode");
    const int modeVal = XmlExportHelpers::stringToInt(modeAttr);
    setMode(toFrameRemapMode(modeVal));
    setEnabled(enabled == "true");
}

void FrameRemappingBase::enableAction(const int minFrame,
                                      const int maxFrame,
                                      const int animStartRelFrame)
{
    if (mEnabled) { return; }
    prp_pushUndoRedoName(tr("Enable Frame Remapping"));
    setValueRange(minFrame, maxFrame);
    if (maxFrame > minFrame) {
        const int firstValue = minFrame;
        const int firstFrame = animStartRelFrame + minFrame;
        const auto firstFrameKey = enve::make_shared<QrealKey>(firstValue,
                                                               firstFrame,
                                                               this);
        anim_appendKey(firstFrameKey);
        const int lastValue = maxFrame;
        const int lastFrame = animStartRelFrame + maxFrame;
        const auto lastFrameKey = enve::make_shared<QrealKey>(lastValue,
                                                              lastFrame,
                                                              this);
        anim_appendKey(lastFrameKey);
    } else {
        setCurrentBaseValue(0);
    }
    setEnabled(true);
}

void FrameRemappingBase::setEnabled(const bool enabled)
{
    if (mEnabled == enabled) {
        updateVisibility();
        return;
    }
    {
        prp_pushUndoRedoName(tr("Set Frame Remapping"));
        UndoRedo ur;
        const auto oldValue = mEnabled;
        const auto newValue = enabled;
        ur.fUndo = [this, oldValue]() { setEnabled(oldValue); };
        ur.fRedo = [this, newValue]() { setEnabled(newValue); };
        prp_addUndoRedo(ur);
    }
    mEnabled = enabled;
    updateVisibility();
    prp_afterWholeInfluenceRangeChanged();
    emit enabledChanged(mEnabled);
}

void FrameRemappingBase::updateVisibility() {
    SWT_setVisible(mEnabled && mMode == FrameRemappingMode::manual);
}

qreal FrameRemappingBase::remappedFrame(const qreal relFrame) const {
    if (!enabled()) { return relFrame; }

    switch (mMode) {
    case FrameRemappingMode::loop:
        return loopFrame(relFrame);
    case FrameRemappingMode::bounce:
        return bounceFrame(relFrame);
    case FrameRemappingMode::manual:
    default:
        return getEffectiveValue(relFrame);
    }
}

qreal FrameRemappingBase::loopFrame(const qreal relFrame) const {
    const qreal minVal = getMinPossibleValue();
    const qreal maxVal = getMaxPossibleValue();
    const qreal range = maxVal - minVal + 1;
    if (range <= 0) { return minVal; }

    qreal rel = std::fmod(relFrame - minVal, range);
    if (rel < 0) { rel += range; }
    return minVal + rel;
}

qreal FrameRemappingBase::bounceFrame(const qreal relFrame) const {
    const qreal minVal = getMinPossibleValue();
    const qreal maxVal = getMaxPossibleValue();
    const qreal range = maxVal - minVal + 1;
    if (range <= 1) { return minVal; }

    const qreal period = 2 * (range - 1);
    qreal rel = std::fmod(relFrame - minVal, period);
    if (rel < 0) { rel += period; }
    if (rel < range) { return minVal + rel; }
    return minVal + (period - rel);
}

IntFrameRemapping::IntFrameRemapping() {
    setNumberDecimals(0);
}

int IntFrameRemapping::frame(const qreal relFrame) const {
    return qRound(remappedFrame(relFrame));
}

QrealFrameRemapping::QrealFrameRemapping() {}

qreal QrealFrameRemapping::frame(const qreal relFrame) const {
    return remappedFrame(relFrame);
}
