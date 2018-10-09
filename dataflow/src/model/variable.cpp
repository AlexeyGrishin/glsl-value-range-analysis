#include "variable.h"
#include "ops.h"


TypeRangeChange::TypeRangeChange(BranchId branchId, CmdId cmdId, TypeRange range, RestoreRange* restore, TypeReason reason)
    : branchId(branchId), cmdId(cmdId), newRange(range), restore(restore), reason(reason)
{}

Variable::Variable(BranchId branchId, CmdId cmdId, TypeRange initialRange) 
: initialRange(initialRange), isActiveFlag(true) {
    changeRange(branchId, cmdId, initialRange, TR_Operation, nullptr);
}

void Variable::changeRange(BranchId branchId, CmdId cmdId, const TypeRange& range, TypeReason reason, RestoreRange* restorer)
{
    auto changePtr = std::make_unique<TypeRangeChange>(branchId, cmdId, range, restorer, reason);
    lastBranchChanges[branchId] = changePtr.get();
    changes.push_back(std::move(changePtr));
}

void Variable::initBranch(BranchId branchId, BranchId parentBranchId)
{
    auto parentChange = lastBranchChanges.find(parentBranchId);
    if (parentChange != lastBranchChanges.end()) {
        lastBranchChanges[branchId] = parentChange->second;
    }
}

void Variable::forget()
{
    isActiveFlag = false;
}

const TypeRange* Variable::getRange(BranchId branchId) const
{
    auto bit = lastBranchChanges.find(branchId);
    return bit == lastBranchChanges.end() ? nullptr : &bit->second->newRange;
} 

const TypeRangeChange* Variable::getLastChangeForBranch(BranchId branchId) const
{
    auto bit = lastBranchChanges.find(branchId);
    return bit == lastBranchChanges.end() ? nullptr : bit->second;
}

const std::vector<const TypeRangeChange*> Variable::getChanges() const
{
    std::vector<const TypeRangeChange*> out;
    for (auto& it: changes)
    {
        out.push_back(it.get());
    }
    return out;
}


bool Variable::isActive() 
{
    return isActiveFlag;
}

Variable::~Variable()
{
}