#pragma once
#include "defs.h"
#include "command.h"
#include "variable.h"
#include "ops.h"
#include <vector>
#include <map>

struct Branch;
struct Warning;
struct VariableChange;

class AnalisysContext {
private:
    //todo: redo with linked lists. also there could be separate list of active variables/branches, 
    //to iterate over them only 
    //todo: do I need map here? not just vector?
    std::map<VarId, std::unique_ptr<Variable>> variables;
    VarId maxCreatedId;
    std::vector<std::unique_ptr<Branch>> branches;
    std::vector<Warning> warnings;

    void populateChanges(CmdId id, BranchId branchId, BranchId parentBranchId, VarId varId, const TypeRange& changedRange);
public:
    AnalisysContext();

    void createVariable(BranchId branchId, CmdId cmdId, VarId id, const TypeRange& range);
    void forgetVariable(VarId id);
    Variable& getVariable(VarId id); 
    const Variable& getVariable(VarId id) const; 
    BranchId createBranch(BranchId parentId, CmdId cmdId, VarId variable, const TypeRange& range);

    std::vector<const Branch*> getActiveBranches() const;
    Branch* getBranch(BranchId id) const;

    void addWarning(const Command* command, BranchId branchId, VarId variable, unsigned int argNr, TypeRange expected, TypeRange actual);

    const std::vector<Warning> getWarnings() const;
    const std::vector<Branch>  getBranches() const;
    const std::vector<VariableChange> getChanges() const;


    ~AnalisysContext();
};

struct VariableChange {
    VarId varId;
    BranchId branchId;
    CmdId cmdId;
    TypeRange newRange;
    TypeReason reason;
    bool revertable;
};

struct Branch {
    BranchId id;
    BranchId parentId;
    CmdId cmdId;
    VarId varId;
    bool active;

    bool skip;
    unsigned int nestedIfs;

    VarId stopSkipOnWatchEnd;

    Branch(){}

    Branch(BranchId id, BranchId parentId, CmdId cmdId, VarId varId): 
        id(id), parentId(parentId), cmdId(cmdId), varId(varId), active(true), skip(false), nestedIfs(0), stopSkipOnWatchEnd(UNKNOWN_VAR) {}

};

struct Warning {
    BranchId branchId;
    CmdId cmdId;
    OpCode call;
    unsigned int argumentNr;
    VarId varId;
    TypeRange expectedRange;
    TypeRange actualRange;
};


class LocalContext {
private:
    AnalisysContext* ctx;
    BranchId branchId;
    const Command* command;
public:

    LocalContext(AnalisysContext* ctx): ctx(ctx), branchId(0), command(nullptr) {}

    void setup(BranchId branchId, const Command* command) {
        this->command = command;
        this->branchId = branchId;
    }

    bool isDefined(int argNr) {
        return command->hasArgument(argNr);
    }

    VarId getVarId(int argNr) const {
        return command->getArgument(argNr);
    }

    const TypeRange& get(int argNr) {
        const TypeRange* out = ctx->getVariable(command->getArgument(argNr)).getRange(branchId);
        return *out;
    }

    void set(int argNr, const TypeRange& newRange) {
        if (!isDefined(argNr)) return;
        ctx->getVariable(command->getArgument(argNr)).changeRange(branchId, command->cmdId, newRange, TR_Operation, nullptr);
    }

    void set(int argNr, const TypeRange& newRange, RestoreRange* restorer, unsigned int depArgNr) {
        if (!isDefined(argNr)) return;
        if (restorer != nullptr) {
            restorer->setDependent(getVarId(depArgNr));
            restorer->setChangedAt(command->cmdId);
        }
        ctx->getVariable(command->getArgument(argNr)).changeRange(branchId, command->cmdId, newRange, TR_Operation, restorer);
    }

    void set(int argNr, bool bval) {
        set(argNr, TypeRange(bval ? 1.0 : 0.0));
    }

    void createBranch(int argNr, const TypeRange& range) {
        if (!range.isValid() || !isDefined(argNr)) return;
        ctx->createBranch(branchId, command->cmdId, command->getArgument(argNr), range);
    }

    void addWarning(unsigned int argNr, TypeRange expectedRange) {
        ctx->addWarning(command, branchId, command->getArgument(argNr), argNr, expectedRange, get(argNr));
    }

    void onIf() {
        Branch* branch = ctx->getBranch(branchId);
        if (branch->skip) {
            branch->nestedIfs++;
        } else {
            TypeRange actual = get(0);
            TypeRange expected = command->range;
            branch->skip = actual != expected;
        }
    }

    void onEndIf() {
        Branch* branch = ctx->getBranch(branchId);

        if (branch->nestedIfs > 0) {
            branch->nestedIfs--;
        } else if (branch->skip && branch->stopSkipOnWatchEnd == UNKNOWN_VAR) {
            branch->skip = false;
        }
    }

    void onWatch(VarId id) {
        //actually do nothing
    }

    void onEndWatch(VarId id) {
        Branch* branch = ctx->getBranch(branchId);
        if (branch->skip && branch->stopSkipOnWatchEnd == id) {
            branch->skip = false;
            branch->stopSkipOnWatchEnd = UNKNOWN_VAR;
        }
    }

    void onIgnoreWatch(VarId id) {
        Branch* branch = ctx->getBranch(branchId);
        if (!branch->skip) {
            branch->skip = true;
            branch->stopSkipOnWatchEnd = id;
        }
    }

};