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

#ifndef SVGANIMATIONIMPORTDIALOG_H
#define SVGANIMATIONIMPORTDIALOG_H

#include <QDialog>
#include <QSizeF>

class QComboBox;
class QCheckBox;
class QRadioButton;

class SVGAnimationImportDialog : public QDialog {
public:
    enum class ScaleMode {
        original,
        fitWidth,
        fitHeight,
        stretch
    };

    SVGAnimationImportDialog(const QSizeF& svgSize, const QSize& sceneSize,
                             QWidget* const parent);

    ScaleMode scaleMode() const;
    bool extendSceneTime() const;

    static bool sExec(const QString& path, const QSize& sceneSize,
                      QSizeF& svgSize, ScaleMode& scaleMode,
                      bool& extendSceneTime,
                      QWidget* const parent);

private:
    static QSizeF readSVGSize(const QString& path);

    QRadioButton* mOriginal;
    QRadioButton* mProportional;
    QRadioButton* mStretch;
    QComboBox* mFitDimension;
    QCheckBox* mExtendSceneTime;
};

#endif // SVGANIMATIONIMPORTDIALOG_H
