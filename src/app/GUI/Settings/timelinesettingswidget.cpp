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

#include "timelinesettingswidget.h"
#include "Private/esettings.h"

#include "GUI/coloranimatorbutton.h"

#include <QLabel>

#include "GUI/global.h"
#include "themesupport.h"

TimelineSettingsWidget::TimelineSettingsWidget(QWidget *parent) :
    SettingsWidget(parent) {
    /*mAlternateRowCheck = new QCheckBox("Alternate row color", this);
    mAlternateRowColor = new ColorAnimatorButton(
                mSett.fTimelineAlternateRowColor);
    add2HWidgets(mAlternateRowCheck, mAlternateRowColor);

    mHighlightRowCheck = new QCheckBox("Highlight row under mouse", this);
    mHighlightRowColor = new ColorAnimatorButton(
                mSett.fTimelineHighlightRowColor);
    add2HWidgets(mHighlightRowCheck, mHighlightRowColor);

    addSeparator();*/

    mObjectKeyframeColor = new ColorAnimatorButton(
                               mSett.fObjectKeyframeColor);
    mPropertyGroupKeyframeColor = new ColorAnimatorButton(
                               mSett.fPropertyGroupKeyframeColor);
    mPropertyKeyframeColor = new ColorAnimatorButton(
                               mSett.fPropertyKeyframeColor);
    mSelectedKeyframeColor = new ColorAnimatorButton(
                               mSett.fSelectedKeyframeColor);
    mThemeButtonBaseColor = new ColorAnimatorButton(
                               mSett.fThemeButtonBaseColor);
    mThemeButtonBorderColor = new ColorAnimatorButton(mSett.fThemeButtonBorderColor);
    mThemeBaseDarkerColor = new ColorAnimatorButton(mSett.fThemeBaseDarkerColor);
    mThemeHighlightColor = new ColorAnimatorButton(mSett.fThemeHighlightColor);
    mThemeBaseColor = new ColorAnimatorButton(mSett.fThemeBaseColor);
    mThemeAlternateColor = new ColorAnimatorButton(mSett.fThemeAlternateColor);
    mThemeColorOrange = new ColorAnimatorButton(mSett.fThemeColorOrange);
    mThemeRangeSelectedColor = new ColorAnimatorButton(mSett.fThemeRangeSelectedColor);
    mThemeColorTextDisabled = new ColorAnimatorButton(mSett.fThemeColorTextDisabled);
    mThemeColorOutputDestinationLineEdit = new ColorAnimatorButton(mSett.fThemeColorOutputDestinationLineEdit);

    add2HWidgets(new QLabel("Object keyframe color"),
                 mObjectKeyframeColor);
    add2HWidgets(new QLabel("Property group keyframe color"),
                 mPropertyGroupKeyframeColor);
    add2HWidgets(new QLabel("Property keyframe color"),
                 mPropertyKeyframeColor);
    add2HWidgets(new QLabel("Selected keyframe color"),
                 mSelectedKeyframeColor);

    addSeparator();
    
    add2HWidgets(new QLabel("Theme Button base color"),
                 mThemeButtonBaseColor);
    add2HWidgets(new QLabel("Theme Button border color"),
                 mThemeButtonBorderColor);
    add2HWidgets(new QLabel("Theme Base darker color"),
                 mThemeBaseDarkerColor);
    add2HWidgets(new QLabel("Theme Highlight color"),
                 mThemeHighlightColor);
    add2HWidgets(new QLabel("Theme Base color"),
                 mThemeBaseColor);
    add2HWidgets(new QLabel("Theme Alternate color"),
                 mThemeAlternateColor);
    add2HWidgets(new QLabel("Theme Color Orange"),
                 mThemeColorOrange);
    add2HWidgets(new QLabel("Theme Range selected color"),
                 mThemeRangeSelectedColor);
    add2HWidgets(new QLabel("Theme Text disabled color"),
                 mThemeColorTextDisabled);
    add2HWidgets(new QLabel("Theme Output-destination LineEdit color"),
                 mThemeColorOutputDestinationLineEdit);

    /*addSeparator();

    mVisibilityRangeColor = new ColorAnimatorButton(
                               mSett.fVisibilityRangeColor);
    mSelectedVisibilityRangeColor = new ColorAnimatorButton(
                               mSett.fSelectedVisibilityRangeColor);
    mAnimationRangeColor = new ColorAnimatorButton(
                               mSett.fAnimationRangeColor);

    add2HWidgets(new QLabel("Visibility range color"),
                 mVisibilityRangeColor);
    add2HWidgets(new QLabel("Selected visibility range color"),
                 mSelectedVisibilityRangeColor);
    add2HWidgets(new QLabel("Animation range color"),
                 mAnimationRangeColor);*/

    /*eSizesUI::widget.add(mAlternateRowCheck, [this](const int size) {
        mAlternateRowCheck->setFixedHeight(size);
        mAlternateRowCheck->setStyleSheet(QString("QCheckBox::indicator { width: %1px; height: %1px;}").arg(size/1.5));
        mHighlightRowCheck->setFixedHeight(size);
        mHighlightRowCheck->setStyleSheet(QString("QCheckBox::indicator { width: %1px; height: %1px;}").arg(size/1.5));
    });*/
}

