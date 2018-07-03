#pragma once
#include "defs.h"
#include "opcodes.h"
#include "range.h"

class LocalContext;

class BaseOp {
public:
    OpCode code;
    BaseOp(OpCode code) : code(code) {}

    virtual bool createBranches(LocalContext& ctx) { return false; }
    virtual void process(LocalContext& ctx) = 0;

    virtual ~BaseOp() {}
};



// when b = op(a), RestoreRange allows to get range for a when range for b is shrinked
// link to it is stored in RangeChange
class RestoreRange {
private:
    //a
    VarId dependent;
    CmdId changedAt;
public:
    void setDependent(VarId dependent) { this->dependent = dependent; }
    VarId getDependent() { return this->dependent; }
    void setChangedAt(CmdId changedAt) { this->changedAt = changedAt; }
    CmdId getChangedAt() { return this->changedAt; }

    //myRange = a, shrinkedRange = b
    //returns - new for a
    virtual TypeRange restore(const TypeRange& myRange, const TypeRange& shrinkedRange) = 0;
    virtual ~RestoreRange() {};
};

class OpsRegistry {
private:
    BaseOp* ops[MAX_OPS];
    static OpsRegistry* singleton;
    OpsRegistry();
public:
    void add(BaseOp* op);
    bool isKnown(OpCode code);
    bool createBranches(OpCode code, LocalContext& ctx);
    void process(OpCode code, LocalContext& ctx);    
    static OpsRegistry& instance();
    ~OpsRegistry();
};
