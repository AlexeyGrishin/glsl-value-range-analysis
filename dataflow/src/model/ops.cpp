#include "ops.h"
#include "opcodes.h"
#include "range.h"
#include "log.h"

OpsRegistry::OpsRegistry()
{
    for (int i = 0; i < MAX_OPS; i++) {
        ops[i] = NULL;
    }
}

void OpsRegistry::add(BaseOp* op)
{
    ops[op->code] = op;
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

OpsRegistry* OpsRegistry::singleton = NULL;


OpsRegistry& OpsRegistry::instance()
{
    if (singleton == NULL) {
        singleton = new OpsRegistry();
    }
    return *singleton;
}
