#pragma once
#include "defs.h"
#include "command.h"
#include "analisys_context.h"
#include "ops.h"

enum ProcessResult {
    PR_OK = 0,
    PR_ERROR = 1,

    PR_UNKNOWN_OP = 2
};

class DataFlowAnalyzer 
{
private:
    AnalisysContext context;
    OpsRegistry& ops;
    LocalContext local;
public:
    DataFlowAnalyzer();
    ProcessResult processCommand(Command* command);

    const TypeRange* getRange(BranchId branchId, VarId varId) const;
    //todo: maybe use std::list and v us ne dut?
    Array<Warning> getWarnings() const;
    Array<Branch>  getBranches() const;
    Array<VariableChange> getChanges() const;
};