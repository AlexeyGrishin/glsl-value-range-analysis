// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

#include "ops_helper.h"
#include "range_ops.h"

class RestorePlus : public RestoreRange {
private:
    TypeRange addedPart;
public:
    RestorePlus(const TypeRange& addedPart) : addedPart(addedPart) {}
    TypeRange restore(const TypeRange& myRange, const TypeRange& shrinkedRange) {
        return shrinkedRange - addedPart;
    }
};

class RestoreMinus : public RestoreRange {
private:
    TypeRange subPart;
public:
    RestoreMinus(const TypeRange& subPart) : subPart(subPart) {}
    TypeRange restore(const TypeRange& myRange, const TypeRange& shrinkedRange) {
        return shrinkedRange + subPart;
    }
};

class RestoreAssign : public RestoreRange {
public:
    TypeRange restore(const TypeRange& myRange, const TypeRange& shrinkedRange) {
        return shrinkedRange;
    }
};

class RestoreUnaryMinus: public RestoreRange {
public:
    TypeRange restore(const TypeRange& myRange, const TypeRange& shrinkedRange) {
        return -shrinkedRange;
    }    
};

void plus(LocalContext& ctx, unsigned int out, unsigned int arg1, unsigned int arg2) {
    if (!ctx.isDefined(out)) return;
    const TypeRange& r1 = ctx.get(arg1);
    const TypeRange& r2 = ctx.get(arg2);
    RestorePlus* restore = nullptr;
    unsigned int dependent = 0;
    if (r2.isSingle()) {
        restore = new RestorePlus(r2);
        dependent = arg1;
    }
    else if (r1.isSingle()) {
        restore = new RestorePlus(r1);
        dependent = arg2;
    }
    ctx.set(out, r1 + r2, restore, dependent);
}

OPERATION(PlusOp, plus_op, {
    //12 args, 4 out, 4 arg1, 4 arg2
    APPLY_VEC4_VEC4_CTX(plus)
});

void minus(LocalContext& ctx, unsigned int out, unsigned int arg1, unsigned int arg2) {
    if (!ctx.isDefined(out)) return;
    const TypeRange& r1 = ctx.get(arg1);
    const TypeRange& r2 = ctx.get(arg2);
    RestoreMinus* restore = nullptr;
    unsigned int dependent = 0;
    if (r2.isSingle()) {
        restore = new RestoreMinus(r2);
        dependent = arg1;
    }
    else if (r1.isSingle()) {
        restore = new RestoreMinus(r1);
        dependent = arg2;
    }
    ctx.set(out, r1 - r2, restore, dependent);
}

OPERATION(MinusOp, minus_op, {
    APPLY_VEC4_VEC4_CTX(minus)
});

OPERATION(MulOp, mul_op, {
    //12 args, 4 out, 4 arg1, 4 arg2
    ctx.set(0, ctx.get(4) * ctx.get(8));
    if (ctx.isDefined(1)) ctx.set(1, ctx.get(5) * ctx.get(9));
    if (ctx.isDefined(2)) ctx.set(2, ctx.get(6) * ctx.get(10));
    if (ctx.isDefined(3)) ctx.set(3, ctx.get(7) * ctx.get(11));
});

OPERATION(DivOp, div_op, {
    //12 args, 4 out, 4 arg1, 4 arg2
    ctx.set(0, ctx.get(4) / ctx.get(8));
    if (ctx.isDefined(1)) ctx.set(1, ctx.get(5) / ctx.get(9));
    if (ctx.isDefined(2)) ctx.set(2, ctx.get(6) / ctx.get(10));
    if (ctx.isDefined(3)) ctx.set(3, ctx.get(7) / ctx.get(11));
});

OPERATION(SinOp, sin_op, {
    APPLY_VEC4(RangeOps::sin)
});

OPERATION(CosOp, cos_op, {
    //8 args, 4 out, 4 in
    APPLY_VEC4(RangeOps::cos)
});

OPERATION(TextureOp, texture2D_op, {
    ctx.set(0, TypeRange(0, 1));
    ctx.set(1, TypeRange(0, 1));
    ctx.set(2, TypeRange(0, 1));
    ctx.set(3, TypeRange(0, 1));
});

void assign(LocalContext& ctx, unsigned int out, unsigned int arg) {
    if (!ctx.isDefined(out)) return;
    ctx.set(out, ctx.get(arg), new RestoreAssign(), arg);
}

OPERATION(AssignOp, assign_op, {
    APPLY_VEC4_CTX(assign)
});

OPERATION(OutputOp, _output_op, {
    TypeRange expected(0, 1);
    for (int i = 0; i < 4; i++) {
        if (ctx.isDefined(i) && !expected.includes(ctx.get(i))) {
            ctx.addWarning(i, expected);
        }
    }
});

