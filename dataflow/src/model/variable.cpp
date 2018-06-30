#include "variable.h"

Variable::Variable(BranchId branchId, CmdId cmdId, TypeRange initialRange) : initialRange(initialRange), lastChangeId(0), tail(NULL), isActiveFlag(true) {
	for (int i = 0; i < MAX_RANGE_CHANGES; i++) changes[i] = NULL;
	changeRange(branchId, cmdId, initialRange);
}

void Variable::changeRange(BranchId branchId, CmdId cmdId, const TypeRange& range)
{
	TypeRangeChange* change = new TypeRangeChange();
	change->branchId = branchId;
	change->cmdId = cmdId;
	change->newRange = range;
	changes[lastChangeId++] = change;
	BranchNode* node = tail;
	while (node) {
		if (node->branchId == branchId) {
			break;
		}
		node = node->prev; 
	}
	if (node) {
		node->change = change;
	}
	else {
		BranchNode* prev = tail;
		tail = new BranchNode();
		tail->branchId = branchId;
		tail->change = change;
		tail->prev = prev;
	}
}

void Variable::initBranch(BranchId branchId, BranchId parentBranchId)
{
	BranchNode* parentBranchNode = tail;
	while (parentBranchNode) {
		if (parentBranchNode->branchId == parentBranchId) {
			break;
		}
		parentBranchNode = parentBranchNode->prev;
	}
	if (parentBranchNode != NULL) {
		BranchNode* prev = tail;
		tail = new BranchNode();
		tail->branchId = branchId;
		tail->change = parentBranchNode->change;
		tail->prev = prev;
	}
}

void Variable::forget()
{
	isActiveFlag = false;
}

const TypeRange* Variable::getRange(BranchId branchId)
{
	//todo: instead, go through changes from tail and look for branch. or maybe have this array dynamic/linked list. and do not init if it is not used anymore
	//lookup is not so frequent, so it could be slower
	BranchNode* node = tail;
	while (node) {
		if (node->branchId == branchId) {
			return &node->change->newRange;
		}
		node = node->prev;
	}
	return NULL;
}

unsigned int Variable::getChangesCount() const
{
	return lastChangeId;
}

const TypeRangeChange& Variable::getChange(unsigned int id) const
{
	return *changes[id];
}



bool Variable::isActive() 
{
	return isActiveFlag;
}

Variable::~Variable()
{
	for (int i = 0; i < lastChangeId; i++) {
		delete changes[i];
	}
	BranchNode* node = tail, *tmp;
	while (node) {
		tmp = node;
		node = node->prev;
		delete tmp;
	}
}