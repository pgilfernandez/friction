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

#include "themesettingswidget.h"

#include "GUI/coloranimatorbutton.h"
#include "Private/esettings.h"
#include "appsupport.h"

#include <QColor>
#include <QComboBox>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QJsonObject>
#include <QLineEdit>
#include <QJsonValue>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QWidget>


namespace {
using ThemeColors = Friction::Core::Theme::Colors;
using ColorMember = QColor ThemeColors::*;

struct ColorDescriptor {
    const char *name;
    ColorMember member;
};

constexpr ColorDescriptor kColorDescriptors[] = {
    {"red", &ThemeColors::red},
    {"blue", &ThemeColors::blue},
    {"yellow", &ThemeColors::yellow},
    {"purple", &ThemeColors::purple},
    {"green", &ThemeColors::green},
    {"darkGreen", &ThemeColors::darkGreen},
    {"orange", &ThemeColors::orange},
    {"gray", &ThemeColors::gray},
    {"darkGray", &ThemeColors::darkGray},
    {"lightGray", &ThemeColors::lightGray},
    {"black", &ThemeColors::black},
    {"white", &ThemeColors::white},
    {"base", &ThemeColors::base},
    {"baseAlt", &ThemeColors::baseAlt},
    {"baseButton", &ThemeColors::baseButton},
    {"baseCombo", &ThemeColors::baseCombo},
    {"baseBorder", &ThemeColors::baseBorder},
    {"baseDark", &ThemeColors::baseDark},
    {"baseDarker", &ThemeColors::baseDarker},
    {"highlight", &ThemeColors::highlight},
    {"highlightAlt", &ThemeColors::highlightAlt},
    {"highlightDarker", &ThemeColors::highlightDarker},
    {"highlightSelected", &ThemeColors::highlightSelected},
    {"scene", &ThemeColors::scene},
    {"sceneClip", &ThemeColors::sceneClip},
    {"sceneBorder", &ThemeColors::sceneBorder},
    {"timelineGrid", &ThemeColors::timelineGrid},
    {"timelineRange", &ThemeColors::timelineRange},
    {"timelineRangeSelected", &ThemeColors::timelineRangeSelected},
    {"timelineHighlightRow", &ThemeColors::timelineHighlightRow},
    {"timelineAltRow", &ThemeColors::timelineAltRow},
    {"timelineAnimRange", &ThemeColors::timelineAnimRange},
    {"keyframeObject", &ThemeColors::keyframeObject},
    {"keyframePropertyGroup", &ThemeColors::keyframePropertyGroup},
    {"keyframeProperty", &ThemeColors::keyframeProperty},
    {"keyframeSelected", &ThemeColors::keyframeSelected},
    {"marker", &ThemeColors::marker},
    {"markerIO", &ThemeColors::markerIO},
    {"defaultStroke", &ThemeColors::defaultStroke},
    {"defaultFill", &ThemeColors::defaultFill},
    {"transformOverlayBase", &ThemeColors::transformOverlayBase},
    {"transformOverlayAlt", &ThemeColors::transformOverlayAlt},
    {"point", &ThemeColors::point},
    {"pointSelected", &ThemeColors::pointSelected},
    {"pointHoverOutline", &ThemeColors::pointHoverOutline},
    {"pointKeyOuter", &ThemeColors::pointKeyOuter},
    {"pointKeyInner", &ThemeColors::pointKeyInner},
    {"pathNode", &ThemeColors::pathNode},
    {"pathNodeSelected", &ThemeColors::pathNodeSelected},
    {"pathDissolvedNode", &ThemeColors::pathDissolvedNode},
    {"pathDissolvedNodeSelected", &ThemeColors::pathDissolvedNodeSelected},
    {"pathControl", &ThemeColors::pathControl},
    {"pathControlSelected", &ThemeColors::pathControlSelected},
    {"pathHoverOuter", &ThemeColors::pathHoverOuter},
    {"pathHoverInner", &ThemeColors::pathHoverInner},
    {"segmentHoverOuter", &ThemeColors::segmentHoverOuter},
    {"segmentHoverInner", &ThemeColors::segmentHoverInner},
    {"boundingBox", &ThemeColors::boundingBox},
    {"nullObject", &ThemeColors::nullObject},
    {"textDisabled", &ThemeColors::textDisabled},
    {"outputDestination", &ThemeColors::outputDestination}
};

constexpr auto kColorDescriptorCount = sizeof(kColorDescriptors) / sizeof(ColorDescriptor);

const QString kDefaultThemeId = QStringLiteral("Default");
const QString kThemesGroup = QStringLiteral("themes");
const QString kThemesPresetsKey = QStringLiteral("presets");
const QString kThemesActiveKey = QStringLiteral("active");
const QString kThemesCurrentColorsKey = QStringLiteral("currentColors");

QString formatColorName(const QString &name)
{
    QString result;
    result.reserve(name.size() + 4);
    for (int i = 0; i < name.size(); ++i) {
        const QChar ch = name.at(i);
        if (i == 0) {
            result.append(ch.toUpper());
            continue;
        }
        const QChar prev = name.at(i - 1);
        if (ch.isUpper() && !prev.isUpper()) {
            result.append(' ');
        } else if (ch.isDigit() && !prev.isDigit()) {
            result.append(' ');
        }
        result.append(ch);
    }
    return result;
}

QString colorToString(const QColor &color)
{
    QColor c(color);
    if (!c.isValid()) { c = QColor(Qt::black); }
    return c.name(QColor::HexArgb);
}

bool stringToColor(const QString &value, QColor &color)
{
    QColor c(value);
    if (!c.isValid()) { return false; }
    color = c;
    return true;
}
}

