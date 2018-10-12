// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

#include "range_ops.h"
#include "../defs.h"
#include <algorithm>

TypeRange RangeOps::sin(const TypeRange& input)
{
    //todo: calc
    return TypeRange(-1, 1);
}
TypeRange RangeOps::cos(const TypeRange& input)
{
    //todo: calc
    return TypeRange(-1, 1);
}

TypeRange RangeOps::min(const TypeRange& arg1, const TypeRange& arg2)
{
    return TypeRange(std::min(arg1.left, arg2.left), std::min(arg1.right, arg2.right));
}

TypeRange RangeOps::max(const TypeRange& arg1, const TypeRange& arg2)
{
    return TypeRange(std::max(arg1.left, arg2.left), std::max(arg1.right, arg2.right));
}

TypeRange RangeOps::clamp(const TypeRange& arg1, const TypeRange& arg2, const TypeRange& arg3)
{
    //glsl spec says: The returned value is computed as min(max(x, minVal), maxVal).
    return min(max(arg1, arg2), arg3);
}

//todo: we can branch here for <1 and >1
TypeRange RangeOps::power(const TypeRange& arg1, const TypeRange& arg2)
{
    //assume that arg1 > 0
    if (arg1.isSingle()) {
        if (arg1.left == 1.0) {
            return TypeRange(1);
        }
    }
    if (!isfinite(arg1.left) || !isfinite(arg2.left) || !isfinite(arg1.right) || !isfinite(arg2.right)) return TypeRange(0, INFINITY, EXCLUDE_BOTH);
    if (arg1.left < 0 || arg2.left < 0) return TypeRange(0, INFINITY, EXCLUDE_BOTH);

    double a1 = pow(arg1.left, arg2.left);
    double a2 = pow(arg1.left, arg2.right);
    double a3 = pow(arg1.right, arg2.left);
    double a4 = pow(arg1.right, arg2.right);

    return TypeRange(std::min(std::min(a1, a2), std::min(a3, a4)), std::max(std::max(a1, a2), std::max(a3, a4)));
}

TypeRange RangeOps::mix(const TypeRange& arg1, const TypeRange& arg2, const TypeRange& mix)
{
    double left1 = arg1.left, left2 = arg2.left;
    double right1 = arg1.right, right2 = arg2.right;

    if (mix.left >= 0 && mix.left <= 1) {
        left1 = arg1.left * (1 - mix.left) + arg2.left * mix.left;
        left2 = arg1.right * (1 - mix.left) + arg2.right * mix.left;
    }
    if (mix.right >= 0 && mix.right <= 1) {
        right1 = arg1.left * (1 - mix.right) + arg2.left * mix.right;
        right2 = arg1.right * (1 - mix.right) + arg2.right * mix.right;
    }
    double realLeft = std::min(left1, left2);
    double realRight = std::max(right1, right2);
    return TypeRange(std::min(realLeft, realRight), std::max(realLeft, realRight));
}

TypeRange RangeOps::dot(const TypeRange& arg1, const TypeRange& arg2, const TypeRange& arg3, const TypeRange& arg4,
                        const TypeRange& arg5, const TypeRange& arg6, const TypeRange& arg7, const TypeRange& arg8)
{
    //very straight-forward
    return arg1*arg5 + arg2*arg6 * arg3*arg7 + arg4*arg8;
}

TypeRange RangeOps::floor(const TypeRange& arg1)
{
    //well, actually it shall be int range, i.e. 1,2,3,4, 1.5 is not an option
    return TypeRange(::floor(arg1.left), ::floor(arg1.right), INCLUDE_BOTH);
}

TypeRange RangeOps::fract(const TypeRange& arg1)
{
    //todo: could use more smart analysis, like for [1.5, 1.8] return [0.5, 0.8].
    return TypeRange(0, 1, INCLUDE_LEFT);
}

TypeRange pow2(const TypeRange& arg1) {
    TypeRange abs = RangeOps::abs(arg1);
    return TypeRange(abs.left*abs.left, abs.right*abs.right);
}

TypeRange RangeOps::length(const TypeRange& arg1, const TypeRange& arg2, const TypeRange& arg3, const TypeRange& arg4)
{
    TypeRange sqLength(0, 0);
    sqLength = pow2(arg1) + pow2(arg2) + pow2(arg3) + pow2(arg4);
    return TypeRange(::sqrt(sqLength.left), ::sqrt(sqLength.right));
}

TypeRange RangeOps::abs(const TypeRange& arg1)
{
    double leftAbs = ::fabs(arg1.left);
    double rightAbs = ::fabs(arg1.right);
    double maxAbs = std::max(leftAbs, rightAbs);
    double minAbs;
    if (arg1.includes(0)) {
        minAbs = 0;
    }
    else {
        minAbs = std::min(leftAbs, rightAbs);
    }
    return TypeRange(minAbs, maxAbs);
}


TypeRange RangeOps::getLeftIncluding(const TypeRange& input, double edge)
{
    if (edge >= input.right) return input;
    if (edge < input.left) return InvalidRange;
    return TypeRange(input.left, edge, (RangeFlag)((input.flag & INCLUDE_LEFT) | INCLUDE_RIGHT));
}

TypeRange RangeOps::getRightIncluding(const TypeRange& input, double edge)
{
    if (edge <= input.left) return input;
    if (edge > input.right) return InvalidRange;

    return TypeRange(edge, input.right, (RangeFlag)((input.flag & INCLUDE_RIGHT) | INCLUDE_LEFT));

}

TypeRange RangeOps::getLeftExcluding(const TypeRange& input, double edge)
{
    if (edge <= input.left) return InvalidRange;
    if (edge > input.right) return input;

    return TypeRange(input.left, edge, (RangeFlag)(input.flag & INCLUDE_LEFT));
}

TypeRange RangeOps::getRightExcluding(const TypeRange& input, double edge)
{
    if (edge >= input.right) return InvalidRange;
    if (edge < input.left) return input;

    return TypeRange(edge, input.right, (RangeFlag)(input.flag & INCLUDE_RIGHT));

}