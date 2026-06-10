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
#include "Animators/SmartPath/smartpathanimator.h"
#include "Animators/transformanimator.h"
#include "Boxes/boundingbox.h"
#include "Boxes/circle.h"
#include "Boxes/containerbox.h"
#include "Boxes/rectangle.h"
#include "Boxes/smartvectorpath.h"
#include "canvas.h"
#include "exceptions.h"
#include "svgimporter.h"

namespace {

struct AnimationTrack {
    QString targetId;
    QString targetName;
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
    const QString existing = target.attribute("data-friction-animation-target");
    if (!existing.isEmpty()) { return existing; }
    const QString id = QString("__friction_svg_animation_%1").arg(nextId++);
    const QString name = target.attribute("inkscape:label",
                                          target.attribute("id",
                                                           target.tagName()));
    target.setAttribute("data-friction-animation-target", id);
    target.setAttribute("data-friction-animation-name", name);
    target.setAttribute("inkscape:label", id);
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
            track.targetName = target.attribute("data-friction-animation-name");
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

AnimationTrack offsetTrack(const AnimationTrack& source, const qreal offset)
{
    AnimationTrack result = source;
    result.values.clear();
    for (const QString& value : source.values) {
        bool ok = false;
        const qreal number = value.trimmed().toDouble(&ok);
        if (!ok) { return AnimationTrack(); }
        result.values.append(QString::number(number + offset));
    }
    return result;
}

const AnimationTrack* findTrack(const QList<AnimationTrack>& tracks,
                                const QString& targetId,
                                const QString& attribute)
{
    for (const AnimationTrack& track : tracks) {
        if (track.targetId == targetId && track.attribute == attribute) {
            return &track;
        }
    }
    return nullptr;
}

AnimationTrack sumTracks(const AnimationTrack& primary,
                         const AnimationTrack* secondary,
                         const qreal secondaryFallback)
{
    if (!secondary ||
        secondary->values.size() != primary.values.size() ||
        secondary->times != primary.times ||
        !qFuzzyCompare(secondary->begin + 1, primary.begin + 1) ||
        !qFuzzyCompare(secondary->duration + 1, primary.duration + 1)) {
        return offsetTrack(primary, secondaryFallback);
    }
    AnimationTrack result = primary;
    result.values.clear();
    for (int i = 0; i < primary.values.size(); ++i) {
        bool primaryOk = false;
        bool secondaryOk = false;
        const qreal primaryValue = primary.values.at(i).trimmed().toDouble(&primaryOk);
        const qreal secondaryValue =
                secondary->values.at(i).trimmed().toDouble(&secondaryOk);
        if (!primaryOk || !secondaryOk) { return AnimationTrack(); }
        result.values.append(QString::number(primaryValue + secondaryValue));
    }
    return result;
}

void applyCircleTrack(Circle* circle, const AnimationTrack& track,
                      const qreal fps)
{
    if (track.attribute == "cx") {
        applyScalarTrack(circle->getCenterAnimator()->getXAnimator(), track, fps);
    } else if (track.attribute == "cy") {
        applyScalarTrack(circle->getCenterAnimator()->getYAnimator(), track, fps);
    } else if (track.attribute == "rx") {
        applyScalarTrack(circle->getHRadiusAnimator()->getXAnimator(), track, fps);
    } else if (track.attribute == "ry") {
        applyScalarTrack(circle->getVRadiusAnimator()->getYAnimator(), track, fps);
    } else if (track.attribute == "r") {
        applyScalarTrack(circle->getHRadiusAnimator()->getXAnimator(), track, fps);
        applyScalarTrack(circle->getVRadiusAnimator()->getYAnimator(), track, fps);
    }
}

void applyRectangleTrack(RectangleBox* rectangle, const AnimationTrack& track,
                         const QList<AnimationTrack>& tracks, const qreal fps)
{
    const QPointF topLeft = rectangle->getTopLeftAnimator()->getBaseValue();
    const QPointF bottomRight = rectangle->getBottomRightAnimator()->getBaseValue();
    if (track.attribute == "x") {
        applyScalarTrack(rectangle->getTopLeftAnimator()->getXAnimator(),
                         track, fps);
        if (!findTrack(tracks, track.targetId, "width")) {
            applyScalarTrack(rectangle->getBottomRightAnimator()->getXAnimator(),
                             offsetTrack(track, bottomRight.x() - topLeft.x()), fps);
        }
    } else if (track.attribute == "y") {
        applyScalarTrack(rectangle->getTopLeftAnimator()->getYAnimator(),
                         track, fps);
        if (!findTrack(tracks, track.targetId, "height")) {
            applyScalarTrack(rectangle->getBottomRightAnimator()->getYAnimator(),
                             offsetTrack(track, bottomRight.y() - topLeft.y()), fps);
        }
    } else if (track.attribute == "width") {
        applyScalarTrack(rectangle->getBottomRightAnimator()->getXAnimator(),
                         sumTracks(track, findTrack(tracks, track.targetId, "x"),
                                   topLeft.x()), fps);
    } else if (track.attribute == "height") {
        applyScalarTrack(rectangle->getBottomRightAnimator()->getYAnimator(),
                         sumTracks(track, findTrack(tracks, track.targetId, "y"),
                                   topLeft.y()), fps);
    } else if (track.attribute == "rx") {
        applyScalarTrack(rectangle->getRadiusAnimator()->getXAnimator(),
                         track, fps);
    } else if (track.attribute == "ry") {
        applyScalarTrack(rectangle->getRadiusAnimator()->getYAnimator(),
                         track, fps);
    }
}

void applyPathTrack(SmartVectorPath* vectorPath, const AnimationTrack& track,
                    const qreal fps)
{
    if (track.attribute != "d") { return; }
    const auto collection = vectorPath->getPathAnimator();
    if (!collection || collection->ca_getNumberOfChildren() != 1) { return; }
    const auto animator = collection->getChild(0);
    QList<SmartPathKey*> keys;
    for (int i = 0; i < track.values.size(); ++i) {
        SkPath path;
        const auto pathString = track.values.at(i).trimmed().toStdString();
        if (!SkParsePath::FromSVGString(pathString.c_str(), &path)) { return; }
        const int frame = qRound((track.begin + track.times.at(i) *
                                  track.duration) * fps);
        const auto key = enve::make_shared<SmartPathKey>(SmartPath(path),
                                                         frame, animator);
        animator->anim_appendKey(key);
        keys.append(key.get());
    }
    if (track.calcMode != "spline") { return; }
    for (int i = 0; i + 1 < keys.size() && i < track.splines.size(); ++i) {
        const auto& spline = track.splines.at(i);
        if (spline.size() != 4) { continue; }
        const qreal frame0 = keys.at(i)->getRelFrame();
        const qreal frame1 = keys.at(i + 1)->getRelFrame();
        const qreal span = frame1 - frame0;
        keys.at(i)->setC1Enabled(true);
        keys.at(i)->setC1Frame(frame0 + spline.at(0) * span);
        keys.at(i)->setC1Value(frame0 + spline.at(1) * span);
        keys.at(i + 1)->setC0Enabled(true);
        keys.at(i + 1)->setC0Frame(frame0 + spline.at(2) * span);
        keys.at(i + 1)->setC0Value(frame0 + spline.at(3) * span);
    }
}

void applyTrack(BoundingBox* box, const AnimationTrack& track,
                const QList<AnimationTrack>& tracks, const qreal fps)
{
    if (!box) { return; }
    if (const auto circle = enve_cast<Circle*>(box)) {
        applyCircleTrack(circle, track, fps);
    }
    if (const auto rectangle = enve_cast<RectangleBox*>(box)) {
        applyRectangleTrack(rectangle, track, tracks, fps);
    }
    if (const auto vectorPath = enve_cast<SmartVectorPath*>(box)) {
        applyPathTrack(vectorPath, track, fps);
    }

    const auto transform = box->getBoxTransformAnimator();
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

    QHash<QString, BoundingBox*> targets;
    for (const AnimationTrack& track : tracks) {
        if (!targets.contains(track.targetId)) {
            targets.insert(track.targetId, findBox(result.get(), track.targetId));
        }
        applyTrack(targets.value(track.targetId), track, tracks, scene->getFps());
    }
    for (const AnimationTrack& track : tracks) {
        BoundingBox* const target = targets.value(track.targetId);
        if (target && !track.targetName.isEmpty() &&
            target->prp_getName() != track.targetName) {
            target->prp_setName(track.targetName);
        }
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
