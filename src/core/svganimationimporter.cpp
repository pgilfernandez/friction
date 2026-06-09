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
*/

#include "svganimationimporter.h"

#include <QDomDocument>
#include <QFile>
#include <QRegularExpression>

#include "Animators/qpointfanimator.h"
#include "Animators/qrealanimator.h"
#include "Animators/qrealkey.h"
#include "Animators/transformanimator.h"
#include "Boxes/boundingbox.h"
#include "Boxes/containerbox.h"
#include "canvas.h"
#include "exceptions.h"
#include "svgimporter.h"

namespace {

struct AnimationTrack {
    QString targetId;
    QString attribute;
    QString type;
    QString calcMode;
    QList<qreal> times;
    QStringList values;
    QList<QList<qreal>> splines;
    qreal begin = 0;
    qreal duration = 0;
};

qreal parseClock(const QString& value, bool* ok = nullptr)
{
    QString text = value.trimmed();
    qreal multiplier = 1;
    if (text.endsWith("ms")) {
        multiplier = 0.001;
        text.chop(2);
    } else if (text.endsWith('s')) {
        text.chop(1);
    }
    bool parsed = false;
    const qreal result = text.toDouble(&parsed) * multiplier;
    if (ok) { *ok = parsed; }
    return parsed ? result : 0;
}

QList<qreal> parseNumbers(const QString& value)
{
    QList<qreal> result;
    const auto parts = value.split(QRegularExpression("[,\\s]+"),
                                   Qt::SkipEmptyParts);
    for (const QString& part : parts) {
        bool ok = false;
        const qreal number = part.toDouble(&ok);
        if (ok) { result.append(number); }
    }
    return result;
}

QList<qreal> parseSemicolonNumbers(const QString& value)
{
    QList<qreal> result;
    for (const QString& part : value.split(';', Qt::SkipEmptyParts)) {
        bool ok = false;
        const qreal number = part.trimmed().toDouble(&ok);
        if (ok) { result.append(number); }
    }
    return result;
}

BoundingBox* findBox(BoundingBox* box, const QString& name)
{
    if (!box) { return nullptr; }
    if (box->prp_getName() == name) { return box; }
    const auto container = enve_cast<ContainerBox*>(box);
    if (!container) { return nullptr; }
    for (BoundingBox* child : container->getContainedBoxes()) {
        if (const auto found = findBox(child, name)) { return found; }
    }
    return nullptr;
}

QString ensureTargetId(QDomElement& target, int& nextId)
{
    QString id = target.attribute("id");
    if (id.isEmpty()) {
        id = QString("__friction_svg_animation_%1").arg(nextId++);
        target.setAttribute("id", id);
    }
    return id;
}

QList<AnimationTrack> collectTracks(QDomDocument& document)
{
    QList<AnimationTrack> result;
    int nextId = 0;
    const QStringList tags{"animate", "animateTransform"};
    for (const QString& tag : tags) {
        const QDomNodeList nodes = document.elementsByTagName(tag);
        for (int i = 0; i < nodes.count(); ++i) {
            const QDomElement animation = nodes.at(i).toElement();
            QDomElement target = animation.parentNode().toElement();
            if (target.isNull()) { continue; }

            AnimationTrack track;
            track.targetId = ensureTargetId(target, nextId);
            track.attribute = animation.attribute("attributeName");
            track.type = animation.attribute("type");
            track.calcMode = animation.attribute("calcMode", "linear");

            bool durationOk = false;
            track.duration = parseClock(animation.attribute("dur"), &durationOk);
            if (!durationOk || track.duration <= 0) { continue; }

            bool beginOk = false;
            track.begin = parseClock(animation.attribute("begin", "0s"), &beginOk);
            if (!beginOk) { track.begin = 0; }

            track.values = animation.attribute("values")
                               .split(';', Qt::SkipEmptyParts);
            if (track.values.isEmpty()) {
                const QString from = animation.attribute("from");
                const QString to = animation.attribute("to");
                if (!from.isEmpty() && !to.isEmpty()) {
                    track.values << from << to;
                }
            }
            if (track.values.size() < 2) { continue; }

            track.times = parseSemicolonNumbers(animation.attribute("keyTimes"));
            if (track.times.size() != track.values.size()) {
                track.times.clear();
                const qreal divisor = track.values.size() - 1;
                for (int valueId = 0; valueId < track.values.size(); ++valueId) {
                    track.times.append(valueId / divisor);
                }
            }

            for (const QString& spline :
                 animation.attribute("keySplines").split(';', Qt::SkipEmptyParts)) {
                track.splines.append(parseNumbers(spline));
            }
            result.append(track);
        }
    }
    return result;
}

void applySpline(QrealAnimator* animator,
                 const int frame0, const int frame1,
                 const qreal value0, const qreal value1,
                 const QList<qreal>& spline)
{
    if (!animator || spline.size() != 4 || frame0 == frame1) { return; }
    auto key0 = animator->anim_getKeyAtRelFrame<QrealKey>(frame0);
    auto key1 = animator->anim_getKeyAtRelFrame<QrealKey>(frame1);
    if (!key0 || !key1) { return; }

    const qreal frameSpan = frame1 - frame0;
    const qreal valueSpan = value1 - value0;
    key0->setC1Enabled(true);
    key0->setC1Frame(frame0 + spline.at(0) * frameSpan);
    key0->setC1Value(value0 + spline.at(1) * valueSpan);
    key1->setC0Enabled(true);
    key1->setC0Frame(frame0 + spline.at(2) * frameSpan);
    key1->setC0Value(value0 + spline.at(3) * valueSpan);
}

void applyScalarTrack(QrealAnimator* animator,
                      const AnimationTrack& track,
                      const qreal fps,
                      const qreal multiplier = 1)
{
    if (!animator) { return; }
    QList<int> frames;
    QList<qreal> values;
    for (int i = 0; i < track.values.size(); ++i) {
        bool ok = false;
        const qreal value = track.values.at(i).trimmed().toDouble(&ok) * multiplier;
        if (!ok) { return; }
        const int frame = qRound((track.begin + track.times.at(i) *
                                  track.duration) * fps);
        animator->saveValueToKey(frame, value);
        frames.append(frame);
        values.append(value);
    }
    if (track.calcMode != "spline") { return; }
    for (int i = 0; i + 1 < frames.size() && i < track.splines.size(); ++i) {
        applySpline(animator, frames.at(i), frames.at(i + 1),
                    values.at(i), values.at(i + 1), track.splines.at(i));
    }
}

void applyPointTrack(QPointFAnimator* animator,
                     const AnimationTrack& track,
                     const qreal fps,
                     const bool uniformSingleValue)
{
    if (!animator) { return; }
    AnimationTrack xTrack = track;
    AnimationTrack yTrack = track;
    xTrack.values.clear();
    yTrack.values.clear();
    for (const QString& value : track.values) {
        const auto numbers = parseNumbers(value);
        if (numbers.isEmpty()) { return; }
        xTrack.values.append(QString::number(numbers.at(0)));
        const qreal y = numbers.size() > 1 ? numbers.at(1) :
                        uniformSingleValue ? numbers.at(0) : 0;
        yTrack.values.append(QString::number(y));
    }
    applyScalarTrack(animator->getXAnimator(), xTrack, fps);
    applyScalarTrack(animator->getYAnimator(), yTrack, fps);
}

void applyTrack(BoundingBox* box, const AnimationTrack& track, const qreal fps)
{
    const auto transform = box ? box->getBoxTransformAnimator() : nullptr;
    if (!transform) { return; }
    if (track.attribute == "opacity") {
        applyScalarTrack(transform->getOpacityAnimator(), track, fps, 100);
    } else if (track.attribute == "transform" && track.type == "translate") {
        applyPointTrack(transform->getPosAnimator(), track, fps, false);
    } else if (track.attribute == "transform" && track.type == "scale") {
        applyPointTrack(transform->getScaleAnimator(), track, fps, true);
    } else if (track.attribute == "transform" && track.type == "rotate") {
        AnimationTrack rotation = track;
        rotation.values.clear();
        for (const QString& value : track.values) {
            const auto numbers = parseNumbers(value);
            if (numbers.isEmpty()) { return; }
            rotation.values.append(QString::number(numbers.first()));
        }
        applyScalarTrack(transform->getRotAnimator(), rotation, fps);
    } else if (track.attribute == "transform" && track.type == "skewX") {
        applyScalarTrack(transform->getShearAnimator()->getXAnimator(),
                         track, fps, 1. / 45.);
    } else if (track.attribute == "transform" && track.type == "skewY") {
        applyScalarTrack(transform->getShearAnimator()->getYAnimator(),
                         track, fps, 1. / 45.);
    }
}

} // namespace

qsptr<BoundingBox> ImportSVGAnimation::loadSVGFile(const QString& filename,
                                                   Canvas* scene)
{
    if (!scene) { RuntimeThrow("SVG animation import requires an active scene"); }
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        RuntimeThrow("Cannot open file " + filename);
    }
    QDomDocument document;
    if (!document.setContent(&file)) {
        RuntimeThrow("Cannot parse SVG animation file " + filename);
    }

    const auto tracks = collectTracks(document);
    const auto gradientCreator = [scene]() { return scene->createNewGradient(); };
    const auto result = ImportSVG::loadSVGFile(document, gradientCreator);
    if (!result) { return nullptr; }

    for (const AnimationTrack& track : tracks) {
        applyTrack(findBox(result.get(), track.targetId), track, scene->getFps());
    }
    int lastFrame = scene->getMaxFrame();
    for (const AnimationTrack& track : tracks) {
        lastFrame = qMax(lastFrame,
                         qRound((track.begin + track.duration) * scene->getFps()));
    }
    if (lastFrame > scene->getMaxFrame()) {
        scene->setFrameRange({scene->getMinFrame(), lastFrame});
    }
    return result;
}
