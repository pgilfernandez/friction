/*
#
# Friction - https://friction.graphics
#
# Copyright (c) Ole-André Rodlie and contributors
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, version 3.
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

#include "transformtoolbar.h"
#include "GUI/global.h"
#include "Private/document.h"

#include <QDebug>

using namespace Friction::Ui;

TransformToolBar::TransformToolBar(QWidget *parent)
    : ToolBar("TransformToolBar", parent)
{
    setEnabled(false);
    setWindowTitle(tr("Transform Toolbar"));
}

void TransformToolBar::setCurrentCanvas(Canvas * const target)
{
    mCanvas.assign(target);
    if (target) {
        // TODO
        //mCanvas << connect(mCanvas, &Canvas::currentBoxChanged);
        //mCanvas << connect(mCanvas, &Canvas::objectSelectionChanged);
        //mCanvas << connect(mCanvas, &Canvas::requestUpdate);
    }
    updateWidgets(target);
}

// TODO
void TransformToolBar::updateWidgets(Canvas * const target)
{
    if (!target) {
        setEnabled(false);
        return;
    }
    setEnabled(true);
}
