#pragma once
#include "defs.h"
#include "opcodes.h"
#include "range.h"

struct Command {
    CmdId cmdId;
    OpCode opCode;
    VarId arguments[MAX_ARGS];
    unsigned int argumentsCount;
    TypeRange range; //for some commands like _define

    Command(CmdId cmdId, OpCode opCode);
    void setRange(TypeRange range);
    void addArgument(VarId arg);
};