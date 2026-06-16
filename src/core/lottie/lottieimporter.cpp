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

#include "lottie/lottieimporter.h"

#include "Animators/SmartPath/smartpathanimator.h"
#include "Animators/SmartPath/smartpathcollection.h"
#include "Animators/coloranimator.h"
#include "Animators/paintsettingsanimator.h"
#include "Animators/qpointfanimator.h"
#include "Animators/qrealanimator.h"
#include "Animators/transformanimator.h"
#include "Boxes/circle.h"
#include "Boxes/containerbox.h"
#include "Boxes/pathbox.h"
#include "Boxes/rectangle.h"
#include "Boxes/smartvectorpath.h"
#include "canvas.h"
#include "exceptions.h"
#include "paintsettings.h"

#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QtMath>

namespace {

struct KeyValue {
    int frame = 0;
    QJsonValue value;
    bool hold = false;
};

struct PaintStyle {
    bool hasFill = false;
    QColor fill = Qt::black;
    bool hasStroke = false;
    QColor stroke = Qt::black;
    qreal strokeWidth = 1;
};

QJsonObject readLottieRoot(const QString& filename)
{
    if (filename.endsWith(QStringLiteral(".lottie"), Qt::CaseInsensitive)) {
        RuntimeThrow("dotLottie import is not implemented yet. Use Lottie JSON.");
    }

    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
        RuntimeThrow("Cannot open Lottie file " + filename);
    }
    QJsonParseError error;
    const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &error);
    if (error.error != QJsonParseError::NoError || !document.isObject()) {
        RuntimeThrow("Cannot parse Lottie file " + filename);
    }
    return document.object();
}

QJsonValue propertyValue(const QJsonObject& property)
{
    if (!property.contains(QStringLiteral("k"))) { return {}; }
    const QJsonValue key = property.value(QStringLiteral("k"));
    if (!key.isArray() || property.value(QStringLiteral("a")).toInt() == 0) {
        return key;
    }

    const QJsonArray keys = key.toArray();
    if (keys.isEmpty()) { return {}; }
    return keys.first().toObject().value(QStringLiteral("s"));
}

QList<KeyValue> propertyKeys(const QJsonObject& property)
{
    QList<KeyValue> result;
    const QJsonValue key = property.value(QStringLiteral("k"));
    if (!key.isArray() || property.value(QStringLiteral("a")).toInt() == 0) {
        result.append({0, key, false});
        return result;
    }

    const QJsonArray keys = key.toArray();
    for (const QJsonValue& value : keys) {
        const QJsonObject keyObject = value.toObject();
        if (!keyObject.contains(QStringLiteral("s"))) { continue; }
        result.append({qRound(keyObject.value(QStringLiteral("t")).toDouble()),
                       keyObject.value(QStringLiteral("s")),
                       keyObject.value(QStringLiteral("h")).toInt() == 1});
    }
    return result;
}

qreal numberAt(const QJsonArray& array, const int index, const qreal fallback = 0)
{
    return index < array.size() ? array.at(index).toDouble(fallback) : fallback;
}

qreal scalarValue(const QJsonValue& value, const qreal fallback = 0)
{
    if (value.isArray()) {
        const QJsonArray array = value.toArray();
        return array.isEmpty() ? fallback : array.first().toDouble(fallback);
    }
    return value.toDouble(fallback);
}

QPointF pointValue(const QJsonValue& value, const QPointF& fallback = QPointF())
{
    const QJsonArray array = value.toArray();
    if (array.isEmpty()) { return fallback; }
    return QPointF(numberAt(array, 0, fallback.x()),
                   numberAt(array, 1, fallback.y()));
}

QColor colorValue(const QJsonValue& value, const QColor& fallback = Qt::black)
{
    const QJsonArray array = value.toArray();
    if (array.size() < 3) { return fallback; }
    QColor color;
    color.setRgbF(qBound(0., numberAt(array, 0), 1.),
                  qBound(0., numberAt(array, 1), 1.),
                  qBound(0., numberAt(array, 2), 1.),
                  qBound(0., numberAt(array, 3, 1), 1.));
    return color;
}

int importFrame(const int lottieFrame, const int inPoint, Canvas* const scene)
{
    return scene->getMinFrame() + lottieFrame - inPoint;
}

