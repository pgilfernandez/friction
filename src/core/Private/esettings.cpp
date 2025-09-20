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

#include "esettings.h"

#include "GUI/global.h"
#include "exceptions.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>

#include "smartPointers/stdselfref.h"
#include "skia/skqtconversions.h"

namespace {
using ThemeColors = Friction::Core::Theme::Colors;

struct ColorDescriptor {
    const char *name;
    QColor ThemeColors::*member;
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

const QString kThemesGroup = QStringLiteral("themes");
const QString kThemesCurrentColorsKey = QStringLiteral("currentColors");
const QString kThemesActiveKey = QStringLiteral("active");

bool deserializeColors(const QJsonObject &object, ThemeColors &colors)
{
    for (const auto &descriptor : kColorDescriptors) {
        const auto value = object.value(QString::fromLatin1(descriptor.name));
        if (value.isUndefined()) { continue; }
        if (!value.isString()) { return false; }
        QColor color(value.toString());
        if (!color.isValid()) { return false; }
        colors.*(descriptor.member) = color;
    }
    return true;
}

void loadStoredTheme(ThemeColors &colors, QString &activeName)
{
    const QString storedColors = AppSupport::getSettings(kThemesGroup,
                                                         kThemesCurrentColorsKey,
                                                         QString()).toString();
    if (!storedColors.isEmpty()) {
        QJsonParseError parseError{};
        const auto doc = QJsonDocument::fromJson(storedColors.toUtf8(), &parseError);
        if (parseError.error == QJsonParseError::NoError && doc.isObject()) {
            ThemeColors temp = colors;
            if (deserializeColors(doc.object(), temp)) {
                colors = temp;
            }
        }
    }
    activeName = AppSupport::getSettings(kThemesGroup,
                                         kThemesActiveKey,
                                         QString()).toString();
}
} // namespace

struct eSetting
{
    eSetting(const QString& name) : fName(name) {}
    virtual ~eSetting() = default;

    virtual bool setValueString(const QString& value) = 0;
    virtual void writeValue() const = 0;
    virtual void loadDefault() = 0;

    const QString fName;
};

template <typename T>
struct eSettingBase : public eSetting
{
    eSettingBase(T &value,
                 const QString &name,
                 const T &defaultValue)
        : eSetting(name)
        , fValue(value)
        , fDefault(defaultValue) {}

    void loadDefault() { fValue = fDefault; };

    T &fValue;
    const T fDefault;
};

struct eBoolSetting : public eSettingBase<bool>
{
    using eSettingBase<bool>::eSettingBase;

    bool setValueString(const QString& valueStr) {
        const bool value = valueStr == "enabled";
        bool ok = value || valueStr == "disabled";
        if(ok) fValue = value;
        return ok;
    }

    void writeValue() const {
        AppSupport::setSettings("settings", fName, fValue ? "enabled" : "disabled");
    }
};

struct eIntSetting : public eSettingBase<int>
{
    using eSettingBase<int>::eSettingBase;

    bool setValueString(const QString& valueStr) {
        bool ok;
        const int value = valueStr.toInt(&ok);
        if(ok) fValue = value;
        return ok;
    }

    void writeValue() const {
        AppSupport::setSettings("settings", fName, fValue);
    }
};

struct eQrealSetting : public eSettingBase<qreal>
{
    using eSettingBase<qreal>::eSettingBase;

    bool setValueString(const QString& valueStr) {
        bool ok;
        const qreal value = valueStr.toDouble(&ok);
        if(ok) fValue = value;
        return ok;
    }

    void writeValue() const {
        AppSupport::setSettings("settings", fName, fValue);
    }
};

struct eStringSetting : public eSettingBase<QString>
{
    using eSettingBase<QString>::eSettingBase;

    bool setValueString(const QString& valueStr) {
        fValue = valueStr;
        return true;
    }

    void writeValue() const {
        AppSupport::setSettings("settings", fName, fValue);
    }
};

struct eColorSetting : public eSettingBase<QColor>
{
    using eSettingBase<QColor>::eSettingBase;

