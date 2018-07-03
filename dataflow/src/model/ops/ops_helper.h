#pragma once
#include "../ops.h"
#include "../analisys_context.h"

#define OPERATION(klsName, code, methodBody) class klsName: public BaseOp {public: klsName():BaseOp(code){} void process(LocalContext& ctx) { \
    methodBody \
}};
#define BRANCH_OPERATION(klsName, code, methodBody, branchBody) class klsName: public BaseOp {public: klsName():BaseOp(code){} void process(LocalContext& ctx) { \
    methodBody \
} bool createBranches(LocalContext& ctx) {\
    branchBody \
}};
#define REGISTER_START(name) void name() {
#define REGISTER(klsName) OpsRegistry::instance().add(new klsName);
#define REGISTER_END }


#define APPLY_VEC4(fn) ctx.set(0, fn(ctx.get(4))); \
if (ctx.isDefined(1)) ctx.set(1, fn(ctx.get(5))); \
if (ctx.isDefined(2)) ctx.set(2, fn(ctx.get(6))); \
if (ctx.isDefined(3)) ctx.set(3, fn(ctx.get(7)));

#define APPLY_VEC4_CTX(fn) fn(ctx, 0, 4); fn(ctx, 1, 5); fn(ctx, 2, 6); fn(ctx, 3, 7);

#define APPLY_VEC4_VEC4(fn) ctx.set(0, fn(ctx.get(4), ctx.get(8))); \
if (ctx.isDefined(1)) ctx.set(1, fn(ctx.get(5), ctx.get(9))); \
if (ctx.isDefined(2)) ctx.set(2, fn(ctx.get(6), ctx.get(10))); \
if (ctx.isDefined(3)) ctx.set(3, fn(ctx.get(7), ctx.get(11)));

#define APPLY_VEC4_VEC4_CTX(fn) fn(ctx, 0, 4, 8); fn(ctx, 1, 5, 9); fn(ctx, 2, 6, 10); fn(ctx, 3, 7, 11);