#pragma once
#include "defs.h"
#include "opcodes.h"

class LocalContext;
class BaseOp;

class OpsRegistry {
private:
    BaseOp* ops[MAX_OPS];
public:
    OpsRegistry();
    bool isKnown(OpCode code);
    bool createBranches(OpCode code, LocalContext& ctx);
    void process(OpCode code, LocalContext& ctx);    
    ~OpsRegistry();
};