ThemeSettingsWidget::ThemeSettingsWidget(QWidget *parent)
    : SettingsWidget(parent)
{
    mColorItems.reserve(static_cast<int>(kColorDescriptorCount));

    setupHeader();
    setupColorGrid();
    loadThemePresets();
    applyThemeToButtons(mSett.fColors);
    updateRemoveButtonState();
}

void ThemeSettingsWidget::setupHeader()
{
    auto *layout = new QHBoxLayout;
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(6);

    auto *label = new QLabel(tr("Theme"), this);
    label->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);

    mThemeSelector = new QComboBox(this);
    mThemeSelector->setFocusPolicy(Qt::ClickFocus);
    mThemeSelector->setSizePolicy(QSizePolicy::Expanding,
                                 QSizePolicy::Preferred);
    mThemeSelector->setToolTip(tr("Select a Theme from the list.\n"
                                 "In case there is no Theme available you can\n"
                                 "save a new one by clicking on the '+' button."));

    const auto mAddButton = new QPushButton(QIcon::fromTheme("plus"),
                                            QString(), this);
    mAddButton->setToolTip(tr("Save as New Theme"));
    mAddButton->setFocusPolicy(Qt::NoFocus);
    const auto mRemoveButton = new QPushButton(QIcon::fromTheme("minus"),
                                            QString(), this);
    mRemoveButton->setToolTip(tr("Delete Current Theme"));
    mRemoveButton->setFocusPolicy(Qt::NoFocus);
    const auto mExportButton = new QPushButton(QIcon::fromTheme("file-export"),
                                            QString(), this);
    mExportButton->setToolTip(tr("Export Active Theme to file"));
    mExportButton->setFocusPolicy(Qt::NoFocus);
    const auto mImportButton = new QPushButton(QIcon::fromTheme("file-import"),
                                            QString(), this);
    mImportButton->setToolTip(tr("Import Theme from file"));
    mImportButton->setFocusPolicy(Qt::NoFocus);

    // mAddButton->setFixedWidth(28);
    // mRemoveButton->setFixedWidth(28);
    // mRemoveButton->setEnabled(false);

    layout->addWidget(label);
    layout->addWidget(mThemeSelector, 1);
    layout->addWidget(mAddButton);
    layout->addWidget(mRemoveButton);
    layout->addWidget(mExportButton);
    layout->addWidget(mImportButton);

    addLayout(layout);

    connect(mThemeSelector, &QComboBox::currentTextChanged,
            this, &ThemeSettingsWidget::onThemeSelected);
    connect(mAddButton, &QPushButton::clicked,
            this, &ThemeSettingsWidget::onAddTheme);
    connect(mRemoveButton, &QPushButton::clicked,
            this, &ThemeSettingsWidget::onRemoveTheme);
    connect(mExportButton, &QPushButton::clicked,
            this, &ThemeSettingsWidget::onExportTheme);
    connect(mImportButton, &QPushButton::clicked,
            this, &ThemeSettingsWidget::onImportTheme);
}

