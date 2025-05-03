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

#include "aligntoolbar.h"
#include "GUI/global.h"
#include "Private/document.h"
#include "appsupport.h"

#include <QStandardItemModel>
#include <QPushButton>

#define INDEX_ALIGN_GEOMETRY 0
#define INDEX_ALIGN_GEOMETRY_PIVOT 1
#define INDEX_ALIGN_PIVOT 2

#define INDEX_REL_SCENE 0
#define INDEX_REL_LAST_SELECTED 1
#define INDEX_REL_LAST_SELECTED_PIVOT 2
#define INDEX_REL_BOUNDINGBOX 3

using namespace Friction::Ui;

AlignToolBar::AlignToolBar(QWidget *parent)
    : ToolBar("AlignToolBar", parent, true)
    , mAlignPivot(nullptr)
    , mRelativeTo(nullptr)
    , mAlignShowAct(nullptr)
    , mAlignPivotAct(nullptr)
    , mRelativeToAct(nullptr)
    , mAlignLeftAct(nullptr)
    , mAlignHCenterAct(nullptr)
    , mAlignRightAct(nullptr)
    , mAlignTopAct(nullptr)
    , mAlignVCenterAct(nullptr)
    , mAlignBottomAct(nullptr)
{
    setupWidgets();
}

void AlignToolBar::setCurrentCanvas(Canvas * const target)
{
    mCanvas.assign(target);
    setEnabled(target ? true : false);
}

