#pragma once
#include "defs.h"
#include "range.h"
#include <map>
#include <vector>
#include <memory>

class RestoreRange;

struct TypeRangeChange {
    BranchId branchId;
    CmdId cmdId;
    TypeRange newRange;
    std::unique_ptr<RestoreRange> restore;
    TypeReason reason;

    TypeRangeChange(BranchId branchId, CmdId cmdId, TypeRange range, RestoreRange* restore, TypeReason reason);
};

class Variable {
private:
    TypeRange initialRange;
    std::vector<std::unique_ptr<TypeRangeChange>> changes;
    std::map<BranchId, TypeRangeChange*> lastBranchChanges;
    bool isActiveFlag;
public:

    Variable(BranchId branchId, CmdId cmdId, TypeRange initialRange);

    bool isActive();
    void changeRange(BranchId branchId, CmdId cmdId, const TypeRange& range, TypeReason reason, RestoreRange* restorer);
    void initBranch(BranchId branchId, BranchId parentBranchId);
    void forget();

    const TypeRange* getRange(BranchId branchId) const;
    const TypeRangeChange* getLastChangeForBranch(BranchId branchId) const;

    //todo: return iterator would be better
    const std::vector<const TypeRangeChange*> getChanges() const;

    ~Variable();
    
};