BRANCH_OPERATION(LtOp, lt_op, {
    //0 - out, float
    //1,2,3,4 - in1
    //5,6,7,8 - in2
    //todo: for now - only floats. Imma lazy 
    TypeRange left = ctx.get(1);
    TypeRange right = ctx.get(5);
    if (right.isSingle()) {
        ctx.set(0, !left.includes(right.left));
    }
    else if (left.isSingle()) {
        ctx.set(0, !right.includes(left.left));
    }
}, {
    TypeRange left = ctx.get(1);
    TypeRange right = ctx.get(5);
    //todo: what if expression is always true for current branch? don't need to branch in this case. need to check and tests
    if (right.isSingle()) {
        ctx.createBranch(1, RangeOps::getLeftExcluding(left, right.left));
        ctx.createBranch(1, RangeOps::getRightIncluding(left, right.left));
        return true;
    }
    else if (left.isSingle()) {
        ctx.createBranch(5, RangeOps::getLeftExcluding(right, left.left));
        ctx.createBranch(5, RangeOps::getRightIncluding(right, left.left));
        return true;
    }
    return false;
    //todo: support 2 ranges
});

void gte(LocalContext& ctx, unsigned int out, unsigned int arg1, unsigned int arg2) {
    if (!ctx.isDefined(out)) return;
    TypeRange left = ctx.get(arg1);
    TypeRange right = ctx.get(arg2);
    if (right.isSingle()) {
        ctx.set(out, left.includes(right.left));
    }
    else if (left.isSingle()) {
        ctx.set(out, right.includes(left.left));
    }
}

BRANCH_OPERATION(StepOp, step_op, {
    //0,1,2,3
    //4,5,6,7 - in1
    //8,9,10,11 - in2
    //todo: for now - only one float as edge 
    gte(ctx, 0, 4, 8);
    gte(ctx, 1, 4, 9);
    gte(ctx, 2, 4, 10);
    gte(ctx, 3, 4, 11);
    }, {
        TypeRange left = ctx.get(4);
    if (!left.isSingle()) return false;
    for (int i = 8; i <= 11; i++) {
        if (!ctx.isDefined(i)) break;
        TypeRange right = ctx.get(i);
        ctx.createBranch(i, RangeOps::getLeftExcluding(right, left.left));
        ctx.createBranch(i, RangeOps::getRightIncluding(right, left.left));
    }
    return true;
});

BRANCH_OPERATION(GtOp, gt_op, {
    //todo: for now - only floats. Imma lazy 
    TypeRange left = ctx.get(1);
    TypeRange right = ctx.get(5);
    if (right.isSingle()) {
        ctx.set(0, !left.includes(right.left));
    }
    else if (left.isSingle()) {
        ctx.set(0, !right.includes(left.left));
    }
}, {
    TypeRange left = ctx.get(1);
    TypeRange right = ctx.get(5);
    if (right.isSingle()) {
        ctx.createBranch(1, RangeOps::getLeftIncluding(left, right.left));
        ctx.createBranch(1, RangeOps::getRightExcluding(left, right.left));
        return true;
    }
    else if (left.isSingle()) {
        ctx.createBranch(5, RangeOps::getLeftIncluding(right, left.left));
        ctx.createBranch(5, RangeOps::getRightExcluding(right, left.left));
        return true;
    }
    return false;
    //todo: support 2 ranges
});

OPERATION(MinOp, min_op, {
    //12 ops, 4 out, 4 in1, 4 in2
    APPLY_VEC4_VEC4(RangeOps::min)
});

OPERATION(MaxOp, max_op, {
    //12 ops, 4 out, 4 in1, 4 in2
    APPLY_VEC4_VEC4(RangeOps::max)
});

void validatePower(LocalContext& ctx, unsigned int out, unsigned int arg1, unsigned int arg2) {
    if (!ctx.isDefined(out)) return;
    const TypeRange& base = ctx.get(arg1);
    const TypeRange& exp = ctx.get(arg2);
    //todo: check spec about power(a,b) when b < 0
    if (base.left <= 0 || base.right < 0 || base.includes(0)) {
        ctx.addWarning(arg1, TypeRange(0, INFINITY, EXCLUDE_BOTH));
    }
    ctx.set(out, RangeOps::power(base, exp));
}

OPERATION(PowOp, power_op, {
    APPLY_VEC4_VEC4_CTX(validatePower)
});

OPERATION(ClampOp, clamp_op, {
    //todo: support all cases. for now - only vec4 clamp(vec4, float min, float max)
    //4 out, 4 arg1, 1 arg2, 1 arg3
    ctx.set(0, RangeOps::clamp(ctx.get(4), ctx.get(8), ctx.get(9)));
    if (ctx.isDefined(1)) ctx.set(1, RangeOps::clamp(ctx.get(5), ctx.get(8), ctx.get(9)));
    if (ctx.isDefined(2)) ctx.set(2, RangeOps::clamp(ctx.get(6), ctx.get(8), ctx.get(9)));
    if (ctx.isDefined(3)) ctx.set(3, RangeOps::clamp(ctx.get(7), ctx.get(8), ctx.get(9)));
});

