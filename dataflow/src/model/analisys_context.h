#pragma once
#include "defs.h"
#include "command.h"
#include "variable.h"
#include "ops.h"

struct Branch;
struct Warning;
struct VariableChange;

class AnalisysContext {
private:
    //todo: redo with linked lists. also there could be separate list of active variables/branches, 
    //to iterate over them only 
    Variable* variables[MAX_VARIABLES];
    VarId maxCreatedId;
    Branch* branches[MAX_BRANCHES];
    BranchId nextBranchId;
    Warning* warnings[MAX_WARNINGS];
    unsigned int warningsCount;

    void populateChanges(CmdId id, BranchId branchId, BranchId parentBranchId, VarId varId, const TypeRange& changedRange);
public:
    AnalisysContext();

    void createVariable(BranchId branchId, CmdId cmdId, VarId id, const TypeRange& range);
    void forgetVariable(VarId id);
    Variable& getVariable(VarId id) const; 
    BranchId createBranch(BranchId parentId, CmdId cmdId, VarId variable, const TypeRange& range);

    Branch* getFirstBranch() const;
    BranchId getLastBranchId() const;
    Branch* getNextBranch(Branch* branch) const;
    Branch* getBranch(BranchId id) const;

    void addWarning(Command* command, BranchId branchId, VarId variable, unsigned int argNr, TypeRange expected, TypeRange actual);

    Array<Warning> getWarnings() const;
    Array<Branch>  getBranches() const;
    Array<VariableChange> getChanges() const;


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

    Branch(){}

    Branch(BranchId id, BranchId parentId, CmdId cmdId, VarId varId): 
        id(id), parentId(parentId), cmdId(cmdId), varId(varId), active(true), skip(false), nestedIfs(0) {}

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
    Command* command;
public:

    LocalContext(AnalisysContext* ctx): ctx(ctx), branchId(0), command(NULL) {}

    void setup(BranchId branchId, Command* command) {
        this->command = command;
        this->branchId = branchId;
    }

    bool isDefined(int argNr) {
        return command->arguments[argNr] != 0;
    }

    VarId getVarId(int argNr) const {
        return command->arguments[argNr];
    }

    const TypeRange& get(int argNr) {
        const TypeRange* out = ctx->getVariable(command->arguments[argNr]).getRange(branchId);
        return *out;
    }

    void set(int argNr, const TypeRange& newRange) {
        if (!isDefined(argNr)) return;
        ctx->getVariable(command->arguments[argNr]).changeRange(branchId, command->cmdId, newRange, TR_Operation, NULL);
    }

    void set(int argNr, const TypeRange& newRange, RestoreRange* restorer, unsigned int depArgNr) {
        if (!isDefined(argNr)) return;
        if (restorer != NULL) {
            restorer->setDependent(getVarId(depArgNr));
            restorer->setChangedAt(command->cmdId);
        }
        ctx->getVariable(command->arguments[argNr]).changeRange(branchId, command->cmdId, newRange, TR_Operation, restorer);
    }

    void set(int argNr, bool bval) {
        set(argNr, TypeRange(bval ? 1.0 : 0.0));
    }

    void createBranch(int argNr, const TypeRange& range) {
        if (!range.isValid() || !isDefined(argNr)) return;
        ctx->createBranch(branchId, command->cmdId, command->arguments[argNr], range);
    }

    void addWarning(unsigned int argNr, TypeRange expectedRange) {
        ctx->addWarning(command, branchId, command->arguments[argNr], argNr, expectedRange, get(argNr));
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
        } else if (branch->skip) {
            branch->skip = false;
        }
    }

};