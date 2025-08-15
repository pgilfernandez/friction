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

const QColor Theme::getThemeBaseColor(int alpha)
{
    return getQColor(26, 26, 30, alpha);
}

SkColor Theme::getThemeBaseSkColor(int alpha)
{
    return SkColorSetARGB(alpha, 26, 26, 30);
}

const QColor Theme::getThemeBaseDarkColor(int alpha)
{
    return getQColor(25, 25, 25, alpha);
}

const QColor Theme::getThemeBaseDarkerColor(int alpha)
{
    return getQColor(19, 19, 21, alpha);
}

const QColor Theme::getThemeAlternateColor(int alpha)
{
    return getQColor(33, 33, 39, alpha);
}

const QColor Theme::getThemeHighlightColor(int alpha)
{
    return getQColor(104, 144, 206, alpha);
}

const QColor Theme::getThemeHighlightDarkerColor(int alpha)
{
    return getQColor(53, 101, 176, alpha);
}

const QColor Theme::getThemeHighlightAlternativeColor(int alpha)
{
    return getQColor(167, 185, 222, alpha);
}

const QColor Theme::getThemeHighlightSelectedColor(int alpha)
{
    return getQColor(150, 191, 255, alpha);
}

SkColor Theme::getThemeHighlightSkColor(int alpha)
{
    return SkColorSetARGB(alpha, 104, 144, 206);
}

const QColor Theme::getThemeButtonBaseColor(int alpha)
{
    return getQColor(49, 49, 59, alpha);
}

const QColor Theme::getThemeButtonBorderColor(int alpha)
{
    return getQColor(65, 65, 80, alpha);
}

const QColor Theme::getThemeComboBaseColor(int alpha)
{
    return getQColor(36, 36, 53, alpha);
}

const QColor Theme::getThemeTimelineColor(int alpha)
{
    return getQColor(44, 44, 49, alpha);
}

const QColor Theme::getThemeRangeColor(int alpha)
{
    return getQColor(56, 73, 101, alpha);
}

const QColor Theme::getThemeRangeSelectedColor(int alpha)
{
    return getQColor(87, 120, 173, alpha);
}

const QColor Theme::getThemeFrameMarkerColor(int alpha)
{
    return getThemeColorOrange(alpha);
}

const QColor Theme::getThemeObjectColor(int alpha)
{
    return getQColor(0, 102, 255, alpha);
}

const QColor Theme::getThemeColorRed(int alpha)
{
    return getQColor(199, 67, 72, alpha);
}

const QColor Theme::getThemeColorBlue(int alpha)
{
    return getQColor(73, 142, 209, alpha);
}

const QColor Theme::getThemeColorYellow(int alpha)
{
    return getQColor(209, 183, 73, alpha);
}

const QColor Theme::getThemeColorPink(int alpha)
{
    return getQColor(169, 73, 209, alpha);
}

const QColor Theme::getThemeColorGreen(int alpha)
{
    return getQColor(73, 209, 132, alpha);
}

const QColor Theme::getThemeColorGreenDark(int alpha)
{
    return getQColor(27, 49, 39, alpha);
}

const QColor Theme::getThemeColorOrange(int alpha)
{
    return getQColor(255, 123, 0, alpha);
}

const QColor Theme::getThemeColorTextDisabled(int alpha)
{
    return getQColor(112, 112, 113, alpha);
}

const QColor Theme::getThemeColorOutputDestinationLineEdit(int alpha)
{
    return getQColor(40, 40, 47, alpha);
}

const QColor Theme::getThemeColorGray(int alpha)
{
    Q_UNUSED(alpha)
    return Qt::gray;
}

const QColor Theme::getThemeColorDarkGray(int alpha)
{
    Q_UNUSED(alpha)
    return Qt::darkGray;
}

const QColor Theme::getThemeColorBlack(int alpha)
{
    Q_UNUSED(alpha)
    return Qt::black;
}

const QColor Theme::getThemeColorWhite(int alpha)
{
    Q_UNUSED(alpha)
    return Qt::white;
}

