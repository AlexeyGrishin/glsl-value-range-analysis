//todo: move here structs and enums
#pragma once

typedef unsigned int CmdId;
typedef unsigned int BranchId;
typedef unsigned int VarId;

constexpr VarId UNKNOWN_VAR = 0;
constexpr double Eps = 1e-10;
constexpr BranchId MAIN_BRANCH = 0;

enum TypeReason {
    TR_Operation = 1,
    TR_Branch = 2,
    TR_BackPropagation = 3
};

//reproduces bug!
//#define NO_COPY_CC 
