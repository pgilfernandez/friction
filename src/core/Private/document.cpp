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

#include "Private/document.h"
#include "FileCacheHandlers/filecachehandler.h"
#include "canvas.h"
#include "simpletask.h"

#include <QVariant>
#include <QColor>

#include <cmath>

Document* Document::sInstance = nullptr;

using namespace Friction::Core;

Document::Document(TaskScheduler& taskScheduler)
{
    Q_ASSERT(!sInstance);
    sInstance = this;
    loadGridSettingsFromSettings();
    connect(&taskScheduler, &TaskScheduler::finishedAllQuedTasks,
            this, &Document::updateScenes);
}

void Document::updateScenes() {
    SimpleTask::sProcessAll();
    TaskScheduler::instance()->queTasks();

    for(const auto& scene : fVisibleScenes) {
        emit scene.first->requestUpdate();
    }
}

void Document::actionFinished() {
    updateScenes();
    for (const auto& scene : fVisibleScenes) {
        const auto newUndoRedo = scene.first->newUndoRedoSet();
        if (newUndoRedo) {
            qDebug() << "document changed";
            emit documentChanged();
        }
    }
}

void Document::replaceClipboard(const stdsptr<Clipboard> &container) {
    fClipboardContainer = container;
}

GridController& Document::gridController()
{
    return mGridController;
}

const GridController& Document::gridController() const
{
    return mGridController;
}

template<typename T>
static bool gridNearlyEqual(const T lhs, const T rhs)
{
    constexpr double eps = 1e-6;
    return std::abs(static_cast<double>(lhs) - static_cast<double>(rhs)) <= eps;
}

static GridSettings sanitizedGridSettings(GridSettings settings)
{
    if (settings.sizeX <= 0.0) { settings.sizeX = 1.0; }
    if (settings.sizeY <= 0.0) { settings.sizeY = 1.0; }
    if (settings.majorEvery < 1) { settings.majorEvery = 1; }
    if (settings.snapThresholdPx < 0) { settings.snapThresholdPx = 0; }
    auto ensureAnimatorColor = [](qsptr<ColorAnimator>& animator,
                                  const QColor& fallback)
    {
        if (!animator) { animator = enve::make_shared<ColorAnimator>(); }
        QColor color = animator->getColor();
        if (!color.isValid()) { color = fallback; }
        int alpha = color.alpha();
        if (alpha < 0) { alpha = 0; }
        if (alpha > 255) { alpha = 255; }
        color.setAlpha(alpha);
        animator->setColor(color);
        return color;
    };
    const QColor minorFallback(255, 255, 255, 96);
    const QColor majorFallback(255, 255, 255, 160);
    ensureAnimatorColor(settings.colorAnimator, minorFallback);
    ensureAnimatorColor(settings.majorColorAnimator, majorFallback);
    return settings;
}

void Document::setGridSnapEnabled(const bool enabled)
{
    auto updated = mGridController.settings;
    if (updated.enabled == enabled) { return; }
    updated.enabled = enabled;
    applyGridSettings(updated, false, false);
}

void Document::setGridVisible(const bool visible)
{
    auto updated = mGridController.settings;
    if (updated.show == visible) { return; }
    updated.show = visible;
    applyGridSettings(updated, false, false);
}

void Document::setGridSettings(const GridSettings& settings)
{
    auto updated = settings;
    updated.enabled = mGridController.settings.enabled;
    applyGridSettings(updated, false, false);
}

