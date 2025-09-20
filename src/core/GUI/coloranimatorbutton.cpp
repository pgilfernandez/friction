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

// Fork of enve - Copyright (C) 2016-2020 Maurycy Liebner

#include "coloranimatorbutton.h"
#include "Animators/coloranimator.h"
#include "colorsetting.h"
#include "GUI/ewidgets.h"
#include "Private/esettings.h"

#include <QVBoxLayout>
#include <QDialog>
#include <QPainter>
#include <QStyleOptionFocusRect>


ColorAnimatorButton::ColorAnimatorButton(QWidget * const parent) :
    BoxesListActionButton(parent) {
    connect(this, &BoxesListActionButton::pressed,
            this, &ColorAnimatorButton::openColorSettingsDialog);
}

ColorAnimatorButton::ColorAnimatorButton(ColorAnimator * const colorTarget,
                                         QWidget * const parent) :
    ColorAnimatorButton(parent) {
    setColorTarget(colorTarget);
}

ColorAnimatorButton::ColorAnimatorButton(const QColor &color,
                                         QWidget * const parent) :
    ColorAnimatorButton(parent) {
    mColor = color;
}

void ColorAnimatorButton::setColorTarget(ColorAnimator * const target) {
    mColorTarget.assign(target);
    if(target) {
        mColorTarget << connect(target->getVal1Animator(),
                                &QrealAnimator::effectiveValueChanged,
                                this, qOverload<>(&ColorAnimatorButton::update));
        mColorTarget << connect(target->getVal2Animator(),
                                &QrealAnimator::effectiveValueChanged,
                                this, qOverload<>(&ColorAnimatorButton::update));
        mColorTarget << connect(target->getVal3Animator(),
                                &QrealAnimator::effectiveValueChanged,
                                this, qOverload<>(&ColorAnimatorButton::update));
    }
    update();
}

void ColorAnimatorButton::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    const QColor swatch = mColorTarget ? mColorTarget->getColor() : mColor;
    const QRectF rect(0.5, 0.5, width() - 1.0, height() - 1.0);
    const bool isDisabled = !isEnabled();

    const auto settings = eSettings::sInstance;
    const auto themeColors = settings ? settings->fColors
                                      : Friction::Core::Theme::Colors();

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QColor fill = swatch;
    if (isDisabled) {
        fill.setAlphaF(fill.alphaF() * 0.35);
    }
    painter.setBrush(fill);

    QColor border = mHover ? themeColors.highlight : themeColors.baseBorder;
    if (!border.isValid()) {
        border = palette().color(mHover ? QPalette::Highlight : QPalette::Mid);
    }
    if (isDisabled) {
        border = themeColors.baseBorder.isValid()
                     ? themeColors.baseBorder.lighter(150)
                     : palette().color(QPalette::Midlight);
    }
    painter.setPen(QPen(border, 1.0));
    painter.drawRoundedRect(rect, 2.0, 2.0);

    QColor inner = fill;
    if (inner.isValid()) {
        inner = inner.lighter(mHover ? 115 : 100);
        inner.setAlphaF(qMin(1.0, inner.alphaF() + 0.1));
        painter.setPen(Qt::NoPen);
        painter.setBrush(inner);
        painter.drawRoundedRect(rect.adjusted(2.0, 2.0, -2.0, -2.0), 1.5, 1.5);
    }

    if (hasFocus()) {
        QStyleOptionFocusRect opt;
        opt.initFrom(this);
        opt.rect = rect.toAlignedRect().adjusted(2, 2, -2, -2);
        style()->drawPrimitive(QStyle::PE_FrameFocusRect, &opt, &painter, this);
    }
}

void ColorAnimatorButton::openColorSettingsDialog() {
    const auto dialog = new QDialog(this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setLayout(new QVBoxLayout(dialog));
    QWidget* colorWidget = nullptr;

    if(mColorTarget) {
        colorWidget = eWidgets::sColorWidget(dialog, mColorTarget);
    } else {
        const auto colOp = [this](const ColorSetting& setting) {
            mColor = setting.getColor();
            update();
        };
        colorWidget = eWidgets::sColorWidget(dialog, mColor, this, colOp);
    }
    dialog->layout()->addWidget(colorWidget);

    dialog->raise();
    dialog->show();
}

void ColorAnimatorButton::setColor(const QColor &color) {
    mColor = color;
    update();
}

QColor ColorAnimatorButton::color() const {
    if(mColorTarget) return mColorTarget->getColor();
    return mColor;
}
