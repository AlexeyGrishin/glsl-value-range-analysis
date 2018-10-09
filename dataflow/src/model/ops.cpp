#include "ops.h"
#include "opcodes.h"
#include "range.h"
#include "log.h"

OpsRegistry::OpsRegistry()
{
}

void OpsRegistry::add(BaseOp* op)
{
    ops[op->code] = std::unique_ptr<BaseOp>(op);
}


bool OpsRegistry::isKnown(OpCode code)
{
    return (bool)ops[code];
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
}

OpsRegistry* OpsRegistry::singleton = nullptr;


OpsRegistry& OpsRegistry::instance()
{
    if (singleton == nullptr) {
        singleton = new OpsRegistry();
    }
    return *singleton;
}