void applyScalarKeys(QrealAnimator* const animator,
                     const QJsonObject& property,
                     Canvas* const scene,
                     const int inPoint,
                     const qreal multiplier = 1)
{
    if (!animator) { return; }
    const QList<KeyValue> keys = propertyKeys(property);
    if (keys.size() == 1) {
        animator->setCurrentBaseValue(scalarValue(keys.first().value) * multiplier);
        return;
    }
    for (int i = 0; i < keys.size(); ++i) {
        const int frame = importFrame(keys.at(i).frame, inPoint, scene);
        if (keys.at(i).hold && i + 1 < keys.size() && frame + 1 < keys.at(i + 1).frame) {
            animator->saveValueToKey(importFrame(keys.at(i + 1).frame, inPoint, scene) - 1,
                                     scalarValue(keys.at(i).value) * multiplier);
        }
        animator->saveValueToKey(frame, scalarValue(keys.at(i).value) * multiplier);
    }
}

void applyPointKeys(QPointFAnimator* const animator,
                    const QJsonObject& property,
                    Canvas* const scene,
                    const int inPoint,
                    const QPointF& multiplier = QPointF(1, 1))
{
    if (!animator) { return; }
    const QList<KeyValue> keys = propertyKeys(property);
    if (keys.size() == 1) {
        const QPointF value = pointValue(keys.first().value);
        animator->setBaseValue(value.x() * multiplier.x(),
                               value.y() * multiplier.y());
        return;
    }
    for (const KeyValue& key : keys) {
        const QPointF value = pointValue(key.value);
        const int frame = importFrame(key.frame, inPoint, scene);
        animator->getXAnimator()->saveValueToKey(frame, value.x() * multiplier.x());
        animator->getYAnimator()->saveValueToKey(frame, value.y() * multiplier.y());
    }
}

void applyColorKeys(ColorAnimator* const animator,
                    const QJsonObject& property,
                    Canvas* const scene,
                    const int inPoint)
{
    if (!animator) { return; }
    animator->setColorMode(ColorMode::rgb);
    const QList<KeyValue> keys = propertyKeys(property);
    if (keys.size() == 1) {
        animator->setColor(colorValue(keys.first().value));
        return;
    }
    for (const KeyValue& key : keys) {
        const QColor color = colorValue(key.value);
        const int frame = importFrame(key.frame, inPoint, scene);
        animator->getVal1Animator()->saveValueToKey(frame, color.redF());
        animator->getVal2Animator()->saveValueToKey(frame, color.greenF());
        animator->getVal3Animator()->saveValueToKey(frame, color.blueF());
        animator->getAlphaAnimator()->saveValueToKey(frame, color.alphaF());
    }
}

void applyTransform(BoundingBox* const box,
                    const QJsonObject& transform,
                    Canvas* const scene,
                    const int inPoint)
{
    if (!box) { return; }
    const auto animator = box->getBoxTransformAnimator();
    if (!animator) { return; }

    applyPointKeys(animator->getPosAnimator(),
                   transform.value(QStringLiteral("p")).toObject(),
                   scene, inPoint);
    applyPointKeys(animator->getScaleAnimator(),
                   transform.value(QStringLiteral("s")).toObject(),
                   scene, inPoint, QPointF(0.01, 0.01));
    applyScalarKeys(animator->getRotAnimator(),
                    transform.value(QStringLiteral("r")).toObject(),
                    scene, inPoint);
    applyScalarKeys(animator->getRotAnimator(),
                    transform.value(QStringLiteral("rz")).toObject(),
                    scene, inPoint);
    applyScalarKeys(animator->getOpacityAnimator(),
                    transform.value(QStringLiteral("o")).toObject(),
                    scene, inPoint);
    applyPointKeys(animator->getPivotAnimator(),
                   transform.value(QStringLiteral("a")).toObject(),
                   scene, inPoint);
}

