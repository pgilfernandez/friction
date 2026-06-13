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

#include "svganimationimportdialog.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDomDocument>
#include <QFile>
#include <QGroupBox>
#include <QHeaderView>
#include <QLabel>
#include <QRadioButton>
#include <QRegularExpression>
#include <QStandardItemModel>
#include <QTableWidget>
#include <QVBoxLayout>

namespace {

qreal readDimension(const QString& value) {
    static const QRegularExpression number(
                QStringLiteral("^\\s*([+-]?(?:\\d*\\.)?\\d+(?:[eE][+-]?\\d+)?)"));
    const auto match = number.match(value);
    if (!match.hasMatch() || value.trimmed().endsWith('%')) { return 0; }
    bool ok = false;
    const qreal result = match.captured(1).toDouble(&ok);
    return ok && result > 0 ? result : 0;
}

QString sizeText(const QSizeF& size) {
    if (size.width() <= 0 || size.height() <= 0) {
        return SVGAnimationImportDialog::tr("Unknown");
    }
    return SVGAnimationImportDialog::tr("%1 x %2")
            .arg(size.width(), 0, 'g', 8)
            .arg(size.height(), 0, 'g', 8);
}

QString durationText(const qreal seconds) {
    return SVGAnimationImportDialog::tr("%1 seconds").arg(seconds, 0, 'g', 8);
}

QTableWidgetItem* tableItem(const QString& text) {
    return new QTableWidgetItem(text);
}

}

SVGAnimationImportDialog::SVGAnimationImportDialog(
        const QSizeF& svgSize, const SceneInfo& scene,
        const ImportSVGAnimation::Analysis& analysis, QWidget* const parent) :
    QDialog(parent) {
    setWindowTitle(tr("Import SVG Animation"));

    const auto mainLayout = new QVBoxLayout(this);
    const int sceneFrameCount = scene.lastFrame - scene.firstFrame + 1;
    const qreal sceneDuration = scene.fps > 0 ? sceneFrameCount / scene.fps : 0;
    const int svgFrames = qRound(analysis.duration * scene.fps);

    const auto comparison = new QTableWidget(4, 3, this);
    comparison->setHorizontalHeaderLabels(
                {tr("Property"), tr("SVG"), tr("Active scene")});
    comparison->verticalHeader()->hide();
    comparison->horizontalHeader()->setSectionResizeMode(
                0, QHeaderView::ResizeToContents);
    comparison->horizontalHeader()->setSectionResizeMode(
                1, QHeaderView::Stretch);
    comparison->horizontalHeader()->setSectionResizeMode(
                2, QHeaderView::Stretch);
    comparison->setEditTriggers(QAbstractItemView::NoEditTriggers);
    comparison->setSelectionMode(QAbstractItemView::NoSelection);
    comparison->setFocusPolicy(Qt::NoFocus);
    comparison->setShowGrid(false);

    comparison->setItem(0, 0, tableItem(tr("Dimensions")));
    comparison->setItem(0, 1, tableItem(sizeText(svgSize)));
    comparison->setItem(0, 2, tableItem(sizeText(scene.size)));
    comparison->setItem(1, 0, tableItem(tr("Duration")));
    comparison->setItem(1, 1, tableItem(durationText(analysis.duration)));
    comparison->setItem(1, 2, tableItem(durationText(sceneDuration)));
    comparison->setItem(2, 0, tableItem(tr("Frames")));
    comparison->setItem(2, 1, tableItem(tr("%1 at scene FPS").arg(svgFrames)));
    comparison->setItem(2, 2, tableItem(tr("%1 to %2 (%3 total)")
                                        .arg(scene.firstFrame)
                                        .arg(scene.lastFrame)
                                        .arg(sceneFrameCount)));
    comparison->setItem(3, 0, tableItem(tr("Animation tracks")));
    comparison->setItem(3, 1, tableItem(tr("%1 supported of %2")
                                        .arg(analysis.supportedTracks)
                                        .arg(analysis.totalTracks)));
    comparison->setItem(3, 2, tableItem(tr("Not applicable")));
    comparison->resizeRowsToContents();
    comparison->setMinimumWidth(460);
    comparison->setFixedHeight(comparison->horizontalHeader()->height() +
                               comparison->verticalHeader()->length() + 2);
    mainLayout->addWidget(comparison);

    if (!analysis.unsupported.isEmpty()) {
        const auto warning = new QLabel(
                    tr("Unsupported animations will be skipped: %1")
                    .arg(analysis.unsupported.join(", ")), this);
        warning->setWordWrap(true);
        mainLayout->addWidget(warning);
    }

    const auto scaleGroup = new QGroupBox(tr("Scale mode"), this);
    const auto scaleLayout = new QVBoxLayout(scaleGroup);
    mScaleMode = new QComboBox(scaleGroup);
    mScaleMode->addItem(tr("Don't scale"));
    mScaleMode->addItem(tr("Scale proportionally (width fit)"));
    mScaleMode->addItem(tr("Scale proportionally (height fit)"));
    mScaleMode->addItem(tr("Deform (fit to scene)"));

    const bool validSize = svgSize.width() > 0 && svgSize.height() > 0 &&
            scene.size.width() > 0 && scene.size.height() > 0;
    if (!validSize) {
        const auto model = qobject_cast<QStandardItemModel*>(mScaleMode->model());
        for (int i = 1; model && i < mScaleMode->count(); ++i) {
            model->item(i)->setEnabled(false);
        }
    }

    scaleLayout->addWidget(mScaleMode);
    mainLayout->addWidget(scaleGroup);

    const auto structureGroup = new QGroupBox(tr("Import structure"), this);
    const auto structureLayout = new QVBoxLayout(structureGroup);
    mNamedGroup = new QRadioButton(tr("Grouped"), structureGroup);
    mDirectObjects = new QRadioButton(tr("Ungrouped"), structureGroup);
    mNamedGroup->setChecked(true);
    structureLayout->addWidget(mNamedGroup);
    structureLayout->addWidget(mDirectObjects);
    mainLayout->addWidget(structureGroup);

    mExtendSceneTime = new QCheckBox(
                tr("Extend scene time if necessary"), this);
    const bool extensionNeeded = analysis.duration > sceneDuration;
    mExtendSceneTime->setChecked(extensionNeeded);
    mExtendSceneTime->setEnabled(extensionNeeded);
    mainLayout->addWidget(mExtendSceneTime);

    const auto buttons = new QDialogButtonBox(QDialogButtonBox::Ok |
                                              QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttons);
}

