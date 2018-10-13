// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

#include "analisys_context.h"
#include "ops.h"
#include "log.h"
#include <memory>

AnalisysContext::AnalisysContext() 
    : maxCreatedId(0) {
    branches.push_back(std::make_unique<Branch>(MAIN_BRANCH, MAIN_BRANCH, 0, 0));
}

void AnalisysContext::createVariable(BranchId branchId, CmdId cmdId, VarId id, const TypeRange& range) {
    auto varIt = variables.find(id);
    if (varIt == variables.end()) {
        varIt = variables.emplace(id, std::make_unique<Variable>(MAIN_BRANCH, cmdId, range)).first;
    }
    varIt->second->initBranch(branchId, MAIN_BRANCH);
    if (id >= maxCreatedId) maxCreatedId = id+1;
}

const Variable* AnalisysContext::forgetVariable(VarId id) {
    Variable* var = getVariable(id);
    if (var != nullptr) {
        var->forget();
        return var;
    }
    return nullptr;
}

Variable* AnalisysContext::getVariable(VarId id) {
    auto varIt = variables.find(id);
    if (varIt == std::end(variables)) {
        return nullptr;
    }
    return varIt->second.get();
} 

const TypeRange* AnalisysContext::getRange(BranchId branchId, VarId varId) const {
    auto varIt = variables.find(varId);
    if (varIt == std::end(variables)) {
        return nullptr;
    }
    return varIt->second->getRange(branchId);
}

const Variable* AnalisysContext::getVariable(VarId id) const {
    auto varIt = variables.find(id);
    if (varIt == std::end(variables)) {
        return nullptr;
    }
    return varIt->second.get();
} 

std::vector<const Branch*> AnalisysContext::getActiveBranches() const {
    std::vector<const Branch*> output;
    for (auto& bptr: branches) {
        if (bptr->active) {
            output.push_back(bptr.get());
        }
    }
    return output;
}


Branch* AnalisysContext::getBranch(BranchId id) const {
    return branches[id].get();
}


BranchId AnalisysContext::createBranch(BranchId parentId, CmdId cmdId, VarId varId, const TypeRange& range) {
    BranchId id = branches.size();
    branches.push_back(std::make_unique<Branch>(id, parentId, cmdId, varId));
    for (auto &it: variables) {
        auto &variable = it.second;
        if (variable->isActive()) { 
            variable->initBranch(id, parentId);
            if (it.first == varId) {
                variable->changeRange(id, cmdId, range, TR_Branch, nullptr);
            }
        }
    }
    //try to populate change up
    populateChanges(cmdId, id, parentId, varId, range);

    branches[parentId]->active = false;
    return id;
}

void AnalisysContext::populateChanges(CmdId cmdId, BranchId branchId, BranchId parentBranchId, VarId varId, const TypeRange& changedRange) {
    auto &lastUpdatedVar = variables.at(varId);
    //todo: issue 1: so here we assume that variable has record for parentBranchId and it points to last change
    //even it occured in prev branch. BUT variable could be already inactive (after _forget), so in createBranch
    //it is ignored and record for new branch is not created. So there are options: 
    // 1. update inactive as well. bad - memory
    // 2. update inactive if they are dependent
    // 3. ... keep as is, because if var is inactive - it is not used later. We may skip updating it's range  
    const TypeRangeChange* change = lastUpdatedVar->getLastChangeForBranch(parentBranchId);
    if (change != nullptr && change->restore != nullptr)
    {
        auto &restorer = change->restore;
        auto &dep = variables.at(restorer->getDependent());
        const TypeRangeChange* depChange = dep->getLastChangeForBranch(parentBranchId);
        if (depChange != nullptr && depChange->cmdId < restorer->getChangedAt()) {
            const TypeRange& newRange = restorer->restore(*(dep->getRange(parentBranchId)), changedRange);
            dep->changeRange(branchId, cmdId, newRange, TR_BackPropagation, nullptr);
            populateChanges(cmdId, branchId, parentBranchId, restorer->getDependent(), newRange);
        
        }
    }
}

void AnalisysContext::addWarning(const Command* command, BranchId branchId, VarId variable, unsigned int argNr, const TypeRange& expected, const TypeRange& actual) {
    Warning w = {branchId, command->cmdId, command->opCode, argNr, variable, expected, actual};
    warnings.push_back(w);
}

const std::vector<Warning> AnalisysContext::getWarnings() const {
    return warnings;
}

const std::vector<Branch> AnalisysContext::getBranches() const {
    std::vector<Branch> out;
    for (auto &bptr: branches) {
        out.push_back(*(bptr.get()));
    }
    return out;
}

const std::vector<VariableChange> AnalisysContext::getChanges() const {
    std::vector<VariableChange> array;
    for (auto& it: variables) {
        auto& variable = it.second;
        auto changes = variable->getChanges();
        for (auto& change: changes) {
            VariableChange outChange = {it.first, change->branchId, change->cmdId, change->newRange, change->reason, change->restore.get() != nullptr};
            array.push_back(outChange);
        }
    }
    std::sort(array.begin(), array.end(), [](VariableChange& v1, VariableChange& v2) {
        if (v1.cmdId > v2.cmdId) return false;
        if (v1.cmdId < v2.cmdId) return true;
        return v1.varId < v2.varId;
    });
    
    return array;
}



AnalisysContext::~AnalisysContext() {
}