void Document::loadGridSettingsFromSettings()
{
    GridSettings defaults;
    GridSettings loaded = defaults;
    loaded.sizeX = AppSupport::getSettings("grid", "sizeX", defaults.sizeX).toDouble();
    loaded.sizeY = AppSupport::getSettings("grid", "sizeY", defaults.sizeY).toDouble();
    loaded.originX = AppSupport::getSettings("grid", "originX", defaults.originX).toDouble();
    loaded.originY = AppSupport::getSettings("grid", "originY", defaults.originY).toDouble();
    loaded.snapThresholdPx = AppSupport::getSettings("grid", "snapThresholdPx", defaults.snapThresholdPx).toInt();
    loaded.enabled = AppSupport::getSettings("grid", "enabled", defaults.enabled).toBool();
    loaded.show = AppSupport::getSettings("grid", "show", defaults.show).toBool();
    loaded.majorEvery = AppSupport::getSettings("grid", "majorEvery", defaults.majorEvery).toInt();
    auto readColor = [](const QVariant& variant,
                        const QColor& fallback)
    {
        QColor value;
        if (variant.canConvert<QColor>()) {
            value = variant.value<QColor>();
        } else {
            value = QColor(variant.toString());
        }
        if (!value.isValid()) { value = fallback; }
        return value;
    };
    QColor storedMinor = readColor(
        AppSupport::getSettings("grid", "color", defaults.colorAnimator->getColor()),
        QColor(255, 255, 255, 96));
    QColor storedMajor = readColor(
        AppSupport::getSettings("grid", "majorColor", defaults.majorColorAnimator->getColor()),
        QColor(255, 255, 255, 160));
    if (auto* settingsMgr = eSettings::sInstance) {
        storedMinor = settingsMgr->fGridColor;
        storedMajor = settingsMgr->fGridMajorColor;
    }
    if (!loaded.colorAnimator) { loaded.colorAnimator = enve::make_shared<ColorAnimator>(); }
    loaded.colorAnimator->setColor(storedMinor);
    if (!loaded.majorColorAnimator) { loaded.majorColorAnimator = enve::make_shared<ColorAnimator>(); }
    loaded.majorColorAnimator->setColor(storedMajor);
    applyGridSettings(loaded, true, true);
}

void Document::saveGridSettingsToSettings(const GridSettings& settings) const
{
    AppSupport::setSettings("grid", "sizeX", settings.sizeX);
    AppSupport::setSettings("grid", "sizeY", settings.sizeY);
    AppSupport::setSettings("grid", "originX", settings.originX);
    AppSupport::setSettings("grid", "originY", settings.originY);
    AppSupport::setSettings("grid", "snapThresholdPx", settings.snapThresholdPx);
    AppSupport::setSettings("grid", "enabled", settings.enabled);
    AppSupport::setSettings("grid", "show", settings.show);
    AppSupport::setSettings("grid", "majorEvery", settings.majorEvery);
    const QColor color = settings.colorAnimator ? settings.colorAnimator->getColor() : QColor(255, 255, 255, 96);
    const QColor majorColor = settings.majorColorAnimator ? settings.majorColorAnimator->getColor() : QColor(255, 255, 255, 160);
    AppSupport::setSettings("grid", "color", color);
    AppSupport::setSettings("grid", "majorColor", majorColor);
}

void Document::saveGridSettingsAsDefault(const GridSettings& settings)
{
    const GridSettings sanitized = sanitizedGridSettings(settings);
    if (auto* settingsMgr = eSettings::sInstance) {
        settingsMgr->fGridColor = sanitized.colorAnimator ? sanitized.colorAnimator->getColor() : QColor(255, 255, 255, 96);
        settingsMgr->fGridMajorColor = sanitized.majorColorAnimator ? sanitized.majorColorAnimator->getColor() : QColor(255, 255, 255, 160);
        settingsMgr->saveKeyToFile("gridColor");
        settingsMgr->saveKeyToFile("gridMajorColor");
    }
    saveGridSettingsToSettings(sanitized);
}