SkPath pathFromShapeValue(const QJsonValue& value)
{
    QJsonObject object = value.toObject();
    if (object.isEmpty() && value.isArray()) {
        const QJsonArray array = value.toArray();
        if (!array.isEmpty()) {
            object = array.first().toObject();
        }
    }
    const QJsonArray vertices = object.value(QStringLiteral("v")).toArray();
    const QJsonArray inTangents = object.value(QStringLiteral("i")).toArray();
    const QJsonArray outTangents = object.value(QStringLiteral("o")).toArray();
    const bool closed = object.value(QStringLiteral("c")).toBool();
    SkPath path;
    if (vertices.isEmpty()) { return path; }

    const QPointF first = pointValue(vertices.first());
    path.moveTo(first.x(), first.y());
    for (int i = 1; i < vertices.size(); ++i) {
        const QPointF previous = pointValue(vertices.at(i - 1));
        const QPointF current = pointValue(vertices.at(i));
        const QPointF out = pointValue(outTangents.at(i - 1));
        const QPointF in = pointValue(inTangents.at(i));
        if (out.isNull() && in.isNull()) {
            path.lineTo(current.x(), current.y());
        } else {
            path.cubicTo(previous.x() + out.x(), previous.y() + out.y(),
                         current.x() + in.x(), current.y() + in.y(),
                         current.x(), current.y());
        }
    }
    if (closed) {
        const int lastIndex = vertices.size() - 1;
        const QPointF last = pointValue(vertices.last());
        const QPointF out = pointValue(outTangents.at(lastIndex));
        const QPointF in = pointValue(inTangents.first());
        if (!out.isNull() || !in.isNull()) {
            path.cubicTo(last.x() + out.x(), last.y() + out.y(),
                         first.x() + in.x(), first.y() + in.y(),
                         first.x(), first.y());
        }
        path.close();
    }
    return path;
}

SkPath pathFromPolystar(const QJsonObject& shape)
{
    const int starType = qRound(scalarValue(shape.value(QStringLiteral("sy")), 1));
    const int pointCount = qMax(3, qRound(scalarValue(propertyValue(
                                      shape.value(QStringLiteral("pt")).toObject()), 5)));
    const QPointF center = pointValue(propertyValue(
                                          shape.value(QStringLiteral("p")).toObject()));
    const qreal rotation = qDegreesToRadians(scalarValue(propertyValue(
                                             shape.value(QStringLiteral("r")).toObject()), 0) - 90);
    const qreal outerRadius = scalarValue(propertyValue(
                                shape.value(QStringLiteral("or")).toObject()), 0);
    const qreal innerRadius = scalarValue(propertyValue(
                                shape.value(QStringLiteral("ir")).toObject()), outerRadius);
    const int vertexCount = starType == 1 ? pointCount*2 : pointCount;

    SkPath path;
    if (outerRadius <= 0 || vertexCount <= 0) { return path; }

    for (int i = 0; i < vertexCount; ++i) {
        const bool innerPoint = starType == 1 && i % 2 == 1;
        const qreal radius = innerPoint ? innerRadius : outerRadius;
        const qreal angle = rotation + 2*M_PI*i/vertexCount;
        const qreal x = center.x() + qCos(angle)*radius;
        const qreal y = center.y() + qSin(angle)*radius;
        if (i == 0) {
            path.moveTo(x, y);
        } else {
            path.lineTo(x, y);
        }
    }
    path.close();
    return path;
}

void applyPathKeys(SmartVectorPath* const box,
                   const QJsonObject& property,
                   Canvas* const scene,
                   const int inPoint)
{
    if (!box) { return; }
    const auto collection = box->getPathAnimator();
    if (!collection || collection->ca_getNumberOfChildren() != 1) { return; }
    const auto animator = collection->getChild(0);
    const QList<KeyValue> keys = propertyKeys(property);
    if (keys.size() <= 1) { return; }

    for (const KeyValue& key : keys) {
        const SkPath path = pathFromShapeValue(key.value);
        const auto pathKey = enve::make_shared<SmartPathKey>(
                    SmartPath(path), importFrame(key.frame, inPoint, scene),
                    animator);
        animator->anim_appendKey(pathKey);
    }
}

