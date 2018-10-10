#pragma once
#include "defs.h"
#include "opcodes.h"
#include "range.h"
#include <vector>

class Command {
private:
    std::vector<VarId> arguments;

public:
    CmdId cmdId;
    OpCode opCode;
    TypeRange range; //for some commands like _define

    Command(CmdId cmdId, OpCode opCode);

    void setRange(const TypeRange& range);
    void addArgument(VarId arg);
    bool hasArgument(unsigned int nr) const;
    VarId getArgument(unsigned int nr) const;

};