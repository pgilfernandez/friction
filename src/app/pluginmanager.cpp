/*
#
# Friction - https://friction.graphics
#
# Copyright (c) Ole-André Rodlie and contributors
#
*/

#include "pluginmanager.h"

#include "GUI/mainwindow.h"
#include "Private/document.h"
#include "appsupport.h"

#include <QAction>
#include <QCoreApplication>
#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QIcon>
#include <QJsonObject>
#include <QLibrary>
#include <QMessageBox>
#include <QPluginLoader>
#include <QSet>
#include <QWidget>

#include <exception>

PluginManager::PluginManager(MainWindow *window, Document *document)
    : QObject(window)
    , mWindow(window)
    , mDocument(document)
{
    QObject::connect(mDocument, &Document::activeSceneSet,
                     this, [this](Canvas *scene) { updateActionState(scene); });
}

PluginManager::~PluginManager()
{
    for (auto &loaded : mPlugins) {
        for (QAction *action : loaded.actions) { delete action; }
        loaded.actions.clear();
        try { loaded.plugin->shutdown(); }
        catch (...) {
            qWarning("Plugin '%s' failed while shutting down",
                     qUtf8Printable(loaded.plugin->id()));
        }
    }
}

QString PluginManager::userPluginsPath()
{
    const QString path = AppSupport::getAppConfigPath() + QStringLiteral("/Plugins");
    QDir().mkpath(path);
    return path;
}

QStringList PluginManager::pluginsPaths()
{
    const QString appDir = QCoreApplication::applicationDirPath();
    QStringList paths{
        userPluginsPath(),
        appDir + QStringLiteral("/plugins"),
        appDir + QStringLiteral("/../PlugIns"),
        appDir + QStringLiteral("/../lib/friction/plugins"),
        appDir + QStringLiteral("/../share/friction/plugins"),
        appDir + QStringLiteral("/../../../plugins")
    };

    QSet<QString> seen;
    QStringList result;
    for (const QString &path : paths) {
        const QString canonical = QFileInfo(path).canonicalFilePath();
        if (canonical.isEmpty() || seen.contains(canonical)) { continue; }
        seen.insert(canonical);
        result.append(canonical);
    }
    return result;
}

void PluginManager::loadPlugins()
{
    QSet<QString> candidates;
    for (const QString &directory : pluginsPaths()) {
        QDirIterator iterator(directory, QDir::Files,
                              QDirIterator::Subdirectories);
        while (iterator.hasNext()) {
            const QString path = iterator.next();
            if (QLibrary::isLibrary(path)) { candidates.insert(path); }
        }
    }

    QStringList sorted = candidates.values();
    sorted.sort();
    for (const QString &path : sorted) { loadPlugin(path); }
    updateActionState(activeScene());
}

void PluginManager::loadPlugin(const QString &path)
{
    auto loader = std::unique_ptr<QPluginLoader>(new QPluginLoader(path));
    if (loader->metaData().value(QStringLiteral("IID")).toString() !=
            QStringLiteral(FRICTION_PLUGIN_IID)) {
        return;
    }
    // Plugin-owned widgets can outlive the manager while the main window is
    // being destroyed. Keep native code mapped until process exit so their
    // destructors and queued slots remain valid.
    loader->setLoadHints(QLibrary::PreventUnloadHint);
    QObject *instance = loader->instance();
    if (!instance) {
        addError(tr("Could not load plugin %1: %2")
                 .arg(path, loader->errorString()));
        return;
    }

    auto plugin = qobject_cast<Friction::Plugins::Plugin *>(instance);
    if (!plugin) {
        loader->unload();
        return;
    }
    if (plugin->apiVersion() != Friction::Plugins::ApiVersion) {
        addError(tr("Plugin %1 uses API %2; Friction supports API %3")
                 .arg(plugin->name())
                 .arg(plugin->apiVersion())
                 .arg(Friction::Plugins::ApiVersion));
        loader->unload();
        return;
    }
    if (plugin->id().trimmed().isEmpty()) {
        addError(tr("Ignoring plugin from %1 because it has no id").arg(path));
        loader->unload();
        return;
    }
    for (const auto &loaded : mPlugins) {
        if (loaded.plugin->id() == plugin->id()) {
            addError(tr("Ignoring duplicate plugin id '%1' from %2")
                     .arg(plugin->id(), path));
            loader->unload();
            return;
        }
    }

    QString error;
    if (!plugin->initialize(this, &error)) {
        addError(tr("Could not initialize plugin %1: %2")
                 .arg(plugin->name(), error));
        loader->unload();
        return;
    }

    LoadedPlugin loaded;
    loaded.loader = std::move(loader);
    loaded.plugin = plugin;
    QSet<QString> actionIds;
    for (const Friction::Plugins::Action &description : plugin->actions()) {
        if (description.id.isEmpty() || description.text.isEmpty() ||
                actionIds.contains(description.id)) {
            addError(tr("Plugin %1 declared an invalid or duplicate action")
                     .arg(plugin->name()));
            continue;
        }
        actionIds.insert(description.id);
        QAction *action = new QAction(QIcon::fromTheme(description.iconName),
                                      description.text, mWindow);
        action->setObjectName(plugin->id() + QLatin1Char('.') + description.id);
        action->setToolTip(description.toolTip);
        action->setData(description.toolTip);
        action->setShortcut(description.shortcut);
        action->setProperty("frictionRequiresActiveScene",
                            description.requiresActiveScene);
        QObject::connect(action, &QAction::triggered, this,
                         [this, plugin, description]() {
            try {
                plugin->triggerAction(description.id);
            } catch (const std::exception &exception) {
                const QString message = tr("Plugin %1 failed: %2")
                        .arg(plugin->name(), QString::fromUtf8(exception.what()));
                addError(message);
                QMessageBox::critical(mWindow, tr("Plugin Error"), message);
            } catch (...) {
                const QString message = tr(
                            "Plugin %1 failed with an unknown error")
                        .arg(plugin->name());
                addError(message);
                QMessageBox::critical(mWindow, tr("Plugin Error"), message);
            }
        });
        mWindow->addPluginAction(action, description.menu,
                                 description.showInToolBar);
        loaded.actions.append(action);
    }

    qInfo("Loaded Friction plugin: %s %s (%s)",
          qUtf8Printable(plugin->name()), qUtf8Printable(plugin->version()),
          qUtf8Printable(path));
    mPlugins.emplace_back(std::move(loaded));
}

void PluginManager::updateActionState(Canvas *scene)
{
    for (const auto &loaded : mPlugins) {
        for (QAction *action : loaded.actions) {
            const bool needsScene = action->property(
                        "frictionRequiresActiveScene").toBool();
            action->setEnabled(!needsScene || scene);
        }
    }
}

void PluginManager::addError(const QString &message)
{
    mErrors.append(message);
    qWarning("%s", qUtf8Printable(message));
}

QStringList PluginManager::loadedPluginIds() const
{
    QStringList result;
    for (const auto &loaded : mPlugins) { result.append(loaded.plugin->id()); }
    return result;
}

QStringList PluginManager::errors() const
{
    return mErrors;
}

QWidget *PluginManager::mainWindow() const
{
    return mWindow;
}

Document *PluginManager::document() const
{
    return mDocument;
}

Canvas *PluginManager::activeScene() const
{
    return mDocument && mDocument->fActiveScene ? *mDocument->fActiveScene : nullptr;
}

void PluginManager::actionFinished()
{
    if (mDocument) { mDocument->actionFinished(); }
}