    bool setValueString(const QString& valueStr) {
        const QString oneVal = QStringLiteral("\\s*(\\d+)\\s*");
        const QString oneValC = QStringLiteral("\\s*(\\d+)\\s*,");

        QRegExp rx("rgba\\("
                        "\\s*(\\d+)\\s*,"
                        "\\s*(\\d+)\\s*,"
                        "\\s*(\\d+)\\s*,"
                        "\\s*(\\d+)\\s*"
                   "\\)",
                   Qt::CaseInsensitive);
        if(rx.exactMatch(valueStr)) {
            rx.indexIn(valueStr);
            const QStringList intRGBA = rx.capturedTexts();
            fValue.setRgb(intRGBA.at(1).toInt(),
                          intRGBA.at(2).toInt(),
                          intRGBA.at(3).toInt(),
                          intRGBA.at(4).toInt());
            return true;
        } else return false;
    }

    void writeValue() const {
        QString col = QString("rgba(%1, %2, %3, %4)").
                              arg(fValue.red()).
                              arg(fValue.green()).
                              arg(fValue.blue()).
                              arg(fValue.alpha());
        AppSupport::setSettings("settings", fName, col);
    }
};

eSettings* eSettings::sInstance = nullptr;

static QList<stdsptr<eSetting>> gSettings;

eSettings::eSettings(const int cpuThreads,
                     const intKB ramKB)
    : fUserSettingsDir(AppSupport::getAppConfigPath())
    , fCpuThreads(cpuThreads)
    , fRamKB(ramKB)
{
    Q_ASSERT(!sInstance);
    sInstance = this;

    fColors = getDefaultThemeColors();
    loadStoredTheme(fColors, fActiveThemeName);
    //fVisibilityRangeColor = fColors.timelineRange;
    //fSelectedVisibilityRangeColor = fColors.timelineRangeSelected;
    //fTimelineHighlightRowColor = Friction::Core::Theme::transparentColor(fColors.highlight, 15);
    fLastUsedStrokeColor = AppSupport::getSettings("FillStroke",
                                                   "LastStrokeColor",
                                                   fColors.defaultStroke).value<QColor>();
    fLastUsedFillColor = AppSupport::getSettings("FillStroke",
                                                 "LastFillColor",
                                                  fColors.defaultFill).value<QColor>();
    //fTimelineAlternateRowColor = Friction::Core::Theme::transparentColor(fColors.black, 25);
    //fAnimationRangeColor = Friction::Core::Theme::transparentColor(fColors.black, 55);

    gSettings << std::make_shared<eIntSetting>(
                     fCpuThreadsCap,
                     "cpuThreadsCap", 0);
    gSettings << std::make_shared<eIntSetting>(
                     reinterpret_cast<int&>(fRamMBCap),
                     "ramMBCap", 0);
    gSettings << std::make_shared<eIntSetting>(
                     reinterpret_cast<int&>(fAccPreference),
                     "accPreference",
                     static_cast<int>(AccPreference::defaultPreference));
    gSettings << std::make_shared<eBoolSetting>(
                     fPathGpuAcc,
                     "pathGpuAcc", true);
    gSettings << std::make_shared<eIntSetting>(
                     fInternalMultisampleCount,
                     "msaa", 4);
    gSettings << std::make_shared<eBoolSetting>(
                     fHddCache,
                     "hddCache", true);
    gSettings << std::make_shared<eIntSetting>(
                     reinterpret_cast<int&>(fHddCacheMBCap),
                     "hddCacheMBCap", 0);

    gSettings << std::make_shared<eQrealSetting>(
                     fInterfaceScaling,
                     "interfaceScaling", 1.);
    gSettings << std::make_shared<eBoolSetting>(
                     fDefaultInterfaceScaling,
                     "defaultInterfaceScaling", true);

    gSettings << std::make_shared<eIntSetting>(fImportFileDirOpt,
                                               "ImportFileDirOpt",
                                               ImportFileDirRecent);

    gSettings << std::make_shared<eBoolSetting>(
                     fCanvasRtlSupport,
                     "rtlTextSupport", false);

    /*gSettings << std::make_shared<eColorSetting>(
                     fPathNodeColor,
                     "pathNodeColor",
                     fColors.pathNode);
    gSettings << std::make_shared<eColorSetting>(
                     fPathNodeSelectedColor,
                     "pathNodeSelectedColor",
                     fColors.pathNodeSelected);*/
    gSettings << std::make_shared<eQrealSetting>(
                     fPathNodeScaling,
                     "pathNodeScaling", 1.);

    /*gSettings << std::make_shared<eColorSetting>(
                     fPathDissolvedNodeColor,
                     "pathDissolvedNodeColor",
                     fColors.pathDissolvedNode);
    gSettings << std::make_shared<eColorSetting>(
                     fPathDissolvedNodeSelectedColor,
                     "pathDissolvedNodeSelectedColor",
                     fColors.pathNodeSelected);*/
    gSettings << std::make_shared<eQrealSetting>(
                     fPathDissolvedNodeScaling,
                     "pathDissolvedNodeScaling", 1.);

    /*gSettings << std::make_shared<eColorSetting>(
                     fPathControlColor,
                     "pathControlColor",
                     fColors.pathControlSelected);
    gSettings << std::make_shared<eColorSetting>(
                     fPathControlSelectedColor,
                     "pathControlSelectedColor",
                     QColor(255, 0, 0));*/
    gSettings << std::make_shared<eQrealSetting>(
                     fPathControlScaling,
                     "pathControlScaling", 1.);

    gSettings << std::make_shared<eIntSetting>(fAdjustSceneFromFirstClip,
                                               "AdjustSceneFromFirstClip",
                                               AdjustSceneAsk);

    gSettings << std::make_shared<eIntSetting>(fDefaultFillStrokeIndex,
                                              "DefaultFillStrokeIndex",
                                               0);

    gSettings << std::make_shared<eBoolSetting>(fPreviewCache,
                                                "PreviewCache",
                                                true);
    /*gSettings << std::make_shared<eBoolSetting>(
                     fTimelineAlternateRow,
                     "timelineAlternateRow", true);
    gSettings << std::make_shared<eColorSetting>(
                     fTimelineAlternateRowColor,
                     "timelineAlternateRowColor",
                     QColor(0, 0, 0, 25));
    gSettings << std::make_shared<eBoolSetting>(
                     fTimelineHighlightRow,
                     "timelineHighlightRow", true);
    gSettings << std::make_shared<eColorSetting>(
                     fTimelineHighlightRowColor,
                     "timelineHighlightRowColor",
                     QColor(255, 0, 0, 15));*/

    /*gSettings << std::make_shared<eColorSetting>(
                     fObjectKeyframeColor,
                     "objectKeyframeColor",
                     fColors.keyframeObject);
    gSettings << std::make_shared<eColorSetting>(
                     fPropertyGroupKeyframeColor,
                     "propertyGroupKeyframeColor",
                     fColors.keyframePropertyGroup);
    gSettings << std::make_shared<eColorSetting>(
                     fPropertyKeyframeColor,
                     "propertyKeyframeColor",
                     fColors.keyframeProperty);
    gSettings << std::make_shared<eColorSetting>(
                     fSelectedKeyframeColor,
                     "selectedKeyframeColor",
                     fColors.keyframeSelected);*/

    /*gSettings << std::make_shared<eColorSetting>(
                     fVisibilityRangeColor,
                     "visibilityRangeColor",
                     QColor(131, 92, 255, 255));
    gSettings << std::make_shared<eColorSetting>(
                     fSelectedVisibilityRangeColor,
                     "selectedVisibilityRangeColor",
                     QColor(116, 18, 98, 255));
    gSettings << std::make_shared<eColorSetting>(
                     fAnimationRangeColor,
                     "animationRangeColor",
                     QColor(0, 0, 0, 55));*/

    loadDefaults();

    eSizesUI::widget.add(this, [this](const int size) {
        mIconsDir = sSettingsDir() + "/icons/" + QString::number(size);
    });
}

int eSettings::sCpuThreadsCapped()
{
    if (sInstance->fCpuThreadsCap > 0) {
        return sInstance->fCpuThreadsCap;
    }
    return sInstance->fCpuThreads;
}

intMB eSettings::sRamMBCap()
{
    if (sInstance->fRamMBCap.fValue > 0) { return sInstance->fRamMBCap; }
    auto mbTot = intMB(sInstance->fRamKB);
    mbTot.fValue *= 8; mbTot.fValue /= 10;
    return mbTot;
}

const QString &eSettings::sSettingsDir()
{
    return sInstance->fUserSettingsDir;
}

const QString &eSettings::sIconsDir()
{
    return sInstance->mIconsDir;
}

const eSettings &eSettings::instance()
{
    return *sInstance;
}

void eSettings::loadDefaults()
{
    for (auto& setting : gSettings) {
        setting->loadDefault();
    }
}

void eSettings::loadFromFile()
{
    loadDefaults();

    for(auto &setting : gSettings) {
        QString val = AppSupport::getSettings("settings",
                                              setting->fName,
                                              QString()).toString();
        if (!val.isEmpty()) { setting->setValueString(val); }
    }

    eSizesUI::font.updateSize();
    eSizesUI::widget.updateSize();
}

void eSettings::saveToFile()
{
    for (const auto& setting : gSettings) {
        setting->writeValue();
    }
}

void eSettings::saveKeyToFile(const QString &key)
{
    for (const auto& setting : gSettings) {
        if (key == setting->fName) {
            setting->writeValue();
            return;
        }
    }
}

const Friction::Core::Theme::Colors eSettings::getDefaultThemeColors()
{
    Friction::Core::Theme::Colors colors;

    colors.red = QColor(199, 67, 72);
    colors.blue = QColor(73, 142, 209);
    colors.yellow = QColor(209, 183, 73);
    colors.purple = QColor(169, 73, 209);
    colors.green = QColor(73, 209, 132);
    colors.darkGreen = QColor(27, 49, 39);
    colors.orange = QColor(255, 123, 0);
    colors.gray = Qt::gray;
    colors.darkGray = Qt::darkGray;
    colors.lightGray = Qt::lightGray;
    colors.black = Qt::black;
    colors.white = Qt::white;

    colors.base = QColor(26, 26, 30);
    colors.baseAlt = QColor(33, 33, 39);
    colors.baseButton = QColor(49, 49, 59);
    colors.baseCombo = QColor(36, 36, 53);
    colors.baseBorder = QColor(65, 65, 80);
    colors.baseDark = QColor(25, 25, 25);
    colors.baseDarker = QColor(19, 19, 21);

    colors.highlight = QColor(104, 144, 206);
    colors.highlightAlt = QColor(167, 185, 222);
    colors.highlightDarker = QColor(53, 101, 176);
    colors.highlightSelected = QColor(150, 191, 255);

    colors.timelineGrid = QColor(44, 44, 49);
    colors.timelineRange = QColor(56, 73, 101);
    colors.timelineRangeSelected = QColor(87, 120, 173);
    colors.timelineHighlightRow = Friction::Core::Theme::transparentColor(colors.highlight, 15);
    colors.timelineAltRow = Friction::Core::Theme::transparentColor(colors.black, 25);
    colors.timelineAnimRange = Friction::Core::Theme::transparentColor(colors.black, 55);

    colors.keyframeObject = colors.blue;
    colors.keyframePropertyGroup = colors.green;
    colors.keyframeProperty = colors.red;
    colors.keyframeSelected = colors.yellow;

    colors.marker = colors.orange;
    colors.markerIO = colors.green;

    colors.scene = colors.base;
    colors.sceneClip = colors.black;
    colors.sceneBorder = colors.gray;

    colors.defaultStroke = QColor(0, 102, 255);
    colors.defaultFill = colors.white;

    colors.transformOverlayBase = colors.highlight;
    colors.transformOverlayAlt = colors.orange;

    colors.point = colors.red;
    colors.pointSelected = QColor(255, 175, 175);
    colors.pointHoverOutline = colors.red;
    colors.pointKeyOuter = colors.white;
    colors.pointKeyInner = colors.red;

    colors.pathNode = QColor(170, 240, 255);
    colors.pathNodeSelected = QColor(0, 200, 255);
    colors.pathDissolvedNode = QColor(255, 120, 120);
    colors.pathDissolvedNodeSelected = QColor(255, 0, 0);
    colors.pathControl = QColor(255, 175, 175);
    colors.pathControlSelected = QColor(255, 0, 0);
    colors.pathHoverOuter = colors.black;
    colors.pathHoverInner = colors.red;

    colors.segmentHoverOuter = colors.black;
    colors.segmentHoverInner = colors.red;

    colors.boundingBox = colors.lightGray;

    colors.nullObject = colors.lightGray;

    colors.textDisabled = QColor(112, 112, 113);
    colors.outputDestination = QColor(40, 40, 47); // remove

    return colors;
}
