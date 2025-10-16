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
#include "gridcontroller.h"

#include <QFormLayout>
#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QPushButton>
#include <QColorDialog>
#include <QHBoxLayout>

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
    , mShowGrid(nullptr)
    , mButtonBox(nullptr)
    , mColorButton(nullptr)
    , mAlphaSpin(nullptr)
    , mColorAnimator(enve::make_shared<ColorAnimator>())
    , mSnapEnabled(true)
    , mCurrentColor(QColor(255, 255, 255, 96))
{
    mColorAnimator->setColor(mCurrentColor);
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

    auto* colorLayout = new QHBoxLayout();
    mColorButton = new QPushButton(tr("Select Color"), this);
    mColorButton->setToolTip(tr("Pick grid line color"));
    colorLayout->addWidget(mColorButton);
    mAlphaSpin = new QSpinBox(this);
    mAlphaSpin->setRange(0, 255);
    mAlphaSpin->setSingleStep(1);
    mAlphaSpin->setToolTip(tr("Opacity (0-255)"));
    mAlphaSpin->setValue(mCurrentColor.alpha());
    colorLayout->addWidget(mAlphaSpin);
    colorLayout->addStretch();
    form->addRow(tr("Grid Color"), colorLayout);

    mShowGrid = new QCheckBox(tr("Show grid"), this);
    form->addRow(QString(), mShowGrid);

    layout->addLayout(form);

    mButtonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    auto* restoreButton = mButtonBox->addButton(tr("Restore Defaults"), QDialogButtonBox::ResetRole);
    connect(restoreButton, &QPushButton::clicked, this, &GridSettingsDialog::restoreDefaults);
    connect(mColorButton, &QPushButton::clicked, this, &GridSettingsDialog::chooseColor);
    mAlphaSpin->setValue(mCurrentColor.alpha());
    connect(mAlphaSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int){ refreshColorButton(); });
    connect(mButtonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(mButtonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    layout->addWidget(mButtonBox);
    refreshColorButton();
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
    mShowGrid->setChecked(settings.show);

    mColorAnimator = enve::make_shared<ColorAnimator>();
    if (settings.colorAnimator) {
        mColorAnimator->setColor(settings.colorAnimator->getColor());
    } else {
        mColorAnimator->setColor(QColor(255, 255, 255, 96));
    }
    mCurrentColor = mColorAnimator->getColor();
    if (mAlphaSpin) {
        mAlphaSpin->setValue(mCurrentColor.alpha());
    }
    refreshColorButton();
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
    result.show = mShowGrid->isChecked();

    QColor finalColor = mCurrentColor;
    if (mAlphaSpin) {
        finalColor.setAlpha(mAlphaSpin->value());
    }
    result.colorAnimator = enve::make_shared<ColorAnimator>();
    result.colorAnimator->setColor(finalColor);
    return result;
}

void GridSettingsDialog::restoreDefaults()
{
    setSettings(GridSettings{});
}

void GridSettingsDialog::chooseColor()
{
    const QColor chosen = QColorDialog::getColor(mCurrentColor, this, tr("Select Grid Color"), QColorDialog::ShowAlphaChannel);
    if (!chosen.isValid()) { return; }
    mCurrentColor = chosen;
    if (mAlphaSpin) {
        mAlphaSpin->setValue(chosen.alpha());
    }
    if (mColorAnimator) {
        mColorAnimator->setColor(mCurrentColor);
    }
    refreshColorButton();
}

void GridSettingsDialog::refreshColorButton()
{
    if (!mColorButton) { return; }
    QColor display = mCurrentColor;
    if (mAlphaSpin) {
        display.setAlpha(mAlphaSpin->value());
    }
    const QColor textColor = display.lightness() > 128 ? Qt::black : Qt::white;
    const QString stylesheet = QStringLiteral("color: %1; background-color: %2; border: 1px solid gray;")
                                   .arg(textColor.name(), display.name(QColor::HexArgb));
    mColorButton->setStyleSheet(stylesheet);
    mColorButton->setText(display.name(QColor::HexArgb));
    mCurrentColor = display;
    if (mColorAnimator) {
        mColorAnimator->setColor(display);
    }
}
