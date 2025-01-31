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

#include "heightbinding.h"
#include "../Properties/property.h"

HeightBinding::HeightBinding(const Property* const context) :
    PropertyBindingBase(context) {}

qsptr<HeightBinding> HeightBinding::sCreate(const Property* const context) {
    const auto result = new HeightBinding(context);
    return qsptr<HeightBinding>(result);
}
    
QJSValue HeightBinding::getJSValue(QJSEngine& e) {
    Q_UNUSED(e);
    if(mContext) {
        double height = mContext->prp_getSceneHeight();
        return QJSValue(height);
    }
    return QJSValue::NullValue;
}

QJSValue HeightBinding::getJSValue(QJSEngine& e, const qreal height) {
    Q_UNUSED(height);
    return getJSValue(e);
}

FrameRange HeightBinding::identicalRelRange(const int absFrame) {
    Q_UNUSED(absFrame);
    return FrameRange::EMINMAX;
}

FrameRange HeightBinding::nextNonUnaryIdenticalRelRange(const int absFrame) {
    Q_UNUSED(absFrame);
    return FrameRange::EMINMAX;
}
