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

#ifndef INTERNALLINKCANVAS_H
#define INTERNALLINKCANVAS_H
#include "internallinkgroupbox.h"
#include "Properties/boolproperty.h"
#include "Boxes/frameremapping.h"

class ComboBoxProperty;

class CORE_EXPORT InternalLinkCanvas : public InternalLinkGroupBox {
    e_OBJECT
protected:
    InternalLinkCanvas(ContainerBox * const linkTarget,
                       const bool innerLink);
public:
    void setupRenderData(const qreal relFrame, const QMatrix& parentM,
                         BoxRenderData * const data,
                         Canvas * const scene) override;

    qsptr<BoundingBox> createLink(const bool inner) override;

    stdsptr<BoxRenderData> createRenderData() override;

    bool relPointInsidePath(const QPointF &relPos) const override;
    void anim_setAbsFrame(const int frame) override;

    void prp_setupTreeViewMenu(PropertyMenu * const menu) override;

    void enableFrameRemappingAction();
    void disableFrameRemappingAction();

    bool clipToCanvas();
    bool isFrameInDurationRect(const int relFrame) const override;
    bool isFrameFInDurationRect(const qreal relFrame) const override;
private:
    void updateFrameRemappingVisibility();
    void updateDurationRangeForRemap();
    qsptr<BoolProperty> mClipToCanvas =
            enve::make_shared<BoolProperty>("clip");
    qsptr<ComboBoxProperty> mFrameRemappingMode;
    qsptr<QrealFrameRemapping> mFrameRemapping =
            enve::make_shared<QrealFrameRemapping>();
};

#endif // INTERNALLINKCANVAS_H
