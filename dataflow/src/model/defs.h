//todo: move here structs and enums
#pragma once
#include <limits>

typedef unsigned int CmdId;
typedef unsigned int BranchId;
typedef unsigned int VarId;

constexpr VarId UNKNOWN_VAR = 0;
constexpr CmdId UNKNOWN_CMD = std::numeric_limits<unsigned int>::max();
constexpr double Eps = 1e-10;
constexpr BranchId MAIN_BRANCH = 0;

enum TypeReason {
    TR_Operation = 1,
    TR_Branch = 2,
    TR_BackPropagation = 3
};

enum ProcessResult {
    PR_OK = 0,
    PR_ERROR = 1,

    PR_UNKNOWN_OP = 2,
    PR_UNKNOWN_VAR = 3,
    PR_ABSENT_ARGUMENT = 4

};

//reproduces bug!
//#define NO_COPY_CC 
