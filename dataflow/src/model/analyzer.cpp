#include "analyzer.h"
#include "opcodes.h"
#include "defs.h"

DataFlowAnalyzer::DataFlowAnalyzer()
    : context(), ops(), local(&context)
{
    
}

//todo: set const everywhere
ProcessResult DataFlowAnalyzer::processCommand(Command* command)
{
    Branch* branch = context.getFirstBranch();
    BranchId lastId = context.getLastBranchId();
    ProcessResult res = PR_OK;
    while (branch != NULL && (branch->id <= lastId)) {
        local.setup(branch->id, command);
        switch (command->opCode)
        {
            case _define_op:
                context.createVariable(branch->id, command->cmdId, command->arguments[0], command->range);
                break;
            case _forget_op:
                context.forgetVariable(command->arguments[0]);
                break;
            case _ifeq_op:
                break;
            case _endif_op:
                break;
            default:
                if (ops.isKnown(command->opCode)) {
                    if (!branch->skip) ops.createBranches(command->opCode, local);
                } else {
                    //printf("%d = NULL\n", command->opCode);
                    res = PR_ERROR;
                }
                break;
        }
        branch = context.getNextBranch(branch);
    }

    branch = context.getFirstBranch();
    while (branch != NULL) {
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
            default:
                if (!branch->skip && ops.isKnown(command->opCode)) {
                    ops.process(command->opCode, local);
                }
                break;
        }
        branch = context.getNextBranch(branch);
    }
    return res;

}

const TypeRange* DataFlowAnalyzer::getRange(BranchId branchId, VarId varId) const
{
    return context.getVariable(varId).getRange(branchId);
}

Array<Warning> DataFlowAnalyzer::getWarnings() const
{
    return context.getWarnings(); 
}

Array<Branch> DataFlowAnalyzer::getBranches() const
{
    return context.getBranches();
}

Array<VariableChange> DataFlowAnalyzer::getChanges() const
{
    return context.getChanges();
}
