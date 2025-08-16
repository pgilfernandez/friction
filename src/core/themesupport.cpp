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

#include "themesupport.h"
#include "Private/esettings.h"

#include <QFile>
#include <QIcon>
#include <QApplication>
#include <QDebug>
#include <QDesktopWidget>
#include <QToolButton>

const QColor ThemeSupport::getQColor(int r,
                                     int g,
                                     int b,
                                     int a)
{
    return a == 255 ? QColor(r, g, b) : QColor(r, g, b, a);
}

const QColor ThemeSupport::getThemeBaseColor(int alpha)
{
    if (eSettings::sInstance) {
        const QColor userCol = eSettings::sInstance->fThemeBaseColor;
        if (userCol.isValid()) { QColor c = userCol; c.setAlpha(alpha); return c; }
    }
    return getQColor(26, 26, 30, alpha);
}

SkColor ThemeSupport::getThemeBaseSkColor(int alpha)
{
    return SkColorSetARGB(alpha, 26, 26, 30);
}

const QColor ThemeSupport::getThemeBaseDarkColor(int alpha)
{
    return getQColor(25, 25, 25, alpha);
}

const QColor ThemeSupport::getThemeBaseDarkerColor(int alpha)
{
    if (eSettings::sInstance) {
        const QColor userCol = eSettings::sInstance->fThemeBaseDarkerColor;
        if (userCol.isValid()) { QColor c = userCol; c.setAlpha(alpha); return c; }
    }
    return getQColor(19, 19, 21, alpha);
}

const QColor ThemeSupport::getThemeAlternateColor(int alpha)
{
    if (eSettings::sInstance) {
        const QColor userCol = eSettings::sInstance->fThemeAlternateColor;
        if (userCol.isValid()) { QColor c = userCol; c.setAlpha(alpha); return c; }
    }
    return getQColor(33, 33, 39, alpha);
}

const QColor ThemeSupport::getThemeHighlightColor(int alpha)
{
    if (eSettings::sInstance) {
        const QColor userCol = eSettings::sInstance->fThemeHighlightColor;
        if (userCol.isValid()) { QColor c = userCol; c.setAlpha(alpha); return c; }
    }
    return getQColor(104, 144, 206, alpha);
}

const QColor ThemeSupport::getThemeHighlightDarkerColor(int alpha)
{
    return getQColor(53, 101, 176, alpha);
}

const QColor ThemeSupport::getThemeHighlightAlternativeColor(int alpha)
{
    return getQColor(167, 185, 222, alpha);
}

const QColor ThemeSupport::getThemeHighlightSelectedColor(int alpha)
{
    return getQColor(150, 191, 255, alpha);
}

SkColor ThemeSupport::getThemeHighlightSkColor(int alpha)
{
    return SkColorSetARGB(alpha, 104, 144, 206);
}

const QColor ThemeSupport::getThemeButtonBaseColor(int alpha)
{
    if (eSettings::sInstance) {
        const QColor userCol = eSettings::sInstance->fThemeButtonBaseColor;
        if (userCol.isValid()) { QColor c = userCol; c.setAlpha(alpha); return c; }
    }
    return getQColor(49, 49, 59, alpha);
}

const QColor ThemeSupport::getThemeButtonBorderColor(int alpha)
{
    if (eSettings::sInstance) {
        const QColor userCol = eSettings::sInstance->fThemeButtonBorderColor;
        if (userCol.isValid()) { QColor c = userCol; c.setAlpha(alpha); return c; }
    }
    return getQColor(65, 65, 80, alpha);
}

const QColor ThemeSupport::getThemeComboBaseColor(int alpha)
{
    return getQColor(36, 36, 53, alpha);
}

const QColor ThemeSupport::getThemeTimelineColor(int alpha)
{
    return getQColor(44, 44, 49, alpha);
}

const QColor ThemeSupport::getThemeRangeColor(int alpha)
{
    return getQColor(56, 73, 101, alpha);
}

const QColor ThemeSupport::getThemeRangeSelectedColor(int alpha)
{
    if (eSettings::sInstance) {
        const QColor userCol = eSettings::sInstance->fThemeRangeSelectedColor;
        if (userCol.isValid()) { QColor c = userCol; c.setAlpha(alpha); return c; }
    }
    return getQColor(87, 120, 173, alpha);
}

const QColor ThemeSupport::getThemeFrameMarkerColor(int alpha)
{
    return getThemeColorOrange(alpha);
}

const QColor ThemeSupport::getThemeObjectColor(int alpha)
{
    return getQColor(0, 102, 255, alpha);
}

const QColor ThemeSupport::getThemeColorRed(int alpha)
{
    return getQColor(199, 67, 72, alpha);
}

const QColor ThemeSupport::getThemeColorBlue(int alpha)
{
    return getQColor(73, 142, 209, alpha);
}

const QColor ThemeSupport::getThemeColorYellow(int alpha)
{
    return getQColor(209, 183, 73, alpha);
}

