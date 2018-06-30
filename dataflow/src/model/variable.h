#pragma once
#include "defs.h"
#include "range.h"

struct TypeRangeChange {
	BranchId branchId;
	CmdId cmdId;
	TypeRange newRange;
	//todo: here could be linked list instead
};

struct BranchNode {
	BranchId branchId;
	TypeRangeChange* change;
	BranchNode* prev;
};

class Variable {
private:
	TypeRange initialRange;
	int lastChangeId;
	TypeRangeChange* changes[MAX_RANGE_CHANGES];
	BranchNode* tail;
	bool isActiveFlag;
public:

	Variable(BranchId branchId, CmdId cmdId, TypeRange initialRange);

	bool isActive();
	void changeRange(BranchId branchId, CmdId cmdId, const TypeRange& range);
	void initBranch(BranchId branchId, BranchId parentBranchId);
	void forget();
	const TypeRange* getRange(BranchId branchId);

	unsigned int getChangesCount() const;
	const TypeRangeChange& getChange(unsigned int id) const;

	~Variable();
	
};