void TimelineSettingsWidget::applySettings() {
    //mSett.fTimelineAlternateRow = mAlternateRowCheck->isChecked();
    //mSett.fTimelineAlternateRowColor = mAlternateRowColor->color();

    //mSett.fTimelineHighlightRow = mHighlightRowCheck->isChecked();
    //mSett.fTimelineHighlightRowColor = mHighlightRowColor->color();

    mSett.fObjectKeyframeColor = mObjectKeyframeColor->color();
    mSett.fPropertyGroupKeyframeColor = mPropertyGroupKeyframeColor->color();
    mSett.fPropertyKeyframeColor = mPropertyKeyframeColor->color();
    mSett.fSelectedKeyframeColor = mSelectedKeyframeColor->color();
    mSett.fThemeButtonBaseColor = mThemeButtonBaseColor->color();
    mSett.fThemeButtonBorderColor = mThemeButtonBorderColor->color();
    mSett.fThemeBaseDarkerColor = mThemeBaseDarkerColor->color();
    mSett.fThemeHighlightColor = mThemeHighlightColor->color();
    mSett.fThemeBaseColor = mThemeBaseColor->color();
    mSett.fThemeAlternateColor = mThemeAlternateColor->color();
    mSett.fThemeColorOrange = mThemeColorOrange->color();
    mSett.fThemeRangeSelectedColor = mThemeRangeSelectedColor->color();
    mSett.fThemeColorTextDisabled = mThemeColorTextDisabled->color();
    mSett.fThemeColorOutputDestinationLineEdit = mThemeColorOutputDestinationLineEdit->color();

    // Persistir todos los cambios registrados y reaplicar el tema
    if (eSettings::sInstance) {
        eSettings::sInstance->saveToFile();
        ThemeSupport::setupTheme(16);
    }

    //mSett.fVisibilityRangeColor = mVisibilityRangeColor->color();
    //mSett.fSelectedVisibilityRangeColor = mSelectedVisibilityRangeColor->color();
    //mSett.fAnimationRangeColor = mAnimationRangeColor->color();
}

void TimelineSettingsWidget::updateSettings(bool restore)
{
    Q_UNUSED(restore)
    //mAlternateRowCheck->setChecked(mSett.fTimelineAlternateRow);
    //mAlternateRowColor->setColor(mSett.fTimelineAlternateRowColor);

    //mHighlightRowCheck->setChecked(mSett.fTimelineHighlightRow);
    //mHighlightRowColor->setColor(mSett.fTimelineHighlightRowColor);

    mObjectKeyframeColor->setColor(mSett.fObjectKeyframeColor);
    mPropertyGroupKeyframeColor->setColor(mSett.fPropertyGroupKeyframeColor);
    mPropertyKeyframeColor->setColor(mSett.fPropertyKeyframeColor);
    mSelectedKeyframeColor->setColor(mSett.fSelectedKeyframeColor);
    mThemeButtonBaseColor->setColor(mSett.fThemeButtonBaseColor);
    mThemeButtonBorderColor->setColor(mSett.fThemeButtonBorderColor);
    mThemeBaseDarkerColor->setColor(mSett.fThemeBaseDarkerColor);
    mThemeHighlightColor->setColor(mSett.fThemeHighlightColor);
    mThemeBaseColor->setColor(mSett.fThemeBaseColor);
    mThemeAlternateColor->setColor(mSett.fThemeAlternateColor);
    mThemeColorOrange->setColor(mSett.fThemeColorOrange);
    mThemeRangeSelectedColor->setColor(mSett.fThemeRangeSelectedColor);
    mThemeColorTextDisabled->setColor(mSett.fThemeColorTextDisabled);
    mThemeColorOutputDestinationLineEdit->setColor(mSett.fThemeColorOutputDestinationLineEdit);

    //mVisibilityRangeColor->setColor(mSett.fVisibilityRangeColor);
    //mSelectedVisibilityRangeColor->setColor(mSett.fSelectedVisibilityRangeColor);
    //mAnimationRangeColor->setColor(mSett.fAnimationRangeColor);
}
