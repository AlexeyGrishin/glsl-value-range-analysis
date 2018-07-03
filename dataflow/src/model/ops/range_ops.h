#pragma once
#include "../range.h"

namespace RangeOps {
    TypeRange sin(const TypeRange& input);
    TypeRange cos(const TypeRange& input);
    TypeRange min(const TypeRange& arg1, const TypeRange& arg2);
    TypeRange max(const TypeRange& arg1, const TypeRange& arg2);
    TypeRange power(const TypeRange& arg1, const TypeRange& arg2);
    TypeRange clamp(const TypeRange& arg1, const TypeRange& arg2, const TypeRange& arg3);
    TypeRange floor(const TypeRange& arg1);
    TypeRange fract(const TypeRange& arg1);
    TypeRange abs(const TypeRange& arg1);
    TypeRange length(const TypeRange& arg1, const TypeRange& arg2, const TypeRange& arg3, const TypeRange& arg4);

    TypeRange mix(const TypeRange& arg1, const TypeRange& arg2, const TypeRange& mix);

    TypeRange dot(const TypeRange& arg1, const TypeRange& arg2, const TypeRange& arg3, const TypeRange& arg4,
                  const TypeRange& arg5, const TypeRange& arg6, const TypeRange& arg7, const TypeRange& arg8
    );

    TypeRange getLeftIncluding(const TypeRange& input, double edge);
    TypeRange getRightIncluding(const TypeRange& input, double edge);
    TypeRange getLeftExcluding(const TypeRange& input, double edge);
    TypeRange getRightExcluding(const TypeRange& input, double edge);
}