void Document::applyGridSettings(const GridSettings& settings,
                                 const bool silent,
                                 const bool skipSave)
{
    const GridSettings sanitized = sanitizedGridSettings(settings);
    const auto previous = mGridController.settings;
    if (previous == sanitized) { return; }

    const bool snapChanged = previous.enabled != sanitized.enabled;
    const bool showChanged = previous.show != sanitized.show;
    const bool metricsChanged =
            gridNearlyEqual(previous.sizeX, sanitized.sizeX) == false ||
            gridNearlyEqual(previous.sizeY, sanitized.sizeY) == false ||
            gridNearlyEqual(previous.originX, sanitized.originX) == false ||
            gridNearlyEqual(previous.originY, sanitized.originY) == false ||
            previous.majorEvery != sanitized.majorEvery;
    const QColor previousColor = previous.colorAnimator ? previous.colorAnimator->getColor() : QColor();
    const QColor sanitizedColor = sanitized.colorAnimator ? sanitized.colorAnimator->getColor() : QColor();
    const QColor previousMajorColor = previous.majorColorAnimator ? previous.majorColorAnimator->getColor() : QColor();
    const QColor sanitizedMajorColor = sanitized.majorColorAnimator ? sanitized.majorColorAnimator->getColor() : QColor();
    const bool colorChanged = previousColor != sanitizedColor ||
                              previousMajorColor != sanitizedMajorColor;

    mGridController.settings = sanitized;

    if (silent) { return; }

    emit gridSettingsChanged(mGridController.settings);
    if (snapChanged) {
        emit gridSnapEnabledChanged(mGridController.settings.enabled);
    }

    if (showChanged || (mGridController.settings.show && (metricsChanged || colorChanged))) {
        updateScenes();
    }
}


Clipboard *Document::getClipboard(const ClipboardType type) const {
    if(!fClipboardContainer) return nullptr;
    if(type == fClipboardContainer->getType())
        return fClipboardContainer.get();
    return nullptr;
}

DynamicPropsClipboard* Document::getDynamicPropsClipboard() const {
    auto contT = getClipboard(ClipboardType::dynamicProperties);
    return static_cast<DynamicPropsClipboard*>(contT);
}

PropertyClipboard* Document::getPropertyClipboard() const {
    auto contT = getClipboard(ClipboardType::property);
    return static_cast<PropertyClipboard*>(contT);
}

KeysClipboard* Document::getKeysClipboard() const {
    auto contT = getClipboard(ClipboardType::keys);
    return static_cast<KeysClipboard*>(contT);
}

BoxesClipboard* Document::getBoxesClipboard() const {
    auto contT = getClipboard(ClipboardType::boxes);
    return static_cast<BoxesClipboard*>(contT);
}

SmartPathClipboard* Document::getSmartPathClipboard() const {
    auto contT = getClipboard(ClipboardType::smartPath);
    return static_cast<SmartPathClipboard*>(contT);
}

void Document::setPath(const QString &path) {
    fEvFile = path;
    emit evFilePathChanged(fEvFile);
}

QString Document::projectDirectory() const {
    if(fEvFile.isEmpty()) {
        return QDir::homePath();
    } else {
        QFileInfo fileInfo(fEvFile);
        return fileInfo.dir().path();
    }
}

void Document::setCanvasMode(const CanvasMode mode) {
    fCanvasMode = mode;
    emit canvasModeSet(mode);
    actionFinished();
}

void Document::setGizmoVisibility(const Gizmos::Interact &ti,
                                  const bool visibility)
{
    QString key;

    switch (ti) {
    case Gizmos::Interact::Position:
        if (fGizmoPositionVisibility == visibility) { return; }
        key = "Position";
        fGizmoPositionVisibility = visibility;
        break;
    case Gizmos::Interact::Rotate:
        if (fGizmoRotateVisibility == visibility) { return; }
        key = "Rotate";
        fGizmoRotateVisibility = visibility;
        break;
    case Gizmos::Interact::Scale:
        if (fGizmoScaleVisibility == visibility) { return; }
        key = "Scale";
        fGizmoScaleVisibility = visibility;
        break;
    case Gizmos::Interact::Shear:
        if (fGizmoShearVisibility == visibility) { return; }
        key = "Shear";
        fGizmoShearVisibility = visibility;
        break;
    default: return;
    }

    for (const auto &scene : fScenes) {
        if (scene) { scene->setGizmoVisibility(ti, visibility); }
    }

    AppSupport::setSettings("gizmos", key, visibility);
    emit gizmoVisibilityChanged(ti, visibility);
}

