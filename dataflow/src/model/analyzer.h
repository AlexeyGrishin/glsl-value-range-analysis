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
    const std::vector<Warning> getWarnings() const;
    const std::vector<Branch>  getBranches() const;
    const std::vector<VariableChange> getChanges() const;

};