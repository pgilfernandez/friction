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
*/

#ifndef FRICTIONPLUGIN_H
#define FRICTIONPLUGIN_H

#include <QKeySequence>
#include <QList>
#include <QString>
#include <QtPlugin>

class Canvas;
class Document;
class QWidget;

namespace Friction {
namespace Plugins {

constexpr int ApiVersion = 1;

enum class MenuLocation {
    Import,
    Export,
    Object,
    Effects,
    Scene,
    Help
};

struct Action {
    QString id;
    QString text;
    QString toolTip;
    QString iconName;
    QKeySequence shortcut;
    MenuLocation menu = MenuLocation::Import;
    bool requiresActiveScene = true;
    bool showInToolBar = false;
};

class Host {
public:
    virtual ~Host() = default;

    virtual QWidget *mainWindow() const = 0;
    virtual Document *document() const = 0;
    virtual Canvas *activeScene() const = 0;
    virtual void actionFinished() = 0;
};

class Plugin {
public:
    virtual ~Plugin() = default;

    virtual int apiVersion() const = 0;
    virtual QString id() const = 0;
    virtual QString name() const = 0;
    virtual QString version() const = 0;
    virtual QList<Action> actions() const = 0;

    virtual bool initialize(Host *host, QString *errorMessage) = 0;
    virtual void triggerAction(const QString &actionId) = 0;
    virtual void shutdown() = 0;
};

} // namespace Plugins
} // namespace Friction

#define FRICTION_PLUGIN_IID "graphics.friction.Plugin/1.0"
Q_DECLARE_INTERFACE(Friction::Plugins::Plugin, FRICTION_PLUGIN_IID)

#endif // FRICTIONPLUGIN_H