void ThemeSettingsWidget::setupColorGrid()
{
    const auto scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);

    const auto container = new QWidget(scrollArea);
    const auto layout = new QGridLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setHorizontalSpacing(12);
    layout->setVerticalSpacing(6);
    layout->setColumnStretch(1, 1);

    int row = 0;
    for (const auto &descriptor : kColorDescriptors) {
        ColorItem item;
        item.displayName = formatColorName(QString::fromLatin1(descriptor.name));
        item.member = descriptor.member;

        auto *label = new QLabel(item.displayName, container);
        label->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);

        auto *button = new ColorAnimatorButton(mSett.fColors.*(descriptor.member), container);
        button->setFixedWidth(80);
        item.button = button;

        mColorItems.append(item);

        layout->addWidget(label, row, 0);
        layout->addWidget(button, row, 1, 1, 1, Qt::AlignLeft);
        ++row;
    }

    layout->setRowStretch(row, 1);
    container->setLayout(layout);

    scrollArea->setWidget(container);
    addWidget(scrollArea);
}

void ThemeSettingsWidget::loadThemePresets()
{
    mThemePresets.clear();
    mThemeOrder.clear();

    const ThemeColors defaultColors = mSett.getDefaultThemeColors();
    insertOrUpdateTheme(kDefaultThemeId, defaultColors, true);

    const QString stored = AppSupport::getSettings(kThemesGroup,
                                                   kThemesPresetsKey,
                                                   QString()).toString();
    if (!stored.isEmpty()) {
        QJsonParseError parseError{};
        const auto doc = QJsonDocument::fromJson(stored.toUtf8(), &parseError);
        if (parseError.error == QJsonParseError::NoError && doc.isObject()) {
            const auto themesObject = doc.object().value(QStringLiteral("themes")).toObject();
            for (auto it = themesObject.begin(); it != themesObject.end(); ++it) {
                if (!it.value().isObject()) { continue; }
                ThemeColors colors = defaultColors;
                if (deserializeColors(it.value().toObject(), colors)) {
                    insertOrUpdateTheme(it.key(), colors, false);
                }
            }
        }
    }

    QString desired = AppSupport::getSettings(kThemesGroup,
                                              kThemesActiveKey,
                                              QString()).toString();
    const QString matched = findMatchingTheme(mSett.fColors);
    if (!matched.isEmpty()) { desired = matched; }

    refreshThemeSelector(desired.isEmpty() ? kDefaultThemeId : desired);
    mSett.fActiveThemeName = mCurrentTheme;
    updateRemoveButtonState();
}

void ThemeSettingsWidget::refreshThemeSelector(const QString &selectedTheme)
{
    if (!mThemeSelector) { return; }

    QSignalBlocker blocker(mThemeSelector);
    mThemeSelector->clear();

    for (const auto &name : mThemeOrder) {
        if (!mThemePresets.contains(name)) { continue; }
        mThemeSelector->addItem(name);
    }

    int index = selectedTheme.isEmpty() ? -1
                                        : mThemeSelector->findText(selectedTheme, Qt::MatchExactly);
    if (index < 0 && mThemeSelector->count() > 0) {
        index = 0;
    }

    if (index >= 0) {
        mThemeSelector->setCurrentIndex(index);
        mCurrentTheme = mThemeSelector->currentText();
    } else {
        mCurrentTheme.clear();
    }
}

