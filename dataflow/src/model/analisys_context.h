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
    std::map<VarId, std::unique_ptr<Variable>> variables;
    VarId maxCreatedId;
    std::vector<std::unique_ptr<Branch>> branches;
    std::vector<Warning> warnings;

    void populateChanges(CmdId id, BranchId branchId, BranchId parentBranchId, VarId varId, const TypeRange& changedRange);
public:
    AnalisysContext();

    void createVariable(BranchId branchId, CmdId cmdId, VarId id, const TypeRange& range);
    const Variable* forgetVariable(VarId id);
    Variable* getVariable(VarId id); 
    const TypeRange* getRange(BranchId branchId, VarId varId) const;
    const Variable* getVariable(VarId id) const; 
    BranchId createBranch(BranchId parentId, CmdId cmdId, VarId variable, const TypeRange& range);

    std::vector<const Branch*> getActiveBranches() const;
    Branch* getBranch(BranchId id) const;

    void addWarning(const Command* command, BranchId branchId, VarId variable, unsigned int argNr, const TypeRange& expected, const TypeRange& actual);

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
    ProcessResult result;
public:

    LocalContext(AnalisysContext* ctx): ctx(ctx), branchId(0), command(nullptr), result(PR_OK) {}

    void start() {
        result = PR_OK;
    }

    bool hasErrors() {
        return result != PR_OK;
    }

    void onError(ProcessResult error) {
        if (!hasErrors()) {
            result = error;
        }
    }

    ProcessResult getStatus() {
        return result;
    }

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
        auto varId = getVarId(argNr);
        if (varId == UNKNOWN_VAR) {
            onError(PR_ABSENT_ARGUMENT); 
            return Zero;
        }
        auto var = ctx->getVariable(varId);
        if (var == nullptr) {
            onError(PR_UNKNOWN_VAR);
            return Zero;
        }
        return *var->getRange(branchId);
    }

    void set(int argNr, const TypeRange& newRange) {
        if (!isDefined(argNr)) return;
        auto var = ctx->getVariable(command->getArgument(argNr));
        if (var == nullptr) {
            onError(PR_UNKNOWN_VAR);
            return;
        }
        var->changeRange(branchId, command->cmdId, newRange, TR_Operation, nullptr);
    }

    void set(int argNr, const TypeRange& newRange, RestoreRange* restorer, unsigned int depArgNr) {
        if (!isDefined(argNr)) return;
        if (restorer != nullptr) {
            restorer->setDependent(getVarId(depArgNr));
            restorer->setChangedAt(command->cmdId);
        }
        auto var = ctx->getVariable(command->getArgument(argNr));
        if (var == nullptr) {
            onError(PR_UNKNOWN_VAR);
            return;
        }
        var->changeRange(branchId, command->cmdId, newRange, TR_Operation, restorer);
    }

    void set(int argNr, bool bval) {
        set(argNr, TypeRange(bval ? 1.0 : 0.0));
    }

    void createBranch(int argNr, const TypeRange& range) {
        if (!range.isValid() || !isDefined(argNr)) return;
        ctx->createBranch(branchId, command->cmdId, command->getArgument(argNr), range);
    }

    void addWarning(unsigned int argNr, const TypeRange& expectedRange) {
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