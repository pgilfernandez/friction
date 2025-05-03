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

#ifndef FRICTION_ALIGN_TOOLBAR_H
#define FRICTION_ALIGN_TOOLBAR_H

#include "ui_global.h"

#include "widgets/toolbar.h"
#include "canvas.h"

#include <QComboBox>
#include <QList>

namespace Friction
{
    namespace Ui
    {
        class UI_EXPORT AlignToolBar : public ToolBar
        {
            Q_OBJECT

        public:
            explicit AlignToolBar(QWidget *parent = nullptr);
            void setCurrentCanvas(Canvas * const target);

        private:
            void setupWidgets();

            void triggerShow(bool triggered);
            void triggerAlign(const Qt::Alignment &align);
            void setComboBoxItemState(QComboBox *box,
                                      int index,
                                      bool enabled);

            ConnContextQPtr<Canvas> mCanvas;

            QComboBox *mAlignPivot;
            QComboBox *mRelativeTo;

            QAction *mAlignShowAct;
            QAction *mAlignPivotAct;
            QAction *mRelativeToAct;
            QAction *mAlignLeftAct;
            QAction *mAlignHCenterAct;
            QAction *mAlignRightAct;
            QAction *mAlignTopAct;
            QAction *mAlignVCenterAct;
            QAction *mAlignBottomAct;

            QList<QAction*> mSeparators;
        };
    }
}

#endif // FRICTION_ALIGN_TOOLBAR_H
