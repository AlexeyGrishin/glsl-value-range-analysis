#include "command.h"

Command::Command(CmdId cmdId, OpCode opCode): cmdId(cmdId), opCode(opCode), argumentsCount(0) 
{
    for (int i = 0; i < MAX_ARGS; i++) arguments[i] = 0;
}

void Command::setRange(TypeRange range)
{
    this->range = range;
}

void Command::addArgument(VarId arg)
{
    this->arguments[this->argumentsCount++] = arg;
}