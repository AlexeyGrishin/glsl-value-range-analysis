#include "range.h"

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
    return TypeRange(-left, -right, flag);
}

TypeRange TypeRange::operator*(const TypeRange& another) const
{
    double ll = left*another.left;
    double lr = left*another.right;
    double rl = right*another.left;
    double rr = right*another.right;

    return TypeRange(MIN(MIN(ll, lr), MIN(rl,rr)), MAX(MAX(ll, lr), MAX(rl, rr)), merge(flag, another.flag));
}

bool TypeRange::operator==(const TypeRange& another) const
{
    return left == another.left && right == another.right && flag == another.flag;
}

bool TypeRange::operator!=(const TypeRange& another) const
{
    return left != another.left || right != another.right || flag != another.flag;
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
