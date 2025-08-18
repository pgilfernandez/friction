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

#ifndef THEMESUPPORT_H
#define THEMESUPPORT_H

#include "core_global.h"

#include <QPalette>
#include <QList>
#include <QSize>
#include <QToolBar>

namespace Friction
{
    namespace Core
    {
        class CORE_EXPORT Theme
        {
        public:
            struct Colors {
                QColor red;
                QColor blue;
                QColor yellow;
                QColor purple;
                QColor green;
                QColor darkGreen;
                QColor orange;
                QColor gray;
                QColor darkGray;
                QColor lightGray;
                QColor black;
                QColor white;

                QColor base;
                QColor baseAlt;
                QColor baseButton;
                QColor baseCombo;
                QColor baseBorder;
                QColor baseDark;
                QColor baseDarker;

                QColor highlight;
                QColor highlightAlt;
                QColor highlightDarker;
                QColor highlightSelected;

                QColor scene;
                QColor sceneClip;
                QColor sceneBorder;

                QColor timelineGrid;
                QColor timelineRange;
                QColor timelineRangeSelected;
                QColor timelineHighlightRow;
                QColor timelineAltRow;
                QColor timelineAnimRange;

                QColor keyframeObject;
                QColor keyframePropertyGroup;
                QColor keyframeProperty;
                QColor keyframeSelected;

                QColor marker;
                QColor markerIO;

                QColor defaultStroke;
                QColor defaultFill;

                QColor transformOverlayBase;
                QColor transformOverlayAlt;

                QColor point;
                QColor pointSelected;
                QColor pointHoverOutline;
                QColor pointKeyOuter;
                QColor pointKeyInner;

                QColor pathNode;
                QColor pathNodeSelected;
                QColor pathDissolvedNode;
                QColor pathDissolvedNodeSelected;
                QColor pathControl;
                QColor pathControlSelected;
                QColor pathHoverOuter;
                QColor pathHoverInner;

                QColor segmentHoverOuter;
                QColor segmentHoverInner;

                QColor boundingBox;

                QColor nullObject;

                QColor textDisabled;
                QColor outputDestination; // remove
            };
            static const QColor transparentColor(QColor c, int a);
            static const QPalette getDefaultPalette(const QColor &highlight = QColor(),
                                                    const Colors &colors = Colors());
            static const QPalette getDarkPalette(const Colors &colors = Colors());
            static const QPalette getDarkerPalette(const Colors &colors = Colors());
            static const QPalette getNotSoDarkPalette(const Colors &colors = Colors());
            static const QString getStyle(int iconSize = 20,
                                          const Colors &colors = Colors(),
                                          const QString qss = ":/styles/friction.qss");
            static void setupTheme(const int iconSize = 20,
                                   const Colors &colors = Colors());
            static const QList<QSize> getAvailableIconSizes();
            static const QSize getIconSize(const int size);
            static bool hasIconSize(const int size);
            static const QSize findClosestIconSize(int iconSize);
            static void setToolbarButtonStyle(const QString &name,
                                              QToolBar *bar,
                                              QAction *act);
        };
    }
}

#endif // THEMESUPPORT_H