void ThemeSettingsWidget::updateRemoveButtonState()
{
    if (!mRemoveButton) { return; }
    bool enabled = false;
    if (!mCurrentTheme.isEmpty()) {
        const auto it = mThemePresets.constFind(mCurrentTheme);
        enabled = it != mThemePresets.cend() && !it->readOnly;
    }
    mRemoveButton->setEnabled(enabled);
}

void ThemeSettingsWidget::saveThemePresets() const
{
    QJsonObject themesObject;
    for (auto it = mThemePresets.constBegin(); it != mThemePresets.constEnd(); ++it) {
        if (it->readOnly) { continue; }
        themesObject.insert(it.key(), serializeColors(it->colors));
    }

    QJsonObject root;
    if (!themesObject.isEmpty()) {
        root.insert(QStringLiteral("themes"), themesObject);
    }

    const auto json = QJsonDocument(root).toJson(QJsonDocument::Compact);
    AppSupport::setSettings(kThemesGroup, kThemesPresetsKey, QString::fromUtf8(json));
}

void ThemeSettingsWidget::saveActiveTheme()
{
    mSett.fActiveThemeName = mCurrentTheme;
    AppSupport::setSettings(kThemesGroup, kThemesActiveKey, mCurrentTheme);
    storeCurrentColors();
}

void ThemeSettingsWidget::storeCurrentColors() const
{
    const QJsonObject colorsObject = serializeColors(mSett.fColors);
    const auto json = QJsonDocument(colorsObject).toJson(QJsonDocument::Compact);
    AppSupport::setSettings(kThemesGroup, kThemesCurrentColorsKey, QString::fromUtf8(json));
}


void ThemeSettingsWidget::syncActiveThemeFromButtons()
{
    if (mCurrentTheme.isEmpty()) { return; }
    const auto it = mThemePresets.find(mCurrentTheme);
    if (it == mThemePresets.end() || it->readOnly) { return; }

    const ThemeColors updated = collectColorsFromButtons();
    if (!colorsEqual(it->colors, updated)) {
        it->colors = updated;
        saveThemePresets();
    }
    saveActiveTheme();
}

void ThemeSettingsWidget::onThemeSelected(const QString &themeName)
{
    if (!mThemePresets.contains(themeName)) {
        mCurrentTheme.clear();
        updateRemoveButtonState();
        return;
    }

    const auto entry = mThemePresets.value(themeName);
    mCurrentTheme = themeName;
    applyThemeToButtons(entry.colors);
    saveActiveTheme();
    updateRemoveButtonState();
}

void ThemeSettingsWidget::onAddTheme()
{
    const ThemeColors colors = collectColorsFromButtons();

    QString suggestion = mCurrentTheme;
    if (suggestion.isEmpty() || suggestion == kDefaultThemeId) {
        suggestion = tr("My Theme");
    }

    bool ok = false;
    QString name = QInputDialog::getText(this,
                                         tr("Guardar tema"),
                                         tr("Nombre del tema:"),
                                         QLineEdit::Normal,
                                         suggestion,
                                         &ok);
    if (!ok) { return; }

    name = sanitizeThemeName(name);
    if (name.isEmpty()) {
        QMessageBox::warning(this, tr("Nombre inválido"),
                             tr("Debes introducir un nombre para el tema."));
        return;
    }
    if (name == kDefaultThemeId) {
        QMessageBox::warning(this, tr("Nombre reservado"),
                             tr("El nombre '%1' está reservado.").arg(kDefaultThemeId));
        return;
    }

    if (mThemePresets.contains(name)) {
        const auto answer = QMessageBox::question(this,
                                                  tr("Reemplazar tema"),
                                                  tr("Ya existe un tema llamado '%1'. ¿Quieres reemplazarlo?").arg(name));
        if (answer != QMessageBox::Yes) { return; }
    }

    insertOrUpdateTheme(name, colors, false);
    refreshThemeSelector(name);
    applyThemeToButtons(colors);
    saveThemePresets();
    saveActiveTheme();
    updateRemoveButtonState();
}

