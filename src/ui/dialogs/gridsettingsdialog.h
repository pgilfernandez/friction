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

#ifndef GRIDSETTINGSDIALOG_H
#define GRIDSETTINGSDIALOG_H

#include <QDialog>
#include "Animators/coloranimator.h"
#include "smartPointers/ememory.h"


class QDoubleSpinBox;
class QSpinBox;
class QDialogButtonBox;
class ColorAnimatorButton;

namespace Friction {
namespace Core {
struct GridSettings;
}
}

class GridSettingsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit GridSettingsDialog(QWidget* parent = nullptr);

    void setSettings(const Friction::Core::GridSettings& settings);
    Friction::Core::GridSettings settings() const;

private slots:
    void restoreDefaults();

private:
    void setupUi();

    QDoubleSpinBox* mSizeX;
    QDoubleSpinBox* mSizeY;
    QDoubleSpinBox* mOriginX;
    QDoubleSpinBox* mOriginY;
    QSpinBox* mSnapThreshold;
    QSpinBox* mMajorEvery;
    QDialogButtonBox* mButtonBox;
    ColorAnimatorButton* mColorButton;
    ColorAnimatorButton* mMajorColorButton;
    qsptr<ColorAnimator> mColorAnimator;
    qsptr<ColorAnimator> mMajorColorAnimator;
    bool mSnapEnabled = true;
    bool mStoredShow = true;
};

#endif // GRIDSETTINGSDIALOG_H
