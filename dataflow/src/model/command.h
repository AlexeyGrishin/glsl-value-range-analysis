#pragma once
#include "defs.h"
#include "opcodes.h"
#include "range.h"
#include <vector>

struct Command {
    CmdId cmdId;
    OpCode opCode;
    std::vector<VarId> arguments;
    TypeRange range; //for some commands like _define

    Command(CmdId cmdId, OpCode opCode);
    void setRange(TypeRange range);
    void addArgument(VarId arg);
};