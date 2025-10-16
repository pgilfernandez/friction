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

#include "gridsettingsdialog.h"

#include "gridcontroller.h"
#include "GUI/coloranimatorbutton.h"

#include <QFormLayout>
#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QColor>
#include <QPushButton>

using Friction::Core::GridSettings;

namespace {
constexpr double kMinSpacing = 1.0;
constexpr double kMaxSpacing = 10000.0;
constexpr double kOriginRange = 100000.0;
constexpr int kMaxSnapThreshold = 200;
constexpr int kMaxMajorEvery = 100;
}

GridSettingsDialog::GridSettingsDialog(QWidget* parent)
    : QDialog(parent)
    , mSizeX(nullptr)
    , mSizeY(nullptr)
    , mOriginX(nullptr)
    , mOriginY(nullptr)
    , mSnapThreshold(nullptr)
    , mMajorEvery(nullptr)
    , mButtonBox(nullptr)
    , mColorButton(nullptr)
    , mMajorColorButton(nullptr)
    , mColorAnimator(enve::make_shared<ColorAnimator>())
    , mMajorColorAnimator(enve::make_shared<ColorAnimator>())
    , mSnapEnabled(true)
{
    mColorAnimator->setColor(QColor(255, 255, 255, 96));
    mMajorColorAnimator->setColor(QColor(255, 255, 255, 160));
    setupUi();
}

void GridSettingsDialog::setupUi()
{
    setWindowTitle(tr("Grid Settings"));
    auto* layout = new QVBoxLayout(this);

    auto* form = new QFormLayout();
    form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

    mSizeX = new QDoubleSpinBox(this);
    mSizeX->setDecimals(2);
    mSizeX->setRange(kMinSpacing, kMaxSpacing);
    mSizeX->setSingleStep(1.0);
    form->addRow(tr("Spacing X"), mSizeX);

    mSizeY = new QDoubleSpinBox(this);
    mSizeY->setDecimals(2);
    mSizeY->setRange(kMinSpacing, kMaxSpacing);
    mSizeY->setSingleStep(1.0);
    form->addRow(tr("Spacing Y"), mSizeY);

    mOriginX = new QDoubleSpinBox(this);
    mOriginX->setDecimals(2);
    mOriginX->setRange(-kOriginRange, kOriginRange);
    mOriginX->setSingleStep(1.0);
    form->addRow(tr("Origin X"), mOriginX);

    mOriginY = new QDoubleSpinBox(this);
    mOriginY->setDecimals(2);
    mOriginY->setRange(-kOriginRange, kOriginRange);
    mOriginY->setSingleStep(1.0);
    form->addRow(tr("Origin Y"), mOriginY);

    mSnapThreshold = new QSpinBox(this);
    mSnapThreshold->setRange(0, kMaxSnapThreshold);
    mSnapThreshold->setSingleStep(1);
    form->addRow(tr("Snap Threshold (px)"), mSnapThreshold);

    mMajorEvery = new QSpinBox(this);
    mMajorEvery->setRange(1, kMaxMajorEvery);
    mMajorEvery->setSingleStep(1);
    form->addRow(tr("Major Line Every"), mMajorEvery);

    mColorButton = new ColorAnimatorButton(mColorAnimator.get(), this);
    form->addRow(tr("Grid Color"), mColorButton);

    mMajorColorButton = new ColorAnimatorButton(mMajorColorAnimator.get(), this);
    form->addRow(tr("Major Line Color"), mMajorColorButton);

    layout->addLayout(form);

    mButtonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    auto* restoreButton = mButtonBox->addButton(tr("Restore Defaults"), QDialogButtonBox::ResetRole);
    connect(restoreButton, &QPushButton::clicked, this, &GridSettingsDialog::restoreDefaults);
    connect(mButtonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(mButtonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    layout->addWidget(mButtonBox);
}

void GridSettingsDialog::setSettings(const GridSettings& settings)
{
    mSnapEnabled = settings.enabled;
    mSizeX->setValue(settings.sizeX);
    mSizeY->setValue(settings.sizeY);
    mOriginX->setValue(settings.originX);
    mOriginY->setValue(settings.originY);
    mSnapThreshold->setValue(settings.snapThresholdPx);
    mMajorEvery->setValue(settings.majorEvery);
    mStoredShow = settings.show;

    const auto ensureAnimator = [](qsptr<ColorAnimator>& animator,
                                   ColorAnimatorButton* button)
    {
        if (!animator) {
            animator = enve::make_shared<ColorAnimator>();
            if (button) {
                button->setColorTarget(animator.get());
            }
        }
    };

    ensureAnimator(mColorAnimator, mColorButton);
    ensureAnimator(mMajorColorAnimator, mMajorColorButton);

    const QColor appliedColor = settings.colorAnimator
        ? settings.colorAnimator->getColor()
        : QColor(255, 255, 255, 96);
    mColorAnimator->setColor(appliedColor);

    const QColor appliedMajorColor = settings.majorColorAnimator
        ? settings.majorColorAnimator->getColor()
        : QColor(255, 255, 255, 160);
    mMajorColorAnimator->setColor(appliedMajorColor);
}

GridSettings GridSettingsDialog::settings() const
{
    GridSettings result;
    result.enabled = mSnapEnabled;
    result.sizeX = mSizeX->value();
    result.sizeY = mSizeY->value();
    result.originX = mOriginX->value();
    result.originY = mOriginY->value();
    result.snapThresholdPx = mSnapThreshold->value();
    result.majorEvery = mMajorEvery->value();
    result.show = mStoredShow;

    const QColor finalColor = mColorAnimator
        ? mColorAnimator->getColor()
        : QColor(255, 255, 255, 96);
    result.colorAnimator = enve::make_shared<ColorAnimator>();
    result.colorAnimator->setColor(finalColor);

    const QColor finalMajorColor = mMajorColorAnimator
        ? mMajorColorAnimator->getColor()
        : QColor(255, 255, 255, 160);
    result.majorColorAnimator = enve::make_shared<ColorAnimator>();
    result.majorColorAnimator->setColor(finalMajorColor);
    return result;
}

void GridSettingsDialog::restoreDefaults()
{
    auto defaults = GridSettings{};
    defaults.show = mStoredShow;
    setSettings(defaults);
}
