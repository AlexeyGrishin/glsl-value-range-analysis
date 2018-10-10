// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

#include "range.h"
#include <algorithm>

bool TypeRange::includes(double n) const
{
    if (flag == IGNORE) return false;
    if ((flag & INCLUDE_LEFT) == 0 && n == left) return false;
    if ((flag & INCLUDE_RIGHT) == 0 && n == right) return false;
    return n >= left && n <= right;
}

bool TypeRange::includes(const TypeRange& anotherRange) const
{
    if (flag == IGNORE || anotherRange.flag == IGNORE) return false;
    if (!includes(anotherRange.left)) return false;
    if (!includes(anotherRange.right)) return false;
    if (left == anotherRange.left && !(flag & INCLUDE_LEFT) && (anotherRange.flag & INCLUDE_LEFT)) return false;
    if (right == anotherRange.right && !(flag & INCLUDE_RIGHT) && (anotherRange.flag & INCLUDE_RIGHT)) return false;
    return true;
}


bool TypeRange::isValid() const
{
    if (flag == IGNORE) return true;
    if (left > right) return false;
    if (left == right) {
        if (flag != INCLUDE_BOTH) return false;
    }
    return true;
}

RangeFlag merge(RangeFlag flag1, RangeFlag flag2)
{
    if (flag1 == IGNORE || flag2 == IGNORE) return IGNORE;
    int out = flag1;
    if ((out & INCLUDE_LEFT) == INCLUDE_LEFT && (flag2 & INCLUDE_LEFT) == 0) {
        out &= ~INCLUDE_LEFT;
    }
    if ((out & INCLUDE_RIGHT) == INCLUDE_RIGHT && (flag2 & INCLUDE_RIGHT) == 0) {
        out &= ~INCLUDE_RIGHT;
    }
    return (RangeFlag)out;
}

RangeFlag reverse(RangeFlag flag)
{
    if (flag == INCLUDE_LEFT) return INCLUDE_RIGHT;
    if (flag == INCLUDE_RIGHT) return INCLUDE_LEFT;
    return flag;
}

TypeRange TypeRange::operator+(const TypeRange& another) const
{
    return TypeRange(left + another.left, right + another.right, merge(flag, another.flag));
}

TypeRange TypeRange::operator-(const TypeRange& another) const
{
    return TypeRange(left - another.left, right - another.right, merge(flag, another.flag));
}

TypeRange TypeRange::operator-() const
{
    return TypeRange(-right, -left, reverse(flag));
}

TypeRange TypeRange::operator*(const TypeRange& another) const
{
    double ll = left*another.left;
    double lr = left*another.right;
    double rl = right*another.left;
    double rr = right*another.right;

    return TypeRange(std::min(std::min(ll, lr), std::min(rl,rr)), std::max(std::max(ll, lr), std::max(rl, rr)), merge(flag, another.flag));
}

bool fequal(double left, double right) {
    if (!isfinite(left) || !isfinite(right)) return left == right;
    return fabs(left - right) < Eps;
}

bool TypeRange::operator==(const TypeRange& another) const
{
    return fequal(left, another.left) && fequal(right, another.right) && flag == another.flag;
}

bool TypeRange::operator!=(const TypeRange& another) const
{
    return !fequal(left, another.left) || !fequal(right, another.right) || flag != another.flag;
}

TypeRange::operator bool() const
{
    return (isSingle() && fequal(left, 0)) ? false : true;
}

bool TypeRange::isSingle() const 
{
    return fequal(left, right);
}

TypeRange& TypeRange::operator=(const TypeRange& another)
{
    left = another.left;
    right = another.right;
    flag = another.flag;
    return *this;
}


TypeRange TypeRange::operator/(const TypeRange& another) const
{
    //if another includes zero - here could be infinite range. so better will be branch for zero case. but not here, in op fn
    if (another.isSingle()) {
        return TypeRange(left / another.left, right / another.left, merge(flag, another.flag));
    }
    return TypeRange();
}