const QColor ThemeSupport::getThemeColorPink(int alpha)
{
    return getQColor(169, 73, 209, alpha);
}

const QColor ThemeSupport::getThemeColorGreen(int alpha)
{
    return getQColor(73, 209, 132, alpha);
}

const QColor ThemeSupport::getThemeColorGreenDark(int alpha)
{
    return getQColor(27, 49, 39, alpha);
}

const QColor ThemeSupport::getThemeColorOrange(int alpha)
{
    if (eSettings::sInstance) {
        const QColor userCol = eSettings::sInstance->fThemeColorOrange;
        if (userCol.isValid()) { QColor c = userCol; c.setAlpha(alpha); return c; }
    }
    return getQColor(255, 123, 0, alpha);
}

const QColor ThemeSupport::getThemeColorTextDisabled(int alpha)
{
    if (eSettings::sInstance) {
        const QColor userCol = eSettings::sInstance->fThemeColorTextDisabled;
        if (userCol.isValid()) { QColor c = userCol; c.setAlpha(alpha); return c; }
    }
    return getQColor(112, 112, 113, alpha);
}

const QColor ThemeSupport::getThemeColorOutputDestinationLineEdit(int alpha)
{
    if (eSettings::sInstance) {
        const QColor userCol = eSettings::sInstance->fThemeColorOutputDestinationLineEdit;
        if (userCol.isValid()) { QColor c = userCol; c.setAlpha(alpha); return c; }
    }
    return getQColor(40, 40, 47, alpha);
}

const QColor ThemeSupport::getThemeColorGray(int alpha)
{
    Q_UNUSED(alpha)
    return Qt::gray;
}

const QColor ThemeSupport::getThemeColorDarkGray(int alpha)
{
    Q_UNUSED(alpha)
    return Qt::darkGray;
}

const QColor ThemeSupport::getThemeColorBlack(int alpha)
{
    Q_UNUSED(alpha)
    return Qt::black;
}

const QColor ThemeSupport::getThemeColorWhite(int alpha)
{
    Q_UNUSED(alpha)
    return Qt::white;
}

const QPalette ThemeSupport::getDefaultPalette(const QColor &highlight)
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

const QPalette ThemeSupport::getDarkPalette(int alpha)
{
    QPalette pal = QPalette();
    pal.setColor(QPalette::Window, getThemeBaseColor(alpha));
    pal.setColor(QPalette::Base, getThemeBaseColor(alpha));
    pal.setColor(QPalette::Button, getThemeBaseColor(alpha));
    return pal;
}

const QPalette ThemeSupport::getDarkerPalette(int alpha)
{
    QPalette pal = QPalette();
    pal.setColor(QPalette::Window, getThemeBaseDarkerColor(alpha));
    pal.setColor(QPalette::Base, getThemeBaseDarkerColor(alpha));
    pal.setColor(QPalette::Button, getThemeBaseDarkerColor(alpha));
    return pal;
}

const QPalette ThemeSupport::getNotSoDarkPalette(int alpha)
{
    QPalette pal = QPalette();
    pal.setColor(QPalette::Window, getThemeAlternateColor(alpha));
    pal.setColor(QPalette::Base, getThemeBaseColor(alpha));
    pal.setColor(QPalette::Button, getThemeBaseColor(alpha));
    return pal;
}

const QString ThemeSupport::getThemeStyle(int iconSize)
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

void ThemeSupport::setupTheme(const int iconSize)
{
    if (eSettings::sInstance) {
        eSettings::sInstance->loadFromFile();
    }

    QIcon::setThemeSearchPaths(QStringList() << QString::fromUtf8(":/icons"));
    QIcon::setThemeName(QString::fromUtf8("hicolor"));
    qApp->setStyle(QString::fromUtf8("fusion"));
    qApp->setPalette(getDefaultPalette());
    qApp->setStyleSheet(getThemeStyle(iconSize));
}

const QList<QSize> ThemeSupport::getAvailableIconSizes()
{
    return QIcon::fromTheme("visible").availableSizes();
}

const QSize ThemeSupport::getIconSize(const int size)
{
    QSize requestedSize(size, size);
    const auto iconSizes = getAvailableIconSizes();
    bool hasIconSize = iconSizes.contains(requestedSize);
    if (hasIconSize) { return requestedSize; }
    const auto foundIconSize = findClosestIconSize(size);
    return foundIconSize;
}

bool ThemeSupport::hasIconSize(const int size)
{
    return getAvailableIconSizes().contains(QSize(size, size));
}

const QSize ThemeSupport::findClosestIconSize(int iconSize)
{
    const auto iconSizes = getAvailableIconSizes();
    return *std::min_element(iconSizes.begin(),
                             iconSizes.end(),
                             [iconSize](const QSize& a,
                                        const QSize& b) {
        return qAbs(a.width() - iconSize) < qAbs(b.width() - iconSize);
    });
}

void ThemeSupport::setToolbarButtonStyle(const QString &name,
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
