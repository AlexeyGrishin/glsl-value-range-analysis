// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

#include "analyzer.h"
#include "opcodes.h"
#include "defs.h"

DataFlowAnalyzer::DataFlowAnalyzer()
    : context(), ops(OpsRegistry::instance()), local(&context)
{
    
}

//todo: set const everywhere
ProcessResult DataFlowAnalyzer::processCommand(Command* command)
{
    auto branches = context.getActiveBranches();
    local.start();

    for (auto branch: branches) {
        local.setup(branch->id, command);
        switch (command->opCode)
        {
            case _define_op:
                context.createVariable(branch->id, command->cmdId, command->getArgument(0), command->range);
                break;
            case _forget_op:
                context.forgetVariable(command->getArgument(0));
                break;
            case _ifeq_op:
            case _endif_op:
            case _watch_op:
            case _endwatch_op:
            case _ignore_op:
                break;
            default:
                if (ops.isKnown(command->opCode)) {
                    if (!branch->skip) ops.createBranches(command->opCode, local);
                } else {
                    //printf("%d = NULL\n", command->opCode);
                    local.onError(PR_UNKNOWN_OP);
                    return local.getStatus();
                }
                break;
        }
    }

    branches = context.getActiveBranches();
    for (auto branch: branches) {
        local.setup(branch->id, command);
        switch (command->opCode)
        {
            case _define_op:
            case _forget_op:
                break;
            case _ifeq_op:
                local.onIf();
                break;
            case _endif_op:
                local.onEndIf();
                break;
            case _watch_op:
                local.onWatch(command->getArgument(0));
                break;
            case _endwatch_op:
                local.onEndWatch(command->getArgument(0));
                break;
            case _ignore_op:
                local.onIgnoreWatch(command->getArgument(0));
                break;
            default:
                if (!branch->skip && ops.isKnown(command->opCode)) {
                    ops.process(command->opCode, local);
                }
                break;
        }
    }
    return local.getStatus();

}

const TypeRange* DataFlowAnalyzer::getRange(BranchId branchId, VarId varId) const
{
    return context.getRange(branchId, varId);
}

const std::vector<Warning> DataFlowAnalyzer::getWarnings() const
{
    return context.getWarnings(); 
}

const std::vector<Branch> DataFlowAnalyzer::getBranches() const
{
    return context.getBranches();
}

const std::vector<VariableChange> DataFlowAnalyzer::getChanges() const
{
    return context.getChanges();
}
