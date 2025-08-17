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

#include "themesupport.h"

#include <QFile>
#include <QIcon>
#include <QApplication>
#include <QDebug>
#include <QDesktopWidget>
#include <QToolButton>

using namespace Friction::Core;

const QColor Theme::getQColor(int r,
                              int g,
                              int b,
                              int a)
{
    return a == 255 ? QColor(r, g, b) : QColor(r, g, b, a);
}

const QColor Theme::getThemeHighlightColor(int alpha)
{
    return getQColor(104, 144, 206, alpha);
}

const QColor Theme::getThemeHighlightSelectedColor(int alpha)
{
    return getQColor(150, 191, 255, alpha);
}

const QColor Theme::getThemeButtonBaseColor(int alpha)
{
    return getQColor(49, 49, 59, alpha);
}

const QColor Theme::getThemeButtonBorderColor(int alpha)
{
    return getQColor(65, 65, 80, alpha);
}

const QColor Theme::getThemeRangeColor(int alpha)
{
    return getQColor(56, 73, 101, alpha);
}

const QColor Theme::getThemeRangeSelectedColor(int alpha)
{
    return getQColor(87, 120, 173, alpha);
}

const QColor Theme::getThemeObjectColor(int alpha)
{
    return getQColor(0, 102, 255, alpha);
}

const QColor Theme::getThemeColorGreen(int alpha)
{
    return getQColor(73, 209, 132, alpha);
}

const QColor Theme::getThemeColorOrange(int alpha)
{
    return getQColor(255, 123, 0, alpha);
}

const QPalette Theme::getDefaultPalette(const QColor &highlight,
                                        const Colors &colors)
{
    QPalette palette;
    palette.setColor(QPalette::Window, colors.baseAlt);
    palette.setColor(QPalette::WindowText, colors.white);
    palette.setColor(QPalette::Base, colors.base);
    palette.setColor(QPalette::AlternateBase, colors.baseAlt);
    palette.setColor(QPalette::Link, colors.white);
    palette.setColor(QPalette::LinkVisited, colors.white);
    palette.setColor(QPalette::ToolTipText, colors.white);
    palette.setColor(QPalette::ToolTipBase, colors.black);
    palette.setColor(QPalette::Text, colors.white);
    palette.setColor(QPalette::Button, colors.base);
    palette.setColor(QPalette::ButtonText, colors.white);
    palette.setColor(QPalette::BrightText, colors.white);
    palette.setColor(QPalette::Highlight, highlight.isValid() ? highlight : colors.highlight);
    palette.setColor(QPalette::HighlightedText, colors.white);
    palette.setColor(QPalette::Disabled, QPalette::Text, colors.darkGray);
    palette.setColor(QPalette::Disabled, QPalette::ButtonText, colors.darkGray);
    return palette;
}

const QPalette Theme::getDarkPalette(const Colors &colors)
{
    QPalette pal = QPalette();
    pal.setColor(QPalette::Window, colors.base);
    pal.setColor(QPalette::Base, colors.base);
    pal.setColor(QPalette::Button, colors.base);
    return pal;
}

const QPalette Theme::getDarkerPalette(const Colors &colors)
{
    QPalette pal = QPalette();
    pal.setColor(QPalette::Window, colors.baseDark);
    pal.setColor(QPalette::Base, colors.baseDark);
    pal.setColor(QPalette::Button, colors.baseDark);
    return pal;
}

const QPalette Theme::getNotSoDarkPalette(const Colors &colors)
{
    QPalette pal = QPalette();
    pal.setColor(QPalette::Window, colors.baseAlt);
    pal.setColor(QPalette::Base, colors.base);
    pal.setColor(QPalette::Button, colors.base);
    return pal;
}

const QString Theme::getThemeStyle(int iconSize,
                                   const Colors &colors)
{
    QString css;
    QFile stylesheet(QString::fromUtf8(":/styles/friction.qss"));
    if (stylesheet.open(QIODevice::ReadOnly | QIODevice::Text)) {
        css = stylesheet.readAll();
        stylesheet.close();
    }
    const qreal iconPixelRatio = iconSize * qApp->desktop()->devicePixelRatioF();
    return css.arg(colors.baseButton.name(),
                   colors.baseBorder.name(),
                   colors.baseDarker.name(),
                   colors.highlight.name(),
                   colors.base.name(),
                   colors.baseAlt.name(),
                   QString::number(getIconSize(iconSize).width()),
                   colors.orange.name(),
                   colors.rangeSelected.name(),
                   QString::number(getIconSize(iconSize / 2).width()),
                   QString::number(getIconSize(qRound(iconPixelRatio)).width()),
                   QString::number(getIconSize(qRound(iconPixelRatio / 2)).width()),
                   colors.textDisabled.name(),
                   QString::number(getIconSize(iconSize).width() / 4),
                   colors.outputDestination.name());
}

void Theme::setupTheme(const int iconSize,
                       const Colors &colors)
{
    QIcon::setThemeSearchPaths(QStringList() << QString::fromUtf8(":/icons"));
    QIcon::setThemeName(QString::fromUtf8("hicolor"));
    qApp->setStyle(QString::fromUtf8("fusion"));
    qApp->setPalette(getDefaultPalette(QColor(), colors));
    qApp->setStyleSheet(getThemeStyle(iconSize, colors));
}

const QList<QSize> Theme::getAvailableIconSizes()
{
    return QIcon::fromTheme("visible").availableSizes();
}

const QSize Theme::getIconSize(const int size)
{
    QSize requestedSize(size, size);
    const auto iconSizes = getAvailableIconSizes();
    bool hasIconSize = iconSizes.contains(requestedSize);
    if (hasIconSize) { return requestedSize; }
    const auto foundIconSize = findClosestIconSize(size);
    return foundIconSize;
}

bool Theme::hasIconSize(const int size)
{
    return getAvailableIconSizes().contains(QSize(size, size));
}

const QSize Theme::findClosestIconSize(int iconSize)
{
    const auto iconSizes = getAvailableIconSizes();
    return *std::min_element(iconSizes.begin(),
                             iconSizes.end(),
                             [iconSize](const QSize& a,
                                        const QSize& b) {
        return qAbs(a.width() - iconSize) < qAbs(b.width() - iconSize);
    });
}

void Theme::setToolbarButtonStyle(const QString &name,
                                         QToolBar *bar,
                                         QAction *act)
{
    if (!bar || !act || name.simplified().isEmpty()) { return; }
    if (QWidget *widget = bar->widgetForAction(act)) {
        if (QToolButton *button = qobject_cast<QToolButton*>(widget)) {
            button->setObjectName(name);
        }
    }
}
