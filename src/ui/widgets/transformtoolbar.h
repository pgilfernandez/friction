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

#ifndef FRICTION_TRANSFORM_TOOLBAR_H
#define FRICTION_TRANSFORM_TOOLBAR_H

#include "ui_global.h"
#include "widgets/toolbar.h"
#include "canvas.h"

namespace Friction
{
    namespace Ui
    {
        class UI_EXPORT TransformToolBar : public ToolBar
        {
        public:
            explicit TransformToolBar(QWidget *parent = nullptr);
            void setCurrentCanvas(Canvas * const target);

        private:
            void updateWidgets(Canvas * const target);
            ConnContextQPtr<Canvas> mCanvas;
        };
    }
}

#endif // FRICTION_TRANSFORM_TOOLBAR_H
