#include "analisys_context.h"
#include "ops.h"
#include "log.h"

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
    for (unsigned int i = 0; i < nextBranchId; i++) {
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
    for (unsigned int i = branch->id+1; i < nextBranchId; i++) {
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
    for (unsigned int i = 0; i < maxCreatedId; i++) {
        Variable* variable = variables[i];
        if (variable != NULL && variable->isActive()) { //todo: issue 1
            variable->initBranch(id, parentId);
            if (i == varId) {
                variable->changeRange(id, cmdId, range, TR_Branch, NULL);
            }
        }
    }
    //try to populate change up
    populateChanges(cmdId, id, parentId, varId, range);

    branches[parentId]->active = false;
    return id;
}

void AnalisysContext::populateChanges(CmdId cmdId, BranchId branchId, BranchId parentBranchId, VarId varId, const TypeRange& changedRange)
{
    const Variable* lastUpdatedVar = variables[varId];
    //todo: issue 1: so here we assume that variable has record for parentBranchId and it points to last change
    //even it occured in prev branch. BUT variable could be already inactive (after _forget), so in createBranch
    //it is ignored and record for new branch is not created. So there are options: 
    // 1. update inactive as well. bad - memory
    // 2. update inactive if they are dependent
    // 3. ... keep as is, because if var is inactive - it is not used later. We may skip updating it's range  
    const TypeRangeChange* change = lastUpdatedVar->getLastChangeForBranch(parentBranchId);
    if (change != NULL && change->restore != NULL)
    {
        RestoreRange* restorer = change->restore;
        Variable* dep = variables[restorer->getDependent()];
        const TypeRangeChange* depChange = dep->getLastChangeForBranch(parentBranchId);
        if (depChange != NULL && depChange->cmdId < restorer->getChangedAt()) {
            const TypeRange& newRange = restorer->restore(*(dep->getRange(parentBranchId)), changedRange);
            dep->changeRange(branchId, cmdId, newRange, TR_BackPropagation, NULL);
            //todo: maybe here I need to use not original branchId/parentBranchId?
            populateChanges(cmdId, branchId, parentBranchId, restorer->getDependent(), newRange);
        
        }
    }
    
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
    for (unsigned int i = 0; i < warningsCount; i++) {
        array.items[i] = *warnings[i];
    }
    return array;
}

Array<Branch> AnalisysContext::getBranches() const
{
    Array<Branch> array(nextBranchId);
    for (unsigned int i = 0; i < nextBranchId; i++) {
        array.items[i] = *branches[i];
    }
    return array;
}

Array<VariableChange> AnalisysContext::getChanges() const
{
    unsigned int total = 0;
    CmdId maxCmdId = 0;
    for (unsigned int i = 0; i < maxCreatedId; i++) {
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
        for (unsigned int i = 0; i < maxCreatedId; i++) {
            Variable* variable = variables[i];
            if (variable != NULL) {
                for (unsigned int ci = 0; ci < variable->getChangesCount(); ci++) {
                    const TypeRangeChange& change = variable->getChange(ci);
                    if (change.cmdId == cmdId) {
                        VariableChange& outChange = array.items[ai];
                        outChange.varId = i;
                        outChange.branchId = change.branchId;
                        outChange.cmdId = change.cmdId;
                        outChange.newRange = change.newRange;
                        outChange.revertable = change.restore != NULL;
                        outChange.reason = change.reason;
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