void ThemeSettingsWidget::onRemoveTheme()
{
    if (mCurrentTheme.isEmpty()) { return; }
    const auto it = mThemePresets.find(mCurrentTheme);
    if (it == mThemePresets.end() || it->readOnly) { return; }

    const auto answer = QMessageBox::question(this,
                                              tr("Eliminar tema"),
                                              tr("¿Seguro que quieres eliminar '%1'?").arg(mCurrentTheme));
    if (answer != QMessageBox::Yes) { return; }

    const int index = mThemeOrder.indexOf(mCurrentTheme);
    if (index >= 0) { mThemeOrder.removeAt(index); }
    mThemePresets.erase(it);

    const QString fallback = !mThemeOrder.isEmpty() ? mThemeOrder.first() : QString();
    refreshThemeSelector(fallback);
    if (!mCurrentTheme.isEmpty() && mThemePresets.contains(mCurrentTheme)) {
        applyThemeToButtons(mThemePresets.value(mCurrentTheme).colors);
    } else if (!fallback.isEmpty() && mThemePresets.contains(fallback)) {
        applyThemeToButtons(mThemePresets.value(fallback).colors);
    }
    saveThemePresets();
    saveActiveTheme();
    updateRemoveButtonState();
}

void ThemeSettingsWidget::onExportTheme()
{
    const ThemeColors colors = collectColorsFromButtons();
    const QString defaultName = mCurrentTheme.isEmpty() ? QStringLiteral("theme.json")
                                                        : QStringLiteral("%1.json").arg(mCurrentTheme);
    const QString fileName = QFileDialog::getSaveFileName(this,
                                                          tr("Exportar tema"),
                                                          defaultName,
                                                          tr("Archivos de tema (*.json *.txt);;Todos los archivos (*.*)"));
    if (fileName.isEmpty()) { return; }

    QJsonObject root;
    root.insert(QStringLiteral("name"), mCurrentTheme.isEmpty() ? QStringLiteral("Theme") : mCurrentTheme);
    root.insert(QStringLiteral("colors"), serializeColors(colors));

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("No se pudo guardar"),
                             tr("No se pudo escribir el archivo '%1'.").arg(QFileInfo(fileName).fileName()));
        return;
    }

    const QByteArray data = QJsonDocument(root).toJson(QJsonDocument::Indented);
    if (file.write(data) != data.size()) {
        QMessageBox::warning(this, tr("No se pudo guardar"),
                             tr("No se pudo escribir el archivo '%1'.").arg(QFileInfo(fileName).fileName()));
    }
}

void ThemeSettingsWidget::onImportTheme()
{
    const QString fileName = QFileDialog::getOpenFileName(this,
                                                          tr("Importar tema"),
                                                          QString(),
                                                          tr("Archivos de tema (*.json *.txt);;Todos los archivos (*.*)"));
    if (fileName.isEmpty()) { return; }

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, tr("No se pudo abrir"),
                             tr("No se pudo abrir el archivo '%1'.").arg(QFileInfo(fileName).fileName()));
        return;
    }

    const QByteArray data = file.readAll();
    QJsonParseError parseError{};
    const auto doc = QJsonDocument::fromJson(data, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        QMessageBox::warning(this, tr("Archivo inválido"),
                             tr("El archivo no contiene un tema válido."));
        return;
    }

    const auto object = doc.object();
    const QString nameValue = sanitizeThemeName(object.value(QStringLiteral("name")).toString());
    const auto colorsObject = object.value(QStringLiteral("colors")).toObject();
    ThemeColors colors = mSett.getDefaultThemeColors();
    if (!deserializeColors(colorsObject, colors)) {
        QMessageBox::warning(this, tr("Archivo inválido"),
                             tr("El archivo no contiene colores válidos."));
        return;
    }

    QString themeName = nameValue;
    if (themeName.isEmpty()) { themeName = QFileInfo(fileName).completeBaseName(); }
    if (themeName.isEmpty()) {
        QMessageBox::warning(this, tr("Nombre inválido"),
                             tr("No se pudo determinar un nombre para el tema."));
        return;
    }
    if (themeName == kDefaultThemeId) {
        QMessageBox::warning(this, tr("Nombre reservado"),
                             tr("El nombre '%1' está reservado.").arg(kDefaultThemeId));
        return;
    }

    if (mThemePresets.contains(themeName)) {
        const auto answer = QMessageBox::question(this,
                                                  tr("Reemplazar tema"),
                                                  tr("Ya existe un tema llamado '%1'. ¿Quieres reemplazarlo?").arg(themeName));
        if (answer != QMessageBox::Yes) { return; }
    }

    insertOrUpdateTheme(themeName, colors, false);
    refreshThemeSelector(themeName);
    applyThemeToButtons(colors);
    saveThemePresets();
    saveActiveTheme();
    updateRemoveButtonState();
}

