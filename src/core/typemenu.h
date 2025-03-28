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

#ifndef TYPEMENU_H
#define TYPEMENU_H

#include <QMenu>
#include <typeindex>
#include "smartPointers/ememory.h"
#include "canvasbase.h"

class Property;
class MovablePoint;
class BoundingBox;

template <typename Type>
class CORE_EXPORT TypeMenu {
    using TTypeMenu = TypeMenu<Type>;
public:
    using PlainTriggeredOp = std::function<void()>;
    using CheckTriggeredOp = std::function<void(bool)>;
    template <class T> using PlainSelectedOp = std::function<void(T*)>;
    template <class T> using CheckSelectedOp = std::function<void(T*, bool)>;
    template <class T> using AllOp = std::function<void(const QList<T*>&)>;

    TypeMenu(QMenu * const targetMenu,
             CanvasBase * const targetCanvas,
             QWidget * const parent) :
        mQMenu(targetMenu),
        mTargetCanvas(targetCanvas),
        mParentWidget(parent) {}

    QAction* addSection(const QString& name) {
        return mQMenu->addSection(name);
    }

    QAction* addCheckableAction(const QString& text, const bool checked,
                                const CheckTriggeredOp& op) {
        QAction * const qAction = mQMenu->addAction(text);
        qAction->setCheckable(true);
        qAction->setChecked(checked);
        QObject::connect(qAction, &QAction::triggered, op);
        return qAction;
    }

    template <class T>
    QAction* addCheckableAction(const QString& text, const bool checked,
                                const CheckSelectedOp<T>& op) {
        QAction * const qAction = mQMenu->addAction(text);
        qAction->setCheckable(true);
        qAction->setChecked(checked);
        const PlainSelectedOp<T> plainOp = [op, checked](T* pt) {
            op(pt, !checked);
        };
        connectAction(qAction, plainOp);
        return qAction;
    }

    QAction* addPlainAction(const QIcon &icon,
                            const QString& text,
                            const PlainTriggeredOp& op)
    {
        QAction * const qAction = mQMenu->addAction(icon, text);
        QObject::connect(qAction, &QAction::triggered, op);
        return qAction;
    }

    template <class T>
    QAction* addPlainAction(const QIcon &icon,
                            const QString& text,
                            const PlainSelectedOp<T>& op) {
        QAction * const qAction = mQMenu->addAction(icon, text);
        connectAction(qAction, op);
        return qAction;
    }

    template <class T>
    QAction* addPlainAction(const QIcon &icon, const QString& text, const AllOp<T>& op) {
        QAction * const qAction = mQMenu->addAction(icon, text);
        connectAction(qAction, op);
        return qAction;
    }

    TTypeMenu * addMenu(const QIcon &icon, const QString& title) {
        QMenu * const qMenu = mQMenu->addMenu(icon, title);
        const auto child = std::make_shared<TTypeMenu>(qMenu, mTargetCanvas,
                                                       mParentWidget);
        mChildMenus.append(child);
        return child.get();
    }

    TTypeMenu * addMenu(const QStringList& titles) {
        TTypeMenu* menu = this;
        for(const auto& title : titles) {
            const auto menuT = menu->childMenu(title);
            if(menuT) menu = menuT;
            else menu = menu->addMenu(QIcon::fromTheme("preferences"), title);
        }
        return menu;
    }

    TTypeMenu* childMenu(const QString& path) {
        for(const auto& child : mChildMenus) {
            if(child->mQMenu->title() == path)
                return child.get();
        }
        return nullptr;
    }

    TTypeMenu* childMenu(const QStringList& path) {
        TTypeMenu* menu = this;
        for(const auto& subPath : path) {
            menu = menu->childMenu(subPath);
            if(!menu) return nullptr;
        }
        return menu;
    }

    QAction* addSeparator() {
        return mQMenu->addSeparator();
    }

    void setEnabled(const bool enabled) {
        mQMenu->setEnabled(enabled);
    }

    void setVisible(const bool visible) {
        mQMenu->setVisible(visible);
    }

    bool isEmpty() {
        return mQMenu->isEmpty();
    }

    void clear() {
        mQMenu->clear();
        mChildMenus.clear();
        mTypeIndex.clear();
    }

    QWidget* getParentWidget() const {
        return mParentWidget;
    }

    void addSharedMenu(const QString& name) {
        mSharedMenus << name;
    }

    bool hasSharedMenu(const QString& name) const {
        return mSharedMenus.contains(name);
    }

    template <typename T>
    void addedActionsForType() {
        mTypeIndex.append(std::type_index(typeid(T)));
    }

    template <typename T>
    bool hasActionsForType() const {
        return mTypeIndex.contains(std::type_index(typeid(T)));
    }
private:
    template <typename U>
    void connectAction(BoundingBox * const, QAction * const qAction,
                       const U& op) {
        const auto targetCanvas = mTargetCanvas;
        const auto canvasOp = [op, targetCanvas]() {
            try {
                targetCanvas->execOpOnSelectedBoxes(op);
            } catch(const std::exception& e) {
                gPrintExceptionCritical(e);
            }
        };
        QObject::connect(qAction, &QAction::triggered, canvasOp);
    }

    template <typename U>
    void connectAction(MovablePoint * const, QAction * const qAction,
                       const U& op) {
        const auto targetCanvas = mTargetCanvas;
        const auto canvasOp = [op, targetCanvas]() {
            try {
                targetCanvas->execOpOnSelectedPoints(op);
            } catch(const std::exception& e) {
                gPrintExceptionCritical(e);
            }
        };
        QObject::connect(qAction, &QAction::triggered, canvasOp);
    }

    template <typename U>
    void connectAction(Property * const, QAction * const qAction,
                       const U& op) {
        const auto targetCanvas = mTargetCanvas;
        const auto canvasOp = [op, targetCanvas]() {
            try {
                targetCanvas->execOpOnSelectedProperties(op);
            } catch(const std::exception& e) {
                gPrintExceptionCritical(e);
            }
        };
        QObject::connect(qAction, &QAction::triggered, canvasOp);
    }

    template <class T>
    void connectAction(QAction * const qAction, const PlainSelectedOp<T>& op) {
        connectAction(static_cast<T*>(nullptr), qAction, op);
    }

    template <class T>
    void connectAction(QAction * const qAction, const AllOp<T>& op) {
        connectAction(static_cast<T*>(nullptr), qAction, op);
    }

    QMenu * const mQMenu;
    CanvasBase * const mTargetCanvas;
    QWidget * const mParentWidget;

    QList<stdsptr<TTypeMenu>> mChildMenus;
    QList<std::type_index> mTypeIndex;
    QStringList mSharedMenus;
};

typedef TypeMenu<MovablePoint> PointTypeMenu;
typedef TypeMenu<Property> PropertyMenu;

#endif // TYPEMENU_H
