#pragma once
#include "range.h"

namespace RangeOps {
    TypeRange sin(const TypeRange& input);
    TypeRange cos(const TypeRange& input);

    TypeRange getLeftIncluding(const TypeRange& input, double edge);
    TypeRange getRightIncluding(const TypeRange& input, double edge);
    TypeRange getLeftExcluding(const TypeRange& input, double edge);
    TypeRange getRightExcluding(const TypeRange& input, double edge);
}