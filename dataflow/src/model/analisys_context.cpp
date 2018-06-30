#include "analisys_context.h"

AnalisysContext::AnalisysContext() 
    : maxCreatedId(0), nextBranchId(1), warningsCount(0)
{
    for (int i = 0; i < MAX_VARIABLES; i++) variables[i] = NULL;
    for (int i = 0; i < MAX_BRANCHES; i++) branches[i] = NULL;
    for (int i = 0; i < MAX_WARNINGS; i++) warnings[i] = NULL;
    branches[MAIN_BRANCH] = new Branch(MAIN_BRANCH, MAIN_BRANCH, 0, 0);
}

void AnalisysContext::createVariable(BranchId branchId, CmdId cmdId, VarId id, const TypeRange& range)
{
    if (variables[id] == NULL) {
        variables[id] = new Variable(MAIN_BRANCH, cmdId, range);
    }
    variables[id]->initBranch(branchId, MAIN_BRANCH);
    if (id >= maxCreatedId) maxCreatedId = id+1;
}

void AnalisysContext::forgetVariable(VarId id)
{
    getVariable(id).forget();
}

Variable& AnalisysContext::getVariable(VarId id) const
{
    return *variables[id];
} 

Branch* AnalisysContext::getFirstBranch() const
{
    for (int i = 0; i < nextBranchId; i++) {
        if (branches[i] != NULL && branches[i]->active) return branches[i];
    }
    return NULL;
}

BranchId AnalisysContext::getLastBranchId() const
{
    return nextBranchId-1;
}

Branch* AnalisysContext::getNextBranch(Branch* branch) const
{
    for (int i = branch->id+1; i < nextBranchId; i++) {
        if (branches[i] != NULL && branches[i]->active) return branches[i];
    }
    return NULL;    
}

Branch* AnalisysContext::getBranch(BranchId id) const
{
    return branches[id];
}


BranchId AnalisysContext::createBranch(BranchId parentId, CmdId cmdId, VarId varId, const TypeRange& range)
{
    BranchId id = nextBranchId++;
    Branch* branch = new Branch(id, parentId, cmdId, varId);
    branches[id] = branch;
    for (int i = 0; i < maxCreatedId; i++) {
        Variable* variable = variables[i];
        if (variable != NULL && variable->isActive()) {
            variable->initBranch(id, parentId);
            if (i == varId) {
                variable->changeRange(id, cmdId, range);
            }
        }
    }
    branches[parentId]->active = false;
    return id;
}

void AnalisysContext::addWarning(Command* command, BranchId branchId, VarId variable, unsigned int argNr, TypeRange expected, TypeRange actual)
{
    Warning* w = new Warning();
    w->cmdId = command->cmdId;
    w->call = command->opCode;
    w->varId = variable;
    w->branchId = branchId;
    w->argumentNr = argNr;
    w->expectedRange = expected;
    w->actualRange = actual;
    warnings[warningsCount++] = w;
}

Array<Warning> AnalisysContext::getWarnings() const
{
    Array<Warning> array(warningsCount);
    for (int i = 0; i < warningsCount; i++) {
        array.items[i] = *warnings[i];
    }
    return array;
}

Array<Branch> AnalisysContext::getBranches() const
{
    Array<Branch> array(nextBranchId);
    for (int i = 0; i < nextBranchId; i++) {
        array.items[i] = *branches[i];
    }
    return array;
}

Array<VariableChange> AnalisysContext::getChanges() const
{
    unsigned int total = 0;
    CmdId maxCmdId = 0;
    for (int i = 0; i < maxCreatedId; i++) {
        Variable* variable = variables[i];
        if (variable != NULL) {
            total += variable->getChangesCount();
            CmdId cmdId = variable->getChange(variable->getChangesCount()-1).cmdId;
            if (cmdId > maxCmdId) {
                maxCmdId = cmdId;
            }
        }
    }
    Array<VariableChange> array(total);
    int ai = 0;
    for (CmdId cmdId = 0; cmdId <= maxCmdId; cmdId++) {
        for (int i = 0; i < maxCreatedId; i++) {
            Variable* variable = variables[i];
            if (variable != NULL) {
                for (int ci = 0; ci < variable->getChangesCount(); ci++) {
                    const TypeRangeChange& change = variable->getChange(ci);
                    if (change.cmdId == cmdId) {
                        VariableChange& outChange = array.items[ai];
                        outChange.varId = i;
                        outChange.branchId = change.branchId;
                        outChange.cmdId = change.cmdId;
                        outChange.newRange = change.newRange;
                        ai++;
                    }
                }
            }
        }    
    }
    return array;

}



AnalisysContext::~AnalisysContext()
{
    for (int i = 0; i < MAX_VARIABLES; i++) delete variables[i];
    for (int i = 0; i < MAX_BRANCHES; i++) delete branches[i];
    for (int i = 0; i < MAX_WARNINGS; i++) delete warnings[i];

}