SVGAnimationImportDialog::ScaleMode SVGAnimationImportDialog::scaleMode() const {
    return static_cast<ScaleMode>(mScaleMode->currentIndex());
}

SVGAnimationImportDialog::StructureMode
SVGAnimationImportDialog::structureMode() const {
    return mDirectObjects->isChecked() ?
                StructureMode::directObjects : StructureMode::namedGroup;
}

bool SVGAnimationImportDialog::extendSceneTime() const {
    return mExtendSceneTime->isChecked();
}

bool SVGAnimationImportDialog::sExec(
        const QString& path, const SceneInfo& scene, QSizeF& svgSize,
        ScaleMode& scaleMode, StructureMode& structureMode,
        bool& extendSceneTime, QWidget* const parent) {
    svgSize = readSVGSize(path);
    const auto analysis = ImportSVGAnimation::analyzeSVGFile(path);
    SVGAnimationImportDialog dialog(svgSize, scene, analysis, parent);
    if (dialog.exec() != QDialog::Accepted) { return false; }
    scaleMode = dialog.scaleMode();
    structureMode = dialog.structureMode();
    extendSceneTime = dialog.extendSceneTime();
    return true;
}

QSizeF SVGAnimationImportDialog::readSVGSize(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) { return {}; }

    QDomDocument document;
    if (!document.setContent(&file)) { return {}; }
    const QDomElement svg = document.documentElement();
    if (svg.tagName().compare(QStringLiteral("svg"),
                              Qt::CaseInsensitive) != 0) {
        return {};
    }

    const QStringList viewBox = svg.attribute(QStringLiteral("viewBox"))
            .split(QRegularExpression(QStringLiteral("[,\\s]+")),
                   Qt::SkipEmptyParts);
    if (viewBox.size() == 4) {
        bool widthOk = false;
        bool heightOk = false;
        const qreal width = viewBox.at(2).toDouble(&widthOk);
        const qreal height = viewBox.at(3).toDouble(&heightOk);
        if (widthOk && heightOk && width > 0 && height > 0) {
            return QSizeF(width, height);
        }
    }

    return QSizeF(readDimension(svg.attribute(QStringLiteral("width"))),
                  readDimension(svg.attribute(QStringLiteral("height"))));
}
