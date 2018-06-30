#include "ops.h"
#include "opcodes.h"
#include "analisys_context.h"
#include "range.h"
#include "range_ops.h"

class BaseOp {
public:
    OpCode code;
    BaseOp(OpCode code): code(code) {}
    
    virtual bool createBranches(LocalContext& ctx) { return false; }
    virtual void process(LocalContext& ctx) = 0;
    
    virtual ~BaseOp() {}
};

#define OPERATION(klsName, code, methodBody) class klsName: public BaseOp {public: klsName():BaseOp(code){} void process(LocalContext& ctx) { \
    methodBody \
}};
#define BRANCH_OPERATION(klsName, code, methodBody, branchBody) class klsName: public BaseOp {public: klsName():BaseOp(code){} void process(LocalContext& ctx) { \
    methodBody \
} bool createBranches(LocalContext& ctx) {\
    branchBody \
}};
#define REGISTER(klsName) {BaseOp* op = new klsName; ops[op->code] = op;}

OPERATION(PlusOp, plus_op, {
    //12 args, 4 out, 4 arg1, 4 arg2
    ctx.set(0, ctx.get(4) + ctx.get(8));
    if (ctx.isDefined(1)) ctx.set(1, ctx.get(5) + ctx.get(9));
    if (ctx.isDefined(2)) ctx.set(2, ctx.get(6) + ctx.get(10));
    if (ctx.isDefined(3)) ctx.set(3, ctx.get(7) + ctx.get(11));
});

OPERATION(MinusOp, minus_op, {
    //12 args, 4 out, 4 arg1, 4 arg2
    ctx.set(0, ctx.get(4) - ctx.get(8));
    if (ctx.isDefined(1)) ctx.set(1, ctx.get(5) - ctx.get(9));
    if (ctx.isDefined(2)) ctx.set(2, ctx.get(6) - ctx.get(10));
    if (ctx.isDefined(3)) ctx.set(3, ctx.get(7) - ctx.get(11));
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
    ctx.set(0, RangeOps::sin(ctx.get(4)));
    if (ctx.isDefined(1)) ctx.set(1, RangeOps::sin(ctx.get(5)));
    if (ctx.isDefined(2)) ctx.set(2, RangeOps::sin(ctx.get(6)));
    if (ctx.isDefined(3)) ctx.set(3, RangeOps::sin(ctx.get(7)));
});

OPERATION(CosOp, cos_op, {
    //8 args, 4 out, 4 in
    ctx.set(0, RangeOps::cos(ctx.get(4)));
    if (ctx.isDefined(1)) ctx.set(1, RangeOps::cos(ctx.get(5)));
    if (ctx.isDefined(2)) ctx.set(2, RangeOps::cos(ctx.get(6)));
    if (ctx.isDefined(3)) ctx.set(3, RangeOps::cos(ctx.get(7)));
});

OPERATION(TextureOp, texture2D_op, {
    ctx.set(0, TypeRange(0, 1));
    ctx.set(1, TypeRange(0, 1));
    ctx.set(2, TypeRange(0, 1));
    ctx.set(3, TypeRange(0, 1));
});

OPERATION(AssignOp, assign_op, {
    ctx.set(0, ctx.get(4));
    if (ctx.isDefined(1)) ctx.set(1, ctx.get(5));
    if (ctx.isDefined(2)) ctx.set(2, ctx.get(6));
    if (ctx.isDefined(3)) ctx.set(3, ctx.get(7));
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
    } else if (left.isSingle()) {
        ctx.set(0, !right.includes(left.left));        
    }
}, {
    TypeRange left = ctx.get(1);
    TypeRange right = ctx.get(5);
    if (right.isSingle()) {
        ctx.createBranch(1, RangeOps::getLeftExcluding(left, right.left));
        ctx.createBranch(1, RangeOps::getRightIncluding(left, right.left));
        return true;
    } else if (left.isSingle()) {
        ctx.createBranch(5, RangeOps::getLeftExcluding(right, left.left));
        ctx.createBranch(5, RangeOps::getRightIncluding(right, left.left));
        return true;
    }
    return false;
    //todo: support 2 ranges
});

BRANCH_OPERATION(GtOp, gt_op, {
    //todo: for now - only floats. Imma lazy 
    TypeRange left = ctx.get(1);
    TypeRange right = ctx.get(5);
    if (right.isSingle()) {
        ctx.set(0, !left.includes(right.left));
    } else if (left.isSingle()) {
        ctx.set(0, !right.includes(left.left));        
    }
}, {
    TypeRange left = ctx.get(1);
    TypeRange right = ctx.get(5);
    if (right.isSingle()) {
        ctx.createBranch(1, RangeOps::getLeftIncluding(left, right.left));
        ctx.createBranch(1, RangeOps::getRightExcluding(left, right.left));
        return true;
    } else if (left.isSingle()) {
        ctx.createBranch(5, RangeOps::getLeftIncluding(right, left.left));
        ctx.createBranch(5, RangeOps::getRightExcluding(right, left.left));
        return true;
    }
    return false;
    //todo: support 2 ranges
});

OpsRegistry::OpsRegistry()
{
    for (int i = 0; i < MAX_OPS; i++) {
        ops[i] = NULL;
    }
    REGISTER(PlusOp)
    REGISTER(MinusOp)
    REGISTER(MulOp)
    REGISTER(DivOp)
    REGISTER(SinOp)
    REGISTER(CosOp)
    REGISTER(TextureOp)
    REGISTER(AssignOp)
    REGISTER(OutputOp)
    REGISTER(LtOp)
    REGISTER(GtOp)
}

bool OpsRegistry::isKnown(OpCode code)
{
    return ops[code] != NULL;
}

bool OpsRegistry::createBranches(OpCode code, LocalContext& ctx)
{
    return ops[code]->createBranches(ctx);
}

void OpsRegistry::process(OpCode code, LocalContext& ctx)
{
    ops[code]->process(ctx);
}


OpsRegistry::~OpsRegistry() {
    for (int i = 0; i < MAX_OPS; i++) {
        delete ops[i];
    }
}

