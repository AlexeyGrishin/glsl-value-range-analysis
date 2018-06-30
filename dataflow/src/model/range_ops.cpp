#include "range_ops.h"

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


TypeRange RangeOps::getLeftIncluding(const TypeRange& input, double edge)
{
    if (edge >= input.right) return input;
    if (edge < input.left) return TypeRange(1,-1);
    return TypeRange(input.left, edge, (RangeFlag)((input.flag & INCLUDE_LEFT) | INCLUDE_RIGHT));
}

TypeRange RangeOps::getRightIncluding(const TypeRange& input, double edge)
{
    if (edge <= input.left) return input;
    if (edge > input.right) return TypeRange(1,-1);

    return TypeRange(edge, input.right, (RangeFlag)((input.flag & INCLUDE_RIGHT) | INCLUDE_LEFT));

}

TypeRange RangeOps::getLeftExcluding(const TypeRange& input, double edge)
{
    if (edge >= input.right) return input;
    if (edge < input.left) return TypeRange(1,-1);

    return TypeRange(input.left, edge, (RangeFlag)(input.flag & INCLUDE_LEFT));
}

TypeRange RangeOps::getRightExcluding(const TypeRange& input, double edge)
{
    if (edge <= input.left) return input;
    if (edge > input.right) return TypeRange(1,-1);

    return TypeRange(edge, input.right, (RangeFlag)(input.flag & INCLUDE_RIGHT));

}