void AlignToolBar::setupWidgets()
{
    setToolButtonStyle(Qt::ToolButtonIconOnly);
    setContextMenuPolicy(Qt::NoContextMenu);
    setAllowedAreas(Qt::TopToolBarArea | Qt::BottomToolBarArea);

    setWindowTitle(tr("Align Toolbar"));

    mAlignShowAct = addAction(QIcon::fromTheme("alignCenter"), tr("Align"));
    mAlignShowAct->setCheckable(true);
    mAlignShowAct->setChecked(AppSupport::getSettings("ui",
                                                      "AlignToolBarShowChecked",
                                                      false).toBool());

    mSeparators << addSeparator();

    mAlignPivot = new QComboBox(this);
    mAlignPivot->setMinimumWidth(20);
    mAlignPivot->setFocusPolicy(Qt::NoFocus);
    mAlignPivot->addItem(tr("Geometry")); // INDEX_ALIGN_GEOMETRY
    mAlignPivot->addItem(tr("Geometry by Pivot")); // INDEX_ALIGN_GEOMETRY_PIVOT
    mAlignPivot->addItem(tr("Pivot")); // INDEX_ALIGN_PIVOT
    mAlignPivotAct = addWidget(mAlignPivot);

    mSeparators << addSeparator();

    mRelativeTo = new QComboBox(this);
    mRelativeTo->setMinimumWidth(20);
    mRelativeTo->setFocusPolicy(Qt::NoFocus);
    mRelativeTo->addItem(tr("Scene")); // INDEX_REL_SCENE
    mRelativeTo->addItem(tr("Last Selected")); // INDEX_REL_LAST_SELECTED
    mRelativeTo->addItem(tr("Last Selected Pivot")); // INDEX_REL_LAST_SELECTED_PIVOT
    mRelativeTo->addItem(tr("Bounding Box")); // INDEX_REL_BOUNDINGBOX
    mRelativeToAct = addWidget(mRelativeTo);

    setComboBoxItemState(mRelativeTo, INDEX_REL_LAST_SELECTED_PIVOT, false);
    setComboBoxItemState(mRelativeTo, INDEX_REL_BOUNDINGBOX, false);

    mSeparators << addSeparator();

    const auto leftButton = new QPushButton(QIcon::fromTheme("pivot-align-left"),
                                            tr(""), this);
    leftButton->setFocusPolicy(Qt::NoFocus);
    leftButton->setToolTip(tr("Align Left"));
    connect(leftButton, &QPushButton::clicked,
            this, [this]() { triggerAlign(Qt::AlignLeft); });
    mAlignLeftAct = addWidget(leftButton);

    mSeparators << addSeparator();

    const auto hCenterButton = new QPushButton(QIcon::fromTheme("pivot-align-hcenter"),
                                               tr(""), this);
    hCenterButton->setFocusPolicy(Qt::NoFocus);
    hCenterButton->setToolTip(tr("Align Horizontal Center"));
    connect(hCenterButton, &QPushButton::clicked,
            this, [this]() { triggerAlign(Qt::AlignHCenter); });
    mAlignHCenterAct = addWidget(hCenterButton);

    mSeparators << addSeparator();

    const auto rightButton = new QPushButton(QIcon::fromTheme("pivot-align-right"),
                                             tr(""), this);
    rightButton->setFocusPolicy(Qt::NoFocus);
    rightButton->setToolTip(tr("Align Right"));
    connect(rightButton, &QPushButton::clicked,
            this, [this]() { triggerAlign(Qt::AlignRight); });
    mAlignRightAct = addWidget(rightButton);

    mSeparators << addSeparator();

    const auto topButton = new QPushButton(QIcon::fromTheme("pivot-align-top"),
                                           tr(""), this);
    topButton->setFocusPolicy(Qt::NoFocus);
    topButton->setToolTip(tr("Align Top"));
    connect(topButton, &QPushButton::clicked,
            this, [this]() { triggerAlign(Qt::AlignTop); });
    mAlignTopAct = addWidget(topButton);

    mSeparators << addSeparator();

    const auto vCenterButton = new QPushButton(QIcon::fromTheme("pivot-align-vcenter"),
                                               tr(""), this);
    vCenterButton->setFocusPolicy(Qt::NoFocus);
    vCenterButton->setToolTip(tr("Align Vertical Center"));
    connect(vCenterButton, &QPushButton::clicked,
            this, [this]() { triggerAlign(Qt::AlignVCenter); });
    mAlignVCenterAct = addWidget(vCenterButton);

    mSeparators << addSeparator();

    const auto bottomButton = new QPushButton(QIcon::fromTheme("pivot-align-bottom"),
                                              tr(""), this);
    bottomButton->setFocusPolicy(Qt::NoFocus);
    bottomButton->setToolTip(tr("Align Bottom"));
    connect(bottomButton, &QPushButton::clicked,
            this, [this]() { triggerAlign(Qt::AlignBottom); });
    mAlignBottomAct = addWidget(bottomButton);

    connect(mAlignPivot,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int index) {
        setComboBoxItemState(mRelativeTo,
                             INDEX_REL_LAST_SELECTED_PIVOT,
                             index == INDEX_ALIGN_PIVOT);
        setComboBoxItemState(mRelativeTo,
                             INDEX_REL_BOUNDINGBOX,
                             index == INDEX_ALIGN_PIVOT);
        if (index == INDEX_ALIGN_PIVOT) { mRelativeTo->setCurrentIndex(INDEX_REL_BOUNDINGBOX); }
        else { mRelativeTo->setCurrentIndex(INDEX_REL_SCENE); }
    });

    connect(mRelativeTo,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int index) {
        mAlignLeftAct->setEnabled(index != INDEX_REL_LAST_SELECTED_PIVOT);
        mAlignRightAct->setEnabled(index != INDEX_REL_LAST_SELECTED_PIVOT);
        mAlignTopAct->setEnabled(index != INDEX_REL_LAST_SELECTED_PIVOT);
        mAlignBottomAct->setEnabled(index != INDEX_REL_LAST_SELECTED_PIVOT);
    });

    triggerShow(mAlignShowAct->isChecked());
    connect(mAlignShowAct, &QAction::triggered,
            this, [this](const bool &triggered) {
        triggerShow(triggered);
        AppSupport::setSettings("ui",
                                "AlignToolBarShowChecked",
                                triggered);
    });

    setEnabled(false);
}

void AlignToolBar::triggerShow(bool triggered)
{
    mAlignPivotAct->setVisible(triggered);
    mRelativeToAct->setVisible(triggered);
    mAlignLeftAct->setVisible(triggered);
    mAlignHCenterAct->setVisible(triggered);
    mAlignRightAct->setVisible(triggered);
    mAlignTopAct->setVisible(triggered);
    mAlignVCenterAct->setVisible(triggered);
    mAlignBottomAct->setVisible(triggered);

    for (const auto &sep : mSeparators) {
        sep->setVisible(triggered);
    }
}

void AlignToolBar::triggerAlign(const Qt::Alignment &align)
{
    const auto alignPivot = static_cast<AlignPivot>(mAlignPivot->currentIndex());
    const auto relativeTo = static_cast<AlignRelativeTo>(mRelativeTo->currentIndex());

    if (!mCanvas) { return; }
    mCanvas->alignSelectedBoxes(align, alignPivot, relativeTo);
    mCanvas->finishedAction();
}

void AlignToolBar::setComboBoxItemState(QComboBox *box,
                                        int index,
                                        bool enabled)
{
    auto model = qobject_cast<QStandardItemModel*>(box->model());
    if (!model) { return; }

    if (index >= box->count() || index < 0) { return; }

    auto item = model->item(index);
    if (!item) { return; }

    item->setEnabled(enabled);
}
