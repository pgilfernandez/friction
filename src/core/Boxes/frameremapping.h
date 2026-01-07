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

#ifndef FRAMEREMAPPING_H
#define FRAMEREMAPPING_H
#include "Animators/qrealanimator.h"

class CORE_EXPORT FrameRemappingBase : public QrealAnimator {
    Q_OBJECT
public:
    enum class FrameRemappingMode : int {
        manual = 0,
        loop,
        bounce
    };
    Q_ENUM(FrameRemappingMode)
protected:
    FrameRemappingBase();

    QDomElement prp_writePropertyXEV_impl(const XevExporter& exp) const override;
    void prp_readPropertyXEV_impl(const QDomElement& ele,
                                  const XevImporter& imp) override;
public:    
    bool enabled() const { return mEnabled; }
    FrameRemappingMode mode() const { return mMode; }

    void enableAction(const int minFrame, const int maxFrame,
                      const int animStartRelFrame);
    void disableAction();

    void setMode(FrameRemappingMode mode);

    void setFrameCount(const int count);

    void prp_readProperty_impl(eReadStream &src) override;
    void prp_writeProperty_impl(eWriteStream &dst) const override;
signals:
    void enabledChanged(const bool enabled);
    void modeChanged(FrameRemappingMode mode);
protected:
    qreal remappedFrame(const qreal relFrame) const;
    qreal loopFrame(const qreal relFrame) const;
    qreal bounceFrame(const qreal relFrame) const;
private:
    void setEnabled(const bool enabled);
    void updateVisibility();
    FrameRemappingMode mMode = FrameRemappingMode::manual;
    bool mEnabled = false;
};

class CORE_EXPORT IntFrameRemapping : public FrameRemappingBase {
    e_OBJECT
protected:
    IntFrameRemapping();
public:
    int frame(const qreal relFrame) const;
};

class CORE_EXPORT QrealFrameRemapping : public FrameRemappingBase {
    e_OBJECT
protected:
    QrealFrameRemapping();
public:
    qreal frame(const qreal relFrame) const;
};

#endif // FRAMEREMAPPING_H