void applyPaint(PathBox* const box,
                const PaintStyle& style,
                Canvas* const scene,
                const int inPoint,
                const QJsonObject& fill = {},
                const QJsonObject& stroke = {})
{
    if (!box) { return; }
    const auto fillSettings = box->getFillSettings();
    if (fillSettings) {
        if (style.hasFill) {
            fillSettings->setPaintType(PaintType::FLATPAINT);
            fillSettings->setCurrentColor(style.fill);
            if (!fill.isEmpty()) {
                applyColorKeys(fillSettings->getColorAnimator(),
                               fill.value(QStringLiteral("c")).toObject(),
                               scene, inPoint);
                applyScalarKeys(fillSettings->getColorAnimator()->getAlphaAnimator(),
                                fill.value(QStringLiteral("o")).toObject(),
                                scene, inPoint, 0.01);
            }
        } else {
            fillSettings->setPaintType(PaintType::NOPAINT);
        }
    }

    const auto strokeSettings = box->getStrokeSettings();
    if (strokeSettings) {
        if (style.hasStroke) {
            strokeSettings->setPaintType(PaintType::FLATPAINT);
            strokeSettings->setCurrentColor(style.stroke);
            strokeSettings->getLineWidthAnimator()->setCurrentBaseValue(
                        style.strokeWidth);
            if (!stroke.isEmpty()) {
                applyColorKeys(strokeSettings->getColorAnimator(),
                               stroke.value(QStringLiteral("c")).toObject(),
                               scene, inPoint);
                applyScalarKeys(strokeSettings->getColorAnimator()->getAlphaAnimator(),
                                stroke.value(QStringLiteral("o")).toObject(),
                                scene, inPoint, 0.01);
                applyScalarKeys(strokeSettings->getLineWidthAnimator(),
                                stroke.value(QStringLiteral("w")).toObject(),
                                scene, inPoint);
            }
        } else {
            strokeSettings->setPaintType(PaintType::NOPAINT);
        }
    }
}

QJsonObject firstItemOfType(const QJsonArray& items, const QString& type)
{
    for (const QJsonValue& value : items) {
        const QJsonObject item = value.toObject();
        if (item.value(QStringLiteral("ty")).toString() == type) { return item; }
    }
    return {};
}

PaintStyle paintStyleForItems(const QJsonArray& items)
{
    PaintStyle style;
    const QJsonObject fill = firstItemOfType(items, QStringLiteral("fl"));
    if (!fill.isEmpty()) {
        style.hasFill = true;
        style.fill = colorValue(propertyValue(fill.value(QStringLiteral("c")).toObject()));
        style.fill.setAlphaF(qBound(0., scalarValue(propertyValue(
                                      fill.value(QStringLiteral("o")).toObject()), 100.) / 100., 1.));
    }
    const QJsonObject stroke = firstItemOfType(items, QStringLiteral("st"));
    if (!stroke.isEmpty()) {
        style.hasStroke = true;
        style.stroke = colorValue(propertyValue(stroke.value(QStringLiteral("c")).toObject()));
        style.stroke.setAlphaF(qBound(0., scalarValue(propertyValue(
                                        stroke.value(QStringLiteral("o")).toObject()), 100.) / 100., 1.));
        style.strokeWidth = scalarValue(propertyValue(
                                            stroke.value(QStringLiteral("w")).toObject()), 1);
    }
    return style;
}

qsptr<SmartVectorPath> createPathBox(const QJsonObject& shape,
                                     const PaintStyle& style,
                                     const QJsonObject& fill,
                                     const QJsonObject& stroke,
                                     Canvas* const scene,
                                     const int inPoint)
{
    const QJsonObject property = shape.value(QStringLiteral("ks")).toObject();
    const SkPath path = pathFromShapeValue(propertyValue(property));
    const auto box = enve::make_shared<SmartVectorPath>();
    box->prp_setName(shape.value(QStringLiteral("nm")).toString(
                         QStringLiteral("Path")));
    box->loadSkPath(path);
    applyPathKeys(box.get(), property, scene, inPoint);
    applyPaint(box.get(), style, scene, inPoint, fill, stroke);
    return box;
}

qsptr<SmartVectorPath> createPolystarBox(const QJsonObject& shape,
                                         const PaintStyle& style,
                                         const QJsonObject& fill,
                                         const QJsonObject& stroke,
                                         Canvas* const scene,
                                         const int inPoint)
{
    const SkPath path = pathFromPolystar(shape);
    const auto box = enve::make_shared<SmartVectorPath>();
    box->prp_setName(shape.value(QStringLiteral("nm")).toString(
                         QStringLiteral("Polystar")));
    box->loadSkPath(path);
    applyPaint(box.get(), style, scene, inPoint, fill, stroke);
    return box;
}

