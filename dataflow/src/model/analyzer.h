#pragma once
#include "defs.h"
#include "command.h"
#include "analisys_context.h"
#include "ops.h"

class DataFlowAnalyzer {
private:
    AnalisysContext context;
    OpsRegistry& ops;
    LocalContext local;
public:
    DataFlowAnalyzer();
    ProcessResult processCommand(const Command* command);

    const TypeRange* getRange(BranchId branchId, VarId varId) const;
    const std::vector<Warning> getWarnings() const;
    const std::vector<Branch>  getBranches() const;
    const std::vector<VariableChange> getChanges() const;

};