bool Document::getGizmoVisibility(const Gizmos::Interact &ti)
{
    switch (ti) {
    case Gizmos::Interact::Position:
        return fGizmoPositionVisibility;
        break;
    case Gizmos::Interact::Rotate:
        return fGizmoRotateVisibility;
        break;
    case Gizmos::Interact::Scale:
        return fGizmoScaleVisibility;
        break;
    case Gizmos::Interact::Shear:
        return fGizmoShearVisibility;
        break;
    default:;
    }
    return false;
}

Canvas *Document::createNewScene(const bool emitCreated)
{
    const auto newScene = enve::make_shared<Canvas>(*this);
    fScenes.append(newScene);
    SWT_addChild(newScene.get());

    newScene->setGizmoVisibility(Gizmos::Interact::Position,
                                 fGizmoPositionVisibility);
    newScene->setGizmoVisibility(Gizmos::Interact::Rotate,
                                 fGizmoRotateVisibility);
    newScene->setGizmoVisibility(Gizmos::Interact::Scale,
                                 fGizmoScaleVisibility);
    newScene->setGizmoVisibility(Gizmos::Interact::Shear,
                                 fGizmoShearVisibility);

    if (emitCreated) {
        emit sceneCreated(newScene.get());
    }
    return newScene.get();
}

bool Document::removeScene(const qsptr<Canvas>& scene) {
    const int id = fScenes.indexOf(scene);
    return removeScene(id);
}

bool Document::removeScene(const int id) {
    if(id < 0 || id >= fScenes.count()) return false;
    const auto scene = fScenes.takeAt(id);
    SWT_removeChild(scene.data());
    emit sceneRemoved(scene.data());
    emit sceneRemoved(id);
    return true;
}

void Document::addVisibleScene(Canvas * const scene) {
    fVisibleScenes[scene]++;
    updateScenes();
}

bool Document::removeVisibleScene(Canvas * const scene) {
    const auto it = fVisibleScenes.find(scene);
    if(it == fVisibleScenes.end()) return false;
    if(it->second == 1) fVisibleScenes.erase(it);
    else it->second--;
    return true;
}

void Document::setActiveScene(Canvas * const scene) {
    if(scene == fActiveScene) return;
    auto& conn = fActiveScene.assign(scene);
    if(fActiveScene) {
        conn << connect(fActiveScene, &Canvas::currentBoxChanged,
                        this, &Document::currentBoxChanged);
        conn << connect(fActiveScene, &Canvas::selectedPaintSettingsChanged,
                        this, &Document::selectedPaintSettingsChanged);
        conn << connect(fActiveScene, &Canvas::destroyed,
                        this, &Document::clearActiveScene);
        conn << connect(fActiveScene, &Canvas::openTextEditor,
                        this, [this] () { emit openTextEditor(); });
        conn << connect(fActiveScene, &Canvas::openMarkerEditor,
                        this, [this] () { emit openMarkerEditor(); });
        conn << connect(fActiveScene, &Canvas::openExpressionDialog,
                        this, [this](QrealAnimator* const target) {
            emit openExpressionDialog(target);
        });
        conn << connect(fActiveScene, &Canvas::openApplyExpressionDialog,
                        this, [this](QrealAnimator* const target) {
            emit openApplyExpressionDialog(target);
        });
        conn << connect(fActiveScene, &Canvas::currentHoverColor,
                        this, [this](const QColor &color) {
            emit currentPixelColor(color);
        });
        emit currentBoxChanged(fActiveScene->getCurrentBox());
        emit selectedPaintSettingsChanged();
    }
    emit activeSceneSet(scene);
}

void Document::clearActiveScene() {
    setActiveScene(nullptr);
}

int Document::getActiveSceneFrame() const {
    if(!fActiveScene) return 0;
    return fActiveScene->anim_getCurrentAbsFrame();
}

void Document::setActiveSceneFrame(const int frame) {
    if(!fActiveScene) return;
    if(fActiveScene->anim_getCurrentRelFrame() == frame) return;
    fActiveScene->anim_setAbsFrame(frame);
    emit activeSceneFrameSet(frame);
}