qsptr<RectangleBox> createRectangleBox(const QJsonObject& shape,
                                       const PaintStyle& style,
                                       const QJsonObject& fill,
                                       const QJsonObject& stroke,
                                       Canvas* const scene,
                                       const int inPoint)
{
    const QPointF position = pointValue(propertyValue(
                                            shape.value(QStringLiteral("p")).toObject()));
    const QPointF size = pointValue(propertyValue(
                                        shape.value(QStringLiteral("s")).toObject()));
    const QPointF radius(scalarValue(propertyValue(
                                         shape.value(QStringLiteral("r")).toObject())), 0);
    const auto box = enve::make_shared<RectangleBox>();
    box->prp_setName(shape.value(QStringLiteral("nm")).toString(
                         QStringLiteral("Rectangle")));
    box->setTopLeftPos(QPointF(position.x() - size.x()*0.5,
                               position.y() - size.y()*0.5));
    box->setBottomRightPos(QPointF(position.x() + size.x()*0.5,
                                   position.y() + size.y()*0.5));
    box->setXRadius(radius.x());
    box->setYRadius(radius.x());
    applyPointKeys(box->getTopLeftAnimator(),
                   shape.value(QStringLiteral("p")).toObject(),
                   scene, inPoint);
    applyPaint(box.get(), style, scene, inPoint, fill, stroke);
    return box;
}

qsptr<Circle> createEllipseBox(const QJsonObject& shape,
                               const PaintStyle& style,
                               const QJsonObject& fill,
                               const QJsonObject& stroke,
                               Canvas* const scene,
                               const int inPoint)
{
    const QPointF position = pointValue(propertyValue(
                                            shape.value(QStringLiteral("p")).toObject()));
    const QPointF size = pointValue(propertyValue(
                                        shape.value(QStringLiteral("s")).toObject()));
    const auto box = enve::make_shared<Circle>();
    box->prp_setName(shape.value(QStringLiteral("nm")).toString(
                         QStringLiteral("Ellipse")));
    box->setCenter(position);
    box->setHorizontalRadius(size.x()*0.5);
    box->setVerticalRadius(size.y()*0.5);
    applyPointKeys(box->getCenterAnimator(),
                   shape.value(QStringLiteral("p")).toObject(),
                   scene, inPoint);
    applyPaint(box.get(), style, scene, inPoint, fill, stroke);
    return box;
}

void importShapeItems(const QJsonArray& items,
                      ContainerBox* const parent,
                      Canvas* const scene,
                      const int inPoint);

qsptr<ContainerBox> createGroup(const QJsonObject& group,
                                Canvas* const scene,
                                const int inPoint)
{
    const auto box = enve::make_shared<ContainerBox>(
                group.value(QStringLiteral("nm")).toString(QStringLiteral("Group")),
                eBoxType::group);
    const QJsonArray items = group.value(QStringLiteral("it")).toArray();
    importShapeItems(items, box.get(), scene, inPoint);
    const QJsonObject transform = firstItemOfType(items, QStringLiteral("tr"));
    if (!transform.isEmpty()) { applyTransform(box.get(), transform, scene, inPoint); }
    return box;
}

void importShapeItems(const QJsonArray& items,
                      ContainerBox* const parent,
                      Canvas* const scene,
                      const int inPoint)
{
    if (!parent) { return; }
    const PaintStyle style = paintStyleForItems(items);
    const QJsonObject fill = firstItemOfType(items, QStringLiteral("fl"));
    const QJsonObject stroke = firstItemOfType(items, QStringLiteral("st"));

    for (const QJsonValue& value : items) {
        const QJsonObject item = value.toObject();
        const QString type = item.value(QStringLiteral("ty")).toString();
        qsptr<eBoxOrSound> imported;
        if (type == QStringLiteral("gr")) {
            imported = createGroup(item, scene, inPoint);
        } else if (type == QStringLiteral("sh")) {
            imported = createPathBox(item, style, fill, stroke, scene, inPoint);
        } else if (type == QStringLiteral("sr")) {
            imported = createPolystarBox(item, style, fill, stroke, scene, inPoint);
        } else if (type == QStringLiteral("rc")) {
            imported = createRectangleBox(item, style, fill, stroke, scene, inPoint);
        } else if (type == QStringLiteral("el")) {
            imported = createEllipseBox(item, style, fill, stroke, scene, inPoint);
        }
        if (imported) { parent->addContained(imported); }
    }
}

