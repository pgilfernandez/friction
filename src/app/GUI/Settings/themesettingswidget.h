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

#ifndef THEMESETTINGSWIDGET_H
#define THEMESETTINGSWIDGET_H

#include "widgets/settingswidget.h"
#include "themesupport.h"

#include <QMap>
#include <QSet>
#include <QVector>
#include <QString>
#include <QJsonObject>

class ColorAnimatorButton;
class QComboBox;
class QPushButton;

class ThemeSettingsWidget : public SettingsWidget
{
public:
    explicit ThemeSettingsWidget(QWidget *parent = nullptr);

    void applySettings() override;
    void updateSettings(bool restore = false) override;

private:
    using ThemeColors = Friction::Core::Theme::Colors;
    using ColorMember = QColor ThemeColors::*;

    struct ColorItem {
        QString displayName;
        ColorMember member = nullptr;
        ColorAnimatorButton *button = nullptr;
    };

    struct ThemeEntry {
        ThemeColors colors;
        bool readOnly = false;
    };

    void setupHeader();
    void setupColorGrid();
    void loadThemePresets();
    void refreshThemeSelector(const QString &selectedTheme = QString());
    void updateRemoveButtonState();
    void saveThemePresets() const;
    void saveActiveTheme();
    void syncActiveThemeFromButtons();
    void storeCurrentColors() const;

    void onThemeSelected(const QString &themeName);
    void onAddTheme();
    void onRemoveTheme();
    void onExportTheme();
    void onImportTheme();

    ThemeColors collectColorsFromButtons() const;
    void applyThemeToButtons(const ThemeColors &colors);
    QString findMatchingTheme(const ThemeColors &colors) const;
    bool colorsEqual(const ThemeColors &lhs, const ThemeColors &rhs) const;
    bool insertOrUpdateTheme(const QString &name,
                             const ThemeColors &colors,
                             bool readOnly);
    bool deserializeColors(const QJsonObject &object,
                           ThemeColors &colors) const;
    QJsonObject serializeColors(const ThemeColors &colors) const;
    QString sanitizeThemeName(const QString &name) const;

    QVector<ColorItem> mColorItems;

    QComboBox *mThemeSelector = nullptr;
    QPushButton *mAddButton = nullptr;
    QPushButton *mRemoveButton = nullptr;
    QPushButton *mExportButton = nullptr;
    QPushButton *mImportButton = nullptr;

    QMap<QString, ThemeEntry> mThemePresets;
    QVector<QString> mThemeOrder;
    QString mCurrentTheme;
};

#endif // THEMESETTINGSWIDGET_H