OPERATION(FloorOp, floor_op, {
    APPLY_VEC4(RangeOps::floor)
});
OPERATION(FractOp, fract_op, {
    APPLY_VEC4(RangeOps::fract)
});


OPERATION(LengthOp, length_op, {
    //out = 1 (float)
    //in = 1-4
    TypeRange zero(0);
    const TypeRange& arg1 = ctx.get(1);
    const TypeRange& arg2 = ctx.isDefined(2) ? ctx.get(2) : zero;
    const TypeRange& arg3 = ctx.isDefined(3) ? ctx.get(3) : zero;
    const TypeRange& arg4 = ctx.isDefined(4) ? ctx.get(4) : zero;
    ctx.set(0, RangeOps::length(arg1, arg2, arg3, arg4));
});

OPERATION(MixOp, mix_op, {
    const TypeRange& aArg = ctx.get(12);
    TypeRange expected(0, 1);
    if (!expected.includes(aArg)) {
        ctx.addWarning(12, expected);
    }
    //todo: implement all cases. for now - only vec4 a = mix(vec4, vec4, float)
    ctx.set(0, RangeOps::mix(ctx.get(4), ctx.get(8), aArg));
    if (ctx.isDefined(1)) ctx.set(1, RangeOps::mix(ctx.get(5), ctx.get(9), aArg));
    if (ctx.isDefined(2)) ctx.set(2, RangeOps::mix(ctx.get(6), ctx.get(10), aArg));
    if (ctx.isDefined(3)) ctx.set(3, RangeOps::mix(ctx.get(7), ctx.get(11), aArg));
});

OPERATION(NormalizeOp, normalize_op, {
    //todo: we can calculate here for cases when vector ranges are small/simple
    ctx.set(0, TypeRange(0, 1));
    if (ctx.isDefined(1)) ctx.set(1, TypeRange(0, 1));
    if (ctx.isDefined(2)) ctx.set(2, TypeRange(0, 1));
    if (ctx.isDefined(3)) ctx.set(3, TypeRange(0, 1));
});

OPERATION(UnaryMinusOp, unary_minus_op, {
    ctx.set(0, -ctx.get(4), new RestoreUnaryMinus(), 4);
    if (ctx.isDefined(1)) ctx.set(1, -ctx.get(5), new RestoreUnaryMinus(), 5);
    if (ctx.isDefined(2)) ctx.set(2, -ctx.get(6), new RestoreUnaryMinus(), 6); 
    if (ctx.isDefined(3)) ctx.set(3, -ctx.get(7), new RestoreUnaryMinus(), 7);
});

OPERATION(DotOp, dot_op, {
    TypeRange zero(0);
    const TypeRange& arg1 = ctx.get(1);
    const TypeRange& arg2 = ctx.isDefined(2) ? ctx.get(2) : zero;
    const TypeRange& arg3 = ctx.isDefined(3) ? ctx.get(3) : zero;
    const TypeRange& arg4 = ctx.isDefined(4) ? ctx.get(4) : zero;
    const TypeRange& arg5 = ctx.isDefined(5) ? ctx.get(5) : zero;
    const TypeRange& arg6 = ctx.isDefined(6) ? ctx.get(6) : zero;
    const TypeRange& arg7 = ctx.isDefined(7) ? ctx.get(7) : zero;
    const TypeRange& arg8 = ctx.isDefined(8) ? ctx.get(8) : zero;
    ctx.set(0, RangeOps::dot(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8));
});

//todo: vec2/vec3/vec4
//todo: tests
OPERATION(OrOp, or_op, {
    ctx.set(0, ctx.get(1) || ctx.get(5));
});

OPERATION(AndOp, and_op, {
    ctx.set(0, ctx.get(1) && ctx.get(5));
});

OPERATION(CopyOp, _copy_op, {
    ctx.set(0, ctx.get(1));
});

REGISTER_START(registerBuiltinOps)
REGISTER(PlusOp)
REGISTER(MinusOp)
REGISTER(MulOp)
REGISTER(DivOp)
REGISTER(SinOp)
REGISTER(CosOp)
REGISTER(FloorOp)
REGISTER(FractOp)
REGISTER(TextureOp)
REGISTER(AssignOp)
REGISTER(OutputOp)
REGISTER(LtOp)
REGISTER(GtOp)
REGISTER(StepOp)
REGISTER(MaxOp)
REGISTER(MinOp)
REGISTER(PowOp)
REGISTER(ClampOp)
REGISTER(LengthOp)
REGISTER(MixOp)
REGISTER(NormalizeOp)
REGISTER(UnaryMinusOp)
REGISTER(DotOp)
REGISTER(OrOp)
REGISTER(AndOp)
REGISTER(CopyOp)
REGISTER_END
