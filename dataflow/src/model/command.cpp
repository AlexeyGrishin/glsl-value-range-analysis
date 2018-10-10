// This is an independent project of an individual developer. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

#include "command.h"

Command::Command(CmdId cmdId, OpCode opCode): cmdId(cmdId), opCode(opCode)
{
}

void Command::setRange(const TypeRange& range)
{
    this->range = range;
}

void Command::addArgument(VarId arg)
{
    this->arguments.push_back(arg);
}

bool Command::hasArgument(unsigned int nr) const
{
    return nr < this->arguments.size() && this->arguments[nr] != UNKNOWN_VAR;
}

VarId Command::getArgument(unsigned int nr) const
{
    return nr < this->arguments.size() ? this->arguments[nr] : UNKNOWN_VAR;
}