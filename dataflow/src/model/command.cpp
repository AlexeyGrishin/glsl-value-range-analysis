#include "command.h"

Command::Command(CmdId cmdId, OpCode opCode): cmdId(cmdId), opCode(opCode)
{
}

void Command::setRange(TypeRange range)
{
    this->range = range;
}

void Command::addArgument(VarId arg)
{
    this->arguments.push_back(arg);
}