const QPalette Theme::getDefaultPalette(const QColor &highlight)
{
    QPalette palette;
    palette.setColor(QPalette::Window, getThemeAlternateColor());
    palette.setColor(QPalette::WindowText, getThemeColorWhite());
    palette.setColor(QPalette::Base, getThemeBaseColor());
    palette.setColor(QPalette::AlternateBase, getThemeAlternateColor());
    palette.setColor(QPalette::Link, getThemeColorWhite());
    palette.setColor(QPalette::LinkVisited, getThemeColorWhite());
    palette.setColor(QPalette::ToolTipText, getThemeColorWhite());
    palette.setColor(QPalette::ToolTipBase, getThemeColorBlack());
    palette.setColor(QPalette::Text, getThemeColorWhite());
    palette.setColor(QPalette::Button, getThemeBaseColor());
    palette.setColor(QPalette::ButtonText, getThemeColorWhite());
    palette.setColor(QPalette::BrightText, getThemeColorWhite());
    palette.setColor(QPalette::Highlight, highlight.isValid() ? highlight : getThemeHighlightColor());
    palette.setColor(QPalette::HighlightedText, getThemeColorWhite());
    palette.setColor(QPalette::Disabled, QPalette::Text, getThemeColorDarkGray());
    palette.setColor(QPalette::Disabled, QPalette::ButtonText, getThemeColorDarkGray());
    return palette;
}

const QPalette Theme::getDarkPalette(int alpha)
{
    QPalette pal = QPalette();
    pal.setColor(QPalette::Window, getThemeBaseColor(alpha));
    pal.setColor(QPalette::Base, getThemeBaseColor(alpha));
    pal.setColor(QPalette::Button, getThemeBaseColor(alpha));
    return pal;
}

const QPalette Theme::getDarkerPalette(int alpha)
{
    QPalette pal = QPalette();
    pal.setColor(QPalette::Window, getThemeBaseDarkerColor(alpha));
    pal.setColor(QPalette::Base, getThemeBaseDarkerColor(alpha));
    pal.setColor(QPalette::Button, getThemeBaseDarkerColor(alpha));
    return pal;
}

const QPalette Theme::getNotSoDarkPalette(int alpha)
{
    QPalette pal = QPalette();
    pal.setColor(QPalette::Window, getThemeAlternateColor(alpha));
    pal.setColor(QPalette::Base, getThemeBaseColor(alpha));
    pal.setColor(QPalette::Button, getThemeBaseColor(alpha));
    return pal;
}

const QString Theme::getThemeStyle(int iconSize)
{
    QString css;
    QFile stylesheet(QString::fromUtf8(":/styles/friction.qss"));
    if (stylesheet.open(QIODevice::ReadOnly | QIODevice::Text)) {
        css = stylesheet.readAll();
        stylesheet.close();
    }
    const qreal iconPixelRatio = iconSize * qApp->desktop()->devicePixelRatioF();
    return css.arg(getThemeButtonBaseColor().name(),
                   getThemeButtonBorderColor().name(),
                   getThemeBaseDarkerColor().name(),
                   getThemeHighlightColor().name(),
                   getThemeBaseColor().name(),
                   getThemeAlternateColor().name(),
                   QString::number(getIconSize(iconSize).width()),
                   getThemeColorOrange().name(),
                   getThemeRangeSelectedColor().name(),
                   QString::number(getIconSize(iconSize / 2).width()),
                   QString::number(getIconSize(qRound(iconPixelRatio)).width()),
                   QString::number(getIconSize(qRound(iconPixelRatio / 2)).width()),
                   getThemeColorTextDisabled().name(),
                   QString::number(getIconSize(iconSize).width() / 4),
                   getThemeColorOutputDestinationLineEdit().name());
}

void Theme::setupTheme(const int iconSize)
{
    QIcon::setThemeSearchPaths(QStringList() << QString::fromUtf8(":/icons"));
    QIcon::setThemeName(QString::fromUtf8("hicolor"));
    qApp->setStyle(QString::fromUtf8("fusion"));
    qApp->setPalette(getDefaultPalette());
    qApp->setStyleSheet(getThemeStyle(iconSize));
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
