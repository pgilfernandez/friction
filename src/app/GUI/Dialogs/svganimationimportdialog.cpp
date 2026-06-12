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
#include <QFormLayout>
#include <QLabel>
#include <QRadioButton>
#include <QRegularExpression>
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

}

SVGAnimationImportDialog::SVGAnimationImportDialog(
        const QSizeF& svgSize, const QSize& sceneSize, QWidget* const parent) :
    QDialog(parent) {
    setWindowTitle(tr("Import SVG Animation"));

    const auto mainLayout = new QVBoxLayout(this);
    const auto dimensions = new QFormLayout();
    dimensions->addRow(tr("Original SVG dimensions:"), new QLabel(sizeText(svgSize)));
    dimensions->addRow(tr("Active scene dimensions:"),
                       new QLabel(sizeText(sceneSize)));
    mainLayout->addLayout(dimensions);

    mOriginal = new QRadioButton(tr("Import at original scale"), this);
    mProportional = new QRadioButton(
                tr("Scale proportionally to the active scene"), this);
    mStretch = new QRadioButton(
                tr("Scale to the active scene and deform the original"), this);
    mFitDimension = new QComboBox(this);
    mFitDimension->addItem(tr("Fit width"));
    mFitDimension->addItem(tr("Fit height"));

    mOriginal->setChecked(true);
    mFitDimension->setEnabled(false);
    connect(mProportional, &QRadioButton::toggled,
            mFitDimension, &QComboBox::setEnabled);

    const bool validSize = svgSize.width() > 0 && svgSize.height() > 0 &&
            sceneSize.width() > 0 && sceneSize.height() > 0;
    mProportional->setEnabled(validSize);
    mStretch->setEnabled(validSize);

    mainLayout->addWidget(mOriginal);
    mainLayout->addWidget(mProportional);
    mainLayout->addWidget(mFitDimension);
    mainLayout->addWidget(mStretch);

    mExtendSceneTime = new QCheckBox(
                tr("Extend scene time if necessary"), this);
    mExtendSceneTime->setChecked(true);
    mainLayout->addWidget(mExtendSceneTime);

    const auto buttons = new QDialogButtonBox(QDialogButtonBox::Ok |
                                              QDialogButtonBox::Cancel);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttons);
}

SVGAnimationImportDialog::ScaleMode SVGAnimationImportDialog::scaleMode() const {
    if (mStretch->isChecked()) { return ScaleMode::stretch; }
    if (mProportional->isChecked()) {
        return mFitDimension->currentIndex() == 0 ?
                    ScaleMode::fitWidth : ScaleMode::fitHeight;
    }
    return ScaleMode::original;
}

bool SVGAnimationImportDialog::extendSceneTime() const {
    return mExtendSceneTime->isChecked();
}

bool SVGAnimationImportDialog::sExec(
        const QString& path, const QSize& sceneSize, QSizeF& svgSize,
        ScaleMode& scaleMode, bool& extendSceneTime, QWidget* const parent) {
    svgSize = readSVGSize(path);
    SVGAnimationImportDialog dialog(svgSize, sceneSize, parent);
    if (dialog.exec() != QDialog::Accepted) { return false; }
    scaleMode = dialog.scaleMode();
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
