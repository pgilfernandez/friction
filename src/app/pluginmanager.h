/*
#
# Friction - https://friction.graphics
#
# Copyright (c) Ole-André Rodlie and contributors
#
*/

#ifndef PLUGINMANAGER_H
#define PLUGINMANAGER_H

#include "frictionplugin.h"

#include <QObject>
#include <QStringList>
#include <memory>
#include <vector>

class QAction;
class Document;
class MainWindow;
class QPluginLoader;

class PluginManager final : public QObject, public Friction::Plugins::Host
{
public:
    PluginManager(MainWindow *window, Document *document);
    ~PluginManager() override;

    void loadPlugins();
    QStringList loadedPluginIds() const;
    QStringList errors() const;

    QWidget *mainWindow() const override;
    Document *document() const override;
    Canvas *activeScene() const override;
    void actionFinished() override;

    static QString userPluginsPath();
    static QStringList pluginsPaths();

private:
    struct LoadedPlugin {
        std::unique_ptr<QPluginLoader> loader;
        Friction::Plugins::Plugin *plugin = nullptr;
        QList<QAction *> actions;
    };

    void loadPlugin(const QString &path);
    void updateActionState(Canvas *scene);
    void addError(const QString &message);

    MainWindow *mWindow;
    Document *mDocument;
    std::vector<LoadedPlugin> mPlugins;
    QStringList mErrors;
};

#endif // PLUGINMANAGER_H
