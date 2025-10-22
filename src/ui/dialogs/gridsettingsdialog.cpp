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
#include "GUI/global.h"
#include "Private/esettings.h"

#include <QFormLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QSpinBox>
#include <QPushButton>
#include <QColor>

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
    , mMajorEveryX(nullptr)
    , mMajorEveryY(nullptr)
    , mSaveAsDefault(nullptr)
    , mOkButton(nullptr)
    , mCancelButton(nullptr)
    , mColorButton(nullptr)
    , mMajorColorButton(nullptr)
    , mColorAnimator(enve::make_shared<ColorAnimator>())
    , mMajorColorAnimator(enve::make_shared<ColorAnimator>())
    , mSnapEnabled(true)
{
    const QColor defaultMinor = GridSettings::defaults().colorAnimator->getColor();
    const QColor defaultMajor = GridSettings::defaults().majorColorAnimator->getColor();
    if (auto* settings = eSettings::sInstance) {
        mColorAnimator->setColor(settings->fGridColor);
        mMajorColorAnimator->setColor(settings->fGridMajorColor);
    } else {
        mColorAnimator->setColor(defaultMinor);
        mMajorColorAnimator->setColor(defaultMajor);
    }
    setupUi();
}

void GridSettingsDialog::setupUi()
{
    setWindowTitle(tr("Grid Settings"));
    auto* layout = new QVBoxLayout(this);

    auto* form = new QFormLayout();
    form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);

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

    mSnapThreshold = new QSpinBox(this);
    mSnapThreshold->setRange(0, kMaxSnapThreshold);
    mSnapThreshold->setSingleStep(1);
    form->addRow(tr("Snap radius"), mSnapThreshold);

    mMajorEveryX = new QSpinBox(this);
    mMajorEveryX->setRange(1, kMaxMajorEvery);
    mMajorEveryX->setSingleStep(1);
    form->addRow(tr("Major line every X"), mMajorEveryX);

    mMajorEveryY = new QSpinBox(this);
    mMajorEveryY->setRange(1, kMaxMajorEvery);
    mMajorEveryY->setSingleStep(1);
    form->addRow(tr("Major line every Y"), mMajorEveryY);

    mColorButton = new ColorAnimatorButton(mColorAnimator.get(), this);
    auto* minorColorContainer = new QWidget(this);
    auto* minorColorLayout = new QHBoxLayout(minorColorContainer);
    minorColorLayout->setContentsMargins(0, 0, 0, 0);
    minorColorLayout->addStretch();
    minorColorLayout->addWidget(mColorButton);
    form->addRow(tr("Minor grid line color"), minorColorContainer);

    mMajorColorButton = new ColorAnimatorButton(mMajorColorAnimator.get(), this);
    auto* majorColorContainer = new QWidget(this);
    auto* majorColorLayout = new QHBoxLayout(majorColorContainer);
    majorColorLayout->setContentsMargins(0, 0, 0, 0);
    majorColorLayout->addStretch();
    majorColorLayout->addWidget(mMajorColorButton);
    form->addRow(tr("Major grid line color"), majorColorContainer);

    layout->addLayout(form);
    eSizesUI::widget.addSpacing(layout);

    mSaveAsDefault = new QCheckBox(tr("Save as default"), this);
    layout->addWidget(mSaveAsDefault);

    mOkButton = new QPushButton(QIcon::fromTheme("dialog-ok"), tr("Ok"), this);
    mCancelButton = new QPushButton(QIcon::fromTheme("dialog-cancel"), tr("Cancel"), this);

    auto* buttonLayout = new QHBoxLayout();
    layout->addLayout(buttonLayout);

    buttonLayout->addWidget(mOkButton);
    buttonLayout->addWidget(mCancelButton);

    connect(mOkButton, &QPushButton::released,
            this, &GridSettingsDialog::accept);
    connect(mCancelButton, &QPushButton::released,
            this, &GridSettingsDialog::reject);
    connect(this, &QDialog::rejected, this, &QDialog::close);
}

void GridSettingsDialog::setSettings(const GridSettings& settings)
{
    mSnapEnabled = settings.enabled;
    mSizeX->setValue(settings.sizeX);
    mSizeY->setValue(settings.sizeY);
    mOriginX->setValue(settings.originX);
    mOriginY->setValue(settings.originY);
    mSnapThreshold->setValue(settings.snapThresholdPx);
    mMajorEveryX->setValue(settings.majorEveryX);
    mMajorEveryY->setValue(settings.majorEveryY);
    mStoredShow = settings.show;
    mStoredDrawOnTop = settings.drawOnTop;
    if (mSaveAsDefault) {
        mSaveAsDefault->setChecked(false);
    }

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
        : GridSettings::defaults().colorAnimator->getColor();
    mColorAnimator->setColor(appliedColor);

    const QColor appliedMajorColor = settings.majorColorAnimator
        ? settings.majorColorAnimator->getColor()
        : GridSettings::defaults().majorColorAnimator->getColor();
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
    result.majorEveryX = mMajorEveryX->value();
    result.majorEveryY = mMajorEveryY->value();
    result.show = mStoredShow;
    result.drawOnTop = mStoredDrawOnTop;

    const QColor finalColor = mColorAnimator
        ? mColorAnimator->getColor()
        : GridSettings::defaults().colorAnimator->getColor();
    result.colorAnimator = enve::make_shared<ColorAnimator>();
    result.colorAnimator->setColor(finalColor);

    const QColor finalMajorColor = mMajorColorAnimator
        ? mMajorColorAnimator->getColor()
        : GridSettings::defaults().majorColorAnimator->getColor();
    result.majorColorAnimator = enve::make_shared<ColorAnimator>();
    result.majorColorAnimator->setColor(finalMajorColor);
    return result;
}

bool GridSettingsDialog::saveAsDefault() const
{
    return mSaveAsDefault && mSaveAsDefault->isChecked();
}