void Document::incActiveSceneFrame() {
    setActiveSceneFrame(getActiveSceneFrame() + 1);
}

void Document::decActiveSceneFrame() {
    setActiveSceneFrame(getActiveSceneFrame() - 1);
}

void Document::addBookmarkBrush(SimpleBrushWrapper * const brush) {
    if(!brush) return;
    removeBookmarkBrush(brush);
    fBrushes << brush;
    emit bookmarkBrushAdded(brush);
}

void Document::removeBookmarkBrush(SimpleBrushWrapper * const brush) {
    if(fBrushes.removeOne(brush))
        emit bookmarkBrushRemoved(brush);
}

void Document::addBookmarkColor(const QColor &color) {
    removeBookmarkColor(color);
    fColors << color;
    emit bookmarkColorAdded(color);
}

void Document::removeBookmarkColor(const QColor &color)
{
    const auto rgba = color.rgba();
    for (int i = 0; i < fColors.count(); i++) {
        if (fColors.at(i).rgba() == rgba) {
            fColors.removeAt(i);
            emit bookmarkColorRemoved(color);
            break;
        }
    }
}

void Document::setBrush(BrushContexedWrapper * const brush) {
    fBrush = brush->getSimpleBrush();
    if(fBrush) {
        fBrush->setColor(fBrushColor);
        switch(fPaintMode) {
        case PaintMode::normal: fBrush->setNormalMode(); break;
        case PaintMode::erase: fBrush->startEraseMode(); break;
        case PaintMode::lockAlpha: fBrush->startAlphaLockMode(); break;
        case PaintMode::colorize: fBrush->startColorizeMode(); break;
        default: break;
        }
    }
    emit brushChanged(brush);
    emit brushSizeChanged(fBrush ? fBrush->getBrushSize() : 0.f);
    emit brushColorChanged(fBrush ? fBrush->getColor() : Qt::white);
}

void Document::setBrushColor(const QColor &color) {
    fBrushColor = color;
    if(fBrush) fBrush->setColor(fBrushColor);
    emit brushColorChanged(color);
}

void Document::incBrushRadius() {
    if(!fBrush) return;
    fBrush->incPaintBrushSize(0.3);
    emit brushSizeChanged(fBrush->getBrushSize());
}

void Document::decBrushRadius() {
    if(!fBrush) return;
    fBrush->decPaintBrushSize(0.3);
    emit brushSizeChanged(fBrush->getBrushSize());
}

void Document::setOnionDisabled(const bool disabled) {
    fOnionVisible = !disabled;
    actionFinished();
}

void Document::setPaintMode(const PaintMode mode) {
    if(mode == fPaintMode) return;
    fPaintMode = mode;
    if(fBrush) {
        switch(fPaintMode) {
        case PaintMode::normal: fBrush->setNormalMode(); break;
        case PaintMode::erase: fBrush->startEraseMode(); break;
        case PaintMode::lockAlpha: fBrush->startAlphaLockMode(); break;
        case PaintMode::colorize: fBrush->startColorizeMode(); break;
        default: break;
        }
    }
    emit paintModeChanged(mode);
}

void Document::clear() {
    setPath("");
    const int nScenes = fScenes.count();
    for(int i = 0; i < nScenes; i++) removeScene(0);
    replaceClipboard(nullptr);
    const auto iBrushes = fBrushes;
    for (const auto brush : iBrushes) {
        removeBookmarkBrush(brush);
    }
    fBrushes.clear();
    const auto iColors = fColors;
    for (const auto& color : iColors) {
        removeBookmarkColor(color);
    }
    fColors.clear();
}

void Document::SWT_setupAbstraction(SWT_Abstraction * const abstraction,
                                    const UpdateFuncs &updateFuncs,
                                    const int visiblePartWidgetId) {
    for(const auto& scene : fScenes) {
        auto abs = scene->SWT_abstractionForWidget(updateFuncs,
                                                   visiblePartWidgetId);
        abstraction->addChildAbstraction(abs->ref<SWT_Abstraction>());
    }
}
