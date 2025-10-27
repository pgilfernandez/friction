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

#include "savedcolorswidget.h"
#include "widgets/savedcolorbutton.h"

#include <QIcon>
#include <QPushButton>
#include <QSize>
#include <QSizePolicy>

#include "colorhelpers.h"
#include "Private/document.h"
#include "GUI/global.h"
#include "themesupport.h"

SavedColorsWidget::SavedColorsWidget(QWidget *parent)
    : QWidget(parent) {
    mMainLayout = new FlowLayout(this);
    setLayout(mMainLayout);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Maximum);

    setupBookmarkButton();

    for(const auto& color : Document::sInstance->fColors) {
        addColor(color);
    }
    connect(Document::sInstance, &Document::bookmarkColorAdded,
            this, &SavedColorsWidget::addColor);
    connect(Document::sInstance, &Document::bookmarkColorRemoved,
            this, &SavedColorsWidget::removeColor);
}

void SavedColorsWidget::setupBookmarkButton() {
    const auto button = new QPushButton(this);
    button->setCursor(Qt::PointingHandCursor);
    button->setFocusPolicy(Qt::NoFocus);
    button->setToolTip(tr("Add bookmarked color"));
    button->setIcon(QIcon::fromTheme("plus"));

    eSizesUI::widget.add(button, [button](int size) {
        button->setFixedSize(size, size);
        const int iconSide = qMax(0, size);
        button->setIconSize(QSize(iconSide, iconSide));
    });

    connect(button, &QPushButton::clicked,
            this, &SavedColorsWidget::addBookmarkButton);
    mMainLayout->addWidget(button);
}

void SavedColorsWidget::addBookmarkButton() {
    if(!mCurrentColor.isValid()) {
        return;
    }
    Document::sInstance->addBookmarkColor(mCurrentColor);
}

void SavedColorsWidget::addColor(const QColor& color) {
    const auto button = new SavedColorButton(color, this);
    connect(button, &SavedColorButton::selected,
            this, &SavedColorsWidget::colorSet);
    mMainLayout->addWidget(button);
    mButtons << button;
}

void SavedColorsWidget::removeColor(const QColor &color) {
    const auto rgba = color.rgba();
    for(const auto wid : mButtons) {
        if(wid->getColor().rgba() == rgba) {
            mButtons.removeOne(wid);
            wid->deleteLater();
            break;
        }
    }
}

void SavedColorsWidget::setColor(const QColor &color) {
    mCurrentColor = color;
    for(const auto wid : mButtons) {
        wid->setSelected(wid->getColor().rgba() == color.rgba());
    }
}
