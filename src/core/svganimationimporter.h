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

#ifndef SVGANIMATIONIMPORTER_H
#define SVGANIMATIONIMPORTER_H

#include "smartPointers/selfref.h"

class BoundingBox;
class Canvas;

namespace ImportSVGAnimation {
    CORE_EXPORT qsptr<BoundingBox> loadSVGFile(const QString& filename,
                                               Canvas* scene);
}

#endif // SVGANIMATIONIMPORTER_H