ThemeSettingsWidget::ThemeColors ThemeSettingsWidget::collectColorsFromButtons() const
{
    ThemeColors colors = mSett.fColors;
    for (const auto &item : mColorItems) {
        if (!item.button || !item.member) { continue; }
        colors.*(item.member) = item.button->color();
    }
    return colors;
}

void ThemeSettingsWidget::applyThemeToButtons(const ThemeColors &colors)
{
    mSett.fColors = colors;
    for (auto &item : mColorItems) {
        if (!item.button || !item.member) { continue; }
        item.button->setColor(mSett.fColors.*(item.member));
    }
    storeCurrentColors();
}

QString ThemeSettingsWidget::findMatchingTheme(const ThemeColors &colors) const
{
    for (auto it = mThemePresets.constBegin(); it != mThemePresets.constEnd(); ++it) {
        if (colorsEqual(it->colors, colors)) { return it.key(); }
    }
    return QString();
}

bool ThemeSettingsWidget::colorsEqual(const ThemeColors &lhs, const ThemeColors &rhs) const
{
    for (const auto &descriptor : kColorDescriptors) {
        if (lhs.*(descriptor.member) != rhs.*(descriptor.member)) { return false; }
    }
    return true;
}

bool ThemeSettingsWidget::insertOrUpdateTheme(const QString &name,
                                              const ThemeColors &colors,
                                              bool readOnly)
{
    if (name.isEmpty()) { return false; }

    const bool contains = mThemePresets.contains(name);
    ThemeEntry entry{colors, readOnly};
    mThemePresets.insert(name, entry);
    if (!contains) {
        mThemeOrder.append(name);
    }
    return true;
}

bool ThemeSettingsWidget::deserializeColors(const QJsonObject &object,
                                            ThemeColors &colors) const
{
    for (const auto &descriptor : kColorDescriptors) {
        const auto value = object.value(QString::fromLatin1(descriptor.name));
        if (!value.isString()) { return false; }
        QColor color;
        if (!stringToColor(value.toString(), color)) { return false; }
        colors.*(descriptor.member) = color;
    }
    return true;
}

QJsonObject ThemeSettingsWidget::serializeColors(const ThemeColors &colors) const
{
    QJsonObject object;
    for (const auto &descriptor : kColorDescriptors) {
        object.insert(QString::fromLatin1(descriptor.name),
                      colorToString(colors.*(descriptor.member)));
    }
    return object;
}

QString ThemeSettingsWidget::sanitizeThemeName(const QString &name) const
{
    QString out = name.trimmed();
    out.replace(QLatin1Char('\n'), QLatin1Char(' '));
    out.replace(QLatin1Char('\r'), QLatin1Char(' '));
    return out.simplified();
}

void ThemeSettingsWidget::applySettings()
{
    mSett.fColors = collectColorsFromButtons();
    storeCurrentColors();
    syncActiveThemeFromButtons();
}

void ThemeSettingsWidget::updateSettings(bool restore)
{
    Q_UNUSED(restore)
    const QString matched = findMatchingTheme(mSett.fColors);
    refreshThemeSelector(matched.isEmpty() ? mCurrentTheme : matched);
    applyThemeToButtons(mSett.fColors);
    updateRemoveButtonState();
}