qsptr<BoundingBox> importShapeLayer(const QJsonObject& layer,
                                    Canvas* const scene,
                                    const int inPoint)
{
    const auto box = enve::make_shared<ContainerBox>(
                layer.value(QStringLiteral("nm")).toString(QStringLiteral("Shape Layer")),
                eBoxType::group);
    importShapeItems(layer.value(QStringLiteral("shapes")).toArray(),
                     box.get(), scene, inPoint);
    applyTransform(box.get(), layer.value(QStringLiteral("ks")).toObject(),
                   scene, inPoint);
    return box;
}

void analyzeShapes(const QJsonArray& shapes, ImportLottie::Analysis& result)
{
    for (const QJsonValue& value : shapes) {
        const QJsonObject shape = value.toObject();
        const QString type = shape.value(QStringLiteral("ty")).toString();
        if (type == QStringLiteral("gr")) {
            analyzeShapes(shape.value(QStringLiteral("it")).toArray(), result);
            continue;
        }
        if (type == QStringLiteral("tr") || type == QStringLiteral("fl") ||
            type == QStringLiteral("st")) {
            continue;
        }
        ++result.totalShapes;
        if (type == QStringLiteral("sh") || type == QStringLiteral("sr") ||
            type == QStringLiteral("rc") ||
            type == QStringLiteral("el")) {
            ++result.supportedShapes;
        } else {
            result.unsupported.append(QStringLiteral("shape:%1").arg(type));
        }
    }
}

} // namespace

ImportLottie::Analysis ImportLottie::analyzeFile(const QString& filename)
{
    const QJsonObject root = readLottieRoot(filename);
    Analysis result;
    result.fps = root.value(QStringLiteral("fr")).toDouble(0);
    result.firstFrame = qRound(root.value(QStringLiteral("ip")).toDouble(0));
    result.lastFrame = qRound(root.value(QStringLiteral("op")).toDouble(0)) - 1;
    result.duration = result.fps > 0 ?
                (result.lastFrame - result.firstFrame + 1) / result.fps : 0;
    result.size = QSize(root.value(QStringLiteral("w")).toInt(),
                        root.value(QStringLiteral("h")).toInt());

    const QJsonArray layers = root.value(QStringLiteral("layers")).toArray();
    for (const QJsonValue& value : layers) {
        const QJsonObject layer = value.toObject();
        ++result.totalLayers;
        const int type = layer.value(QStringLiteral("ty")).toInt(-1);
        if (type == 4) {
            ++result.supportedLayers;
            analyzeShapes(layer.value(QStringLiteral("shapes")).toArray(), result);
        } else {
            result.unsupported.append(QStringLiteral("layer:%1").arg(type));
        }
    }
    result.unsupported.removeDuplicates();
    return result;
}

qsptr<BoundingBox> ImportLottie::loadFile(const QString& filename,
                                          Canvas* const scene,
                                          const SceneDurationMode durationMode)
{
    if (!scene) { RuntimeThrow("Lottie import requires an active scene"); }
    const QJsonObject root = readLottieRoot(filename);
    const int inPoint = qRound(root.value(QStringLiteral("ip")).toDouble(0));
    const int outPoint = qRound(root.value(QStringLiteral("op")).toDouble(0));
    const auto result = enve::make_shared<ContainerBox>(
                QFileInfo(filename).completeBaseName(), eBoxType::group);

    const QJsonArray layers = root.value(QStringLiteral("layers")).toArray();
    for (int i = layers.size() - 1; i >= 0; --i) {
        const QJsonObject layer = layers.at(i).toObject();
        if (layer.value(QStringLiteral("ty")).toInt(-1) != 4) { continue; }
        const auto imported = importShapeLayer(layer, scene, inPoint);
        if (imported) { result->addContained(imported); }
    }

    const int importedLastFrame = scene->getMinFrame() + qMax(1, outPoint - inPoint);
    if (durationMode == SceneDurationMode::fitImportedLottie && outPoint > inPoint) {
        scene->setFrameRange({scene->getMinFrame(), importedLastFrame});
    } else if (durationMode == SceneDurationMode::extendIfNeeded &&
               importedLastFrame > scene->getMaxFrame()) {
        scene->setFrameRange({scene->getMinFrame(), importedLastFrame});
    